#include "InteractableComponent.h"
#include "InteractionDeniedContext.h"
#include "InteractionDeniedGameplayTags.h"
#include "InteractorComponent.h"
#include "InteractionRuleset.h"
#include "InteractionRegulationHandler.h"
#include "InteractionSettings.h"
#include "InteractionVisualHandler.h"
#include "LogInteractionSystem.h"
#include "Net/UnrealNetwork.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	UActorComponent::SetComponentTickEnabled(false);
}

// ============================================================
// Lifecycle
// ============================================================

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	// Dedicated servers don't need to set up handlers, which are purely for visuals and client-side effects.
	// They also won't run any focus events, so we can skip all of this.
	if(GetNetMode() == NM_DedicatedServer)
		return;

	if(!bFallBackToDefaultHandlers)
		return;
	
	const UInteractionSettings* Settings = GetDefault<UInteractionSettings>();

	// Default LocalVisualHandler
	if (LocalVisualHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultLocalVisualHandlerClass())
			LocalVisualHandlers.Add(NewObject<UInteractionVisualHandler>(this, Default));

	// Default GlobalVisualHandler
	if (GlobalVisualHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultGlobalVisualHandlerClass())
			GlobalVisualHandlers.Add(NewObject<UInteractionVisualHandler>(this, Default));
	
	// DefaultRegulationHandler
	if (!RegulationHandler && Settings)
		if (auto Default = Settings->GetDefaultRegulationHandlerClass())
		{
			RegulationHandler = NewObject<UInteractionRegulationHandler>(GetOwner(), Default);
			RegulationHandler->RegisterComponent();
		}
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractableComponent, InteractState);
	DOREPLIFETIME(UInteractableComponent, CurrentInteractors);
}

void UInteractableComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only run on the local interactor
	if(!IsValid(CachedLocalFocusInteractor))
		return;

	FInteractionDeniedContext FocusContext = FInteractionDeniedContext();
	FInteractionDeniedContext InteractContext = FInteractionDeniedContext();
	
	const bool bNewCanFocus = IsFocusable(CachedLocalFocusInteractor, FocusContext);
	const bool bNewCanInteract = IsInteractable(CachedLocalFocusInteractor, InteractContext);
	
	if(bNewCanFocus != bCachedCanFocus)
	{
		bCachedCanFocus = bNewCanFocus;
		for (auto& LocalVisualHandler : LocalVisualHandlers)
			if(LocalVisualHandler)
				LocalVisualHandler->HandleInteractionStateChanged(this, CachedLocalFocusInteractor, bNewCanFocus, bNewCanInteract, FocusContext);
	}
	
	if (bNewCanInteract != bCachedCanInteract)
	{
		bCachedCanInteract = bNewCanInteract;

		for (auto& LocalVisualHandler : LocalVisualHandlers)
			if(LocalVisualHandler)
				LocalVisualHandler->HandleInteractionStateChanged(this, CachedLocalFocusInteractor, bNewCanFocus, bNewCanInteract, InteractContext);

		if (!bNewCanInteract)
		{
			const UInteractionRuleset* Ruleset = GetRuleset();
			if (Ruleset && Ruleset->bCancelOnInteractabilityLost)
				if (IsValid(CachedLocalFocusInteractor))
					CachedLocalFocusInteractor->RequestCancelInteraction(this);
		}
	}
}

// ============================================================
// SERVER-SIDE ENTRY POINTS
// These functions must only be called from the server,
// via UInteractorComponent's Server RPCs
// ============================================================

void UInteractableComponent::BeginInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractors.AddUnique(Interactor);
	if(CurrentInteractors.Num() == 1)
		InteractState = EInteractionState::Interacting;
	UInteractionRegulationHandler* RegHandler = GetOwner()->FindComponentByClass<UInteractionRegulationHandler>();
	if (RegHandler)
		RegHandler->OwnerInteractStart(this, Interactor);
	
	Multicast_OnInteractionBegun(Interactor, ProgressPercent);
}

void UInteractableComponent::FinishInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractors.Remove(Interactor);
	
	if(CurrentInteractors.IsEmpty())
		InteractState = EInteractionState::Idle;

	UInteractionRegulationHandler* RegHandler = GetOwner()->FindComponentByClass<UInteractionRegulationHandler>();
	if (RegHandler)
		RegHandler->OwnerInteractFinish(this, Interactor);
	
	Multicast_OnInteractionFinished(Interactor, ProgressPercent);
}

void UInteractableComponent::CancelInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractors.Remove(Interactor);
	
	if(CurrentInteractors.IsEmpty())
		InteractState = EInteractionState::Idle;

	UInteractionRegulationHandler* RegHandler = GetOwner()->FindComponentByClass<UInteractionRegulationHandler>();
	if (RegHandler)
		RegHandler->OwnerInteractCancel(this, Interactor);
	
	Multicast_OnInteractionCancelled(Interactor, ProgressPercent);
}

// ============================================================
// LOCAL-ONLY ENTRY POINTS
// Called by UInteractorComponent on the local client. Never involve the server or replication.
// ============================================================

void UInteractableComponent::FocusGained(UInteractorComponent* Interactor)
{
	UE_LOG(LogInteract, VeryVerbose, TEXT
		("UInteractableComponent::FocusGained called on %s via %s"),
		*GetOwner()->GetName(), *Interactor->GetOwner()->GetName());
	
	// Enable tick only while focused — no wasted evaluation otherwise
	SetComponentTickEnabled(true);

	UInteractionRegulationHandler* RegHandler = GetOwner()->FindComponentByClass<UInteractionRegulationHandler>();
	if (RegHandler)
		RegHandler->OwnerFocusGained(this, Interactor);

	FInteractionDeniedContext DeniedContext = FInteractionDeniedContext();
	
	// Cache initial state so the first tick has something to diff against
	CachedLocalFocusInteractor = Interactor;
	bCachedCanFocus = IsFocusable(Interactor, DeniedContext);
	bCachedCanInteract = IsInteractable(Interactor, DeniedContext);
	
	OnFocusGained.Broadcast(Interactor);

	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleFocusGained(this, Interactor, bCachedCanInteract, DeniedContext);
}

void UInteractableComponent::FocusLost(UInteractorComponent* Interactor)
{
	UE_LOG(LogInteract, VeryVerbose, TEXT
		("UInteractableComponent::FocusLost called on %s via %s"),
		*GetOwner()->GetName(), *Interactor->GetOwner()->GetName());
	
	SetComponentTickEnabled(false);

	UInteractionRegulationHandler* RegHandler = GetOwner()->FindComponentByClass<UInteractionRegulationHandler>();
	if (RegHandler)
		RegHandler->OwnerFocusLost(this, Interactor);

	CachedLocalFocusInteractor = nullptr;
	
	OnFocusLost.Broadcast(Interactor);
	
	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::InteractBegin(UInteractorComponent* Interactor, const float ProgressPercent)
{
	UE_LOG(LogInteract, Log, TEXT
		("UInteractableComponent::InteractBegin called on %s via %s"),
		*GetOwner()->GetName(), *Interactor->GetOwner()->GetName());
	
	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractCancel(UInteractorComponent* Interactor, const float ProgressPercent)
{
	UE_LOG(LogInteract, Log, TEXT
		("UInteractableComponent::InteractCancel called on %s via %s"),
		*GetOwner()->GetName(), *Interactor->GetOwner()->GetName());
	
	for (const auto& LocalVisualHandler  : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractFinish(UInteractorComponent* Interactor, const float ProgressPercent)
{
	UE_LOG(LogInteract, Log, TEXT
		("UInteractableComponent::InteractFinish called on %s via %s"),
		*GetOwner()->GetName(), *Interactor->GetOwner()->GetName());
	
	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::UpdateProgress(UInteractorComponent* Interactor, const float ProgressPercent)
{
	UE_LOG(LogInteract, VeryVerbose, TEXT
		("UInteractableComponent::UpdateProgress called on %s via %s"),
		*GetOwner()->GetName(), *Interactor->GetOwner()->GetName());
	
	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
}

// ============================================================
// Ruleset
// ============================================================

const UInteractionRuleset* UInteractableComponent::GetRuleset() const
{
	// Step 1: Check per-instance override on this specific component
	if(InstanceRuleset)
		return InstanceRuleset;

	// Step 2: Project-wide default configured in ProjectSettings > Interaction System
	if (const UInteractionSettings* Settings = GetDefault<UInteractionSettings>())
		if (const UInteractionRuleset* ProjectDefault = Settings->GetDefaultRuleset())
			return ProjectDefault;

	//Step 3: CDO hard-coded fallback - always valid, holds the property defaults
	return GetDefault<UInteractionRuleset>();
}

bool UInteractableComponent::IsFocusable(UInteractorComponent* Interactor, FInteractionDeniedContext& OutDeniedContext)
{
	if (bDisableFocus)
	{
		OutDeniedContext = FInteractionDeniedContext(
			TAG_Interaction_Denied_NotFocusable,
			NSLOCTEXT("ExtensibleInteractionSystem", "NotInteractable", "Cannot interact"),
			this);
		return false;
	}

	const UInteractionRuleset* Ruleset = GetRuleset();
	if (Ruleset && Ruleset->bDisableFocus)
	{
		OutDeniedContext = FInteractionDeniedContext(
			TAG_Interaction_Denied_NotFocusable,
			NSLOCTEXT("ExtensibleInteractionSystem", "NotInteractable", "Cannot interact"),
			this);
		return false;
	}

	UInteractionRegulationHandler* RegHandler = GetOwner()->FindComponentByClass<UInteractionRegulationHandler>();
	if (IsValid(RegHandler))
	{
		if (!RegHandler->CanBeFocused_Global(this, Interactor, OutDeniedContext))
			return false;
		if (!RegHandler->CanBeFocused_Local(this, Interactor, OutDeniedContext))
			return false;
	}
	return true;
}

bool UInteractableComponent::IsInteractable(UInteractorComponent* Interactor, FInteractionDeniedContext& OutDeniedContext)
{
	if(bDisableInteraction)
		return false;

	const UInteractionRuleset* Ruleset = GetRuleset();
	if(Ruleset && Ruleset->bDisableInteraction)
		return false;

	UInteractionRegulationHandler* RegHandler = GetOwner()->FindComponentByClass<UInteractionRegulationHandler>();
	if(IsValid(RegHandler))
	{
		if(!RegHandler->CanInteract_Global(this, Interactor, OutDeniedContext))
			return false;

		if(!RegHandler->CanInteract_Local(this, Interactor, OutDeniedContext))
			return false;
	}
	return true;
}

bool UInteractableComponent::EvaluateInteractionGates(UInteractorComponent* Interactor, FInteractionDeniedContext& OutContext, bool bNotifyDisplayHandlers)
{
	bool bAllowed = true;

	if (bDisableInteraction)
	{
		OutContext = FInteractionDeniedContext(
			TAG_Interaction_Denied_NotInteractable,
			FText::FromString("Cannot interact"), this);
		bAllowed = false;
	}
	else if (UInteractionRegulationHandler* RegHandler =
		GetOwner()->FindComponentByClass<UInteractionRegulationHandler>())
	{
		if (!RegHandler->CanInteract_Global(this, Interactor, OutContext))
			bAllowed = false;
		else if (!RegHandler->CanInteract_Local(this, Interactor, OutContext))
			bAllowed = false;
	}
	
	if (!bAllowed)
	{
		if (bNotifyDisplayHandlers)
			for (auto& LocalVisualHandler : LocalVisualHandlers)
				if (LocalVisualHandler)
					LocalVisualHandler->HandleInteractionDenied(this, Interactor, OutContext);

		OnInteractionDenied.Broadcast(Interactor, OutContext);
	}

	UE_LOG(LogInteract, Verbose, TEXT
		("EvaluateInteractionGates called on %s via %s. Allowed: %s. Reason: %s"),
		*GetOwner()->GetName(),
		IsValid(Interactor) ? *Interactor->GetName() : TEXT("None(NullObject)"),
		bAllowed ? TEXT("true") : TEXT("false"),
		*OutContext.Reason.ToString());
	
	return bAllowed;
}

// ============================================================
// MULTICAST RPCs
// ============================================================

void UInteractableComponent::Multicast_OnInteractionBegun_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnBeginInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Verbose, TEXT
		("Multicast_OnInteractionBegun called on %s via %s"),
		*GetOwner()->GetName(),
		IsValid(Interactor) ? *Interactor->GetName() : TEXT("None(NullObject)"));

	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionFinished_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnFinishInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Verbose, TEXT
		("Multicast_OnInteractionFinished called on %s via %s"),
		*GetOwner()->GetName(),
		IsValid(Interactor) ? *Interactor->GetName() : TEXT("None(NullObject)"));
	
	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionCancelled_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnCancelInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Verbose, TEXT
		("Multicast_OnInteractionCancelled called on %s via %s"),
		*GetOwner()->GetName(),
		IsValid(Interactor) ? *Interactor->GetName() : TEXT("None(NullObject)"));

	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_FocusGained_Implementation(UInteractorComponent* Interactor)
{
	UE_LOG(LogInteract, Verbose, TEXT
		("Multicast_FocusGained called on %s via %s"),
		*GetOwner()->GetName(),
		IsValid(Interactor) ? *Interactor->GetName() : TEXT("None(NullObject)"));
	
	if(IsLocallyInstigated(Interactor))
		return;

	FInteractionDeniedContext DeniedContext = FInteractionDeniedContext();
	
	for (const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleFocusGained(this, Interactor, IsInteractable(Interactor, DeniedContext), DeniedContext);
}

void UInteractableComponent::Multicast_FocusLost_Implementation(UInteractorComponent* Interactor)
{
	UE_LOG(LogInteract, Verbose, TEXT
		("Multicast_FocusLost called on %s via %s"),
		*GetOwner()->GetName(),
		IsValid(Interactor) ? *Interactor->GetName() : TEXT("None(NullObject)"));
	
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::Multicast_UpdateProgress_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnUpdateProgress.Broadcast(Interactor, ProgressPercent);
	
	UE_LOG(LogInteract, Verbose, TEXT
		("Multicast_UpdateProgress called on %s via %s"),
		*GetOwner()->GetName(),
		IsValid(Interactor) ? *Interactor->GetName() : TEXT("None(NullObject)"));
	
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
}

// ============================================================
// Helpers
// ============================================================

bool UInteractableComponent::IsLocallyInstigated(const UInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
		return false;
	
	const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner());
	
	return OwningPawn && OwningPawn->IsLocallyControlled();
}

// ============================================================
// Setters
// ============================================================

void UInteractableComponent::SetLocalVisualHandlers(const TArray<TSubclassOf<UInteractionVisualHandler>>& NewVisualHandlers)
{
	LocalVisualHandlers.Empty(NewVisualHandlers.Num());

	for (const TSubclassOf<UInteractionVisualHandler>& HandlerClass : NewVisualHandlers)
	{
		if (!HandlerClass)
			continue;
		LocalVisualHandlers.Add(NewObject<UInteractionVisualHandler>(this, HandlerClass));
	}
}

UInteractionVisualHandler* UInteractableComponent::AddLocalVisualHandler(TSubclassOf<UInteractionVisualHandler> VisualHandlerToAdd)
{
	if (!VisualHandlerToAdd)
		return nullptr;

	UInteractionVisualHandler* NewHandler = NewObject<UInteractionVisualHandler>(this, VisualHandlerToAdd);
	LocalVisualHandlers.Add(NewHandler);
	return NewHandler;
}

bool UInteractableComponent::RemoveLocalVisualHandler(UInteractionVisualHandler* VisualHandlerToRemove)
{
	if (!VisualHandlerToRemove)
		return false;

	return LocalVisualHandlers.Remove(VisualHandlerToRemove) > 0;
}

bool UInteractableComponent::RemoveLocalVisualHandlerAt(const int Index)
{
	if (!LocalVisualHandlers.IsValidIndex(Index))
		return false;

	LocalVisualHandlers.RemoveAt(Index);
	return true;
}

void UInteractableComponent::SetGlobalVisualHandlers(const TArray<TSubclassOf<UInteractionVisualHandler>>& NewVisualHandlers)
{
	LocalVisualHandlers.Empty(NewVisualHandlers.Num());

	for (const TSubclassOf<UInteractionVisualHandler>& HandlerClass : NewVisualHandlers)
	{
		if (!HandlerClass)
			continue;
		LocalVisualHandlers.Add(NewObject<UInteractionVisualHandler>(this, HandlerClass));
	}
}

UInteractionVisualHandler* UInteractableComponent::AddGlobalVisualHandler(TSubclassOf<UInteractionVisualHandler> VisualHandlerToAdd)
{
	if (!VisualHandlerToAdd)
		return nullptr;

	UInteractionVisualHandler* NewHandler = NewObject<UInteractionVisualHandler>(this, VisualHandlerToAdd);
	LocalVisualHandlers.Add(NewHandler);
	return NewHandler;
}

bool UInteractableComponent::RemoveGlobalVisualHandler(UInteractionVisualHandler* VisualHandlerToRemove)
{
	if (!VisualHandlerToRemove)
		return false;

	return LocalVisualHandlers.Remove(VisualHandlerToRemove) > 0;
}

bool UInteractableComponent::RemoveGlobalVisualHandlerAt(const int Index)
{
	if (!LocalVisualHandlers.IsValidIndex(Index))
		return false;

	LocalVisualHandlers.RemoveAt(Index);
	return true;
}

void UInteractableComponent::SetRegulationHandler(const TSubclassOf<UInteractionRegulationHandler>& NewRegulationHandler)
{
	if (!NewRegulationHandler)
	{
		if (RegulationHandler)
		{
			RegulationHandler->DestroyComponent();
			RegulationHandler = nullptr;
		}
		return;
	}

	if (RegulationHandler)
		RegulationHandler->DestroyComponent();

	RegulationHandler = NewObject<UInteractionRegulationHandler>(GetOwner(), NewRegulationHandler);
	RegulationHandler->RegisterComponent();
}
