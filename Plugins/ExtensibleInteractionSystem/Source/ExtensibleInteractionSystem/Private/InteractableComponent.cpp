#include "InteractableComponent.h"
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
			RegulationHandler = NewObject<UInteractionRegulationHandler>(this, Default);
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

	const bool bNewCanFocus = IsFocusable(CachedLocalFocusInteractor);
	const bool bNewCanInteract = IsInteractable(CachedLocalFocusInteractor);

	if (bNewCanFocus != bCachedCanFocus || bNewCanInteract != bCachedCanInteract)
	{
		bCachedCanFocus = bNewCanFocus;
		bCachedCanInteract = bNewCanInteract;

		for (auto& Handler : LocalVisualHandlers)
			Handler->HandleInteractionStateChanged(this, CachedLocalFocusInteractor, bNewCanFocus, bNewCanInteract);
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

	if (RegulationHandler)
		RegulationHandler->OwnerInteractStart(this, Interactor);
	
	Multicast_OnInteractionBegun(Interactor, ProgressPercent);
}

void UInteractableComponent::FinishInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractors.Remove(Interactor);
	
	if(CurrentInteractors.IsEmpty())
		InteractState = EInteractionState::Idle;

	if (RegulationHandler)
		RegulationHandler->OwnerInteractFinish(this, Interactor);
	
	Multicast_OnInteractionFinished(Interactor, ProgressPercent);
}

void UInteractableComponent::CancelInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractors.Remove(Interactor);
	
	if(CurrentInteractors.IsEmpty())
		InteractState = EInteractionState::Idle;

	if (RegulationHandler)
		RegulationHandler->OwnerInteractCancel(this, Interactor);
	
	Multicast_OnInteractionCancelled(Interactor, ProgressPercent);
}

// ============================================================
// LOCAL-ONLY ENTRY POINTS
// Called by UInteractorComponent on the local client. Never involve the server or replication.
// ============================================================

void UInteractableComponent::FocusGained(UInteractorComponent* Interactor)
{
	// Enable tick only while focused — no wasted evaluation otherwise
	SetComponentTickEnabled(true);

	if (RegulationHandler)
		RegulationHandler->OwnerFocusGained(this, Interactor);
	
	// Cache initial state so the first tick has something to diff against
	bCachedCanFocus = IsFocusable(Interactor);
	bCachedCanInteract = IsInteractable(Interactor);
	CachedLocalFocusInteractor = Interactor;
	
	OnFocusGained.Broadcast(Interactor);

	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::FocusLost(UInteractorComponent* Interactor)
{
	SetComponentTickEnabled(false);

	if (RegulationHandler)
		RegulationHandler->OwnerFocusLost(this, Interactor);

	CachedLocalFocusInteractor = nullptr;
	
	OnFocusLost.Broadcast(Interactor);
	
	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::InteractBegin(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractCancel(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& LocalVisualHandler  : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractFinish(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& LocalVisualHandler : LocalVisualHandlers)
		if (LocalVisualHandler)
			LocalVisualHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::UpdateProgress(UInteractorComponent* Interactor, const float ProgressPercent)
{
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

bool UInteractableComponent::IsFocusable(UInteractorComponent* Interactor) const
{
	if(bDisableFocus)
		return false;

	const UInteractionRuleset* Ruleset = GetRuleset();
	if(Ruleset && Ruleset->bDisableFocus)
		return false;

	if (IsValid(RegulationHandler))
	{
		// Global — reads replicated state, same result for all players
		if (!RegulationHandler->CanBeFocused_Global(this, Interactor))
			return false;

		// Local — reads per-player state, can differ per client
		if (!RegulationHandler->CanBeFocused_Local(this, Interactor))
			return false;
	}
	
	return true;
}

bool UInteractableComponent::IsInteractable(UInteractorComponent* Interactor) const
{
	if(bDisableInteraction)
		return false;

	const UInteractionRuleset* Ruleset = GetRuleset();
	if(Ruleset && Ruleset->bDisableInteraction)
		return false;

	if(IsValid(RegulationHandler))
	{
		if(!RegulationHandler->CanInteract_Global(this, Interactor))
			return false;

		if(!RegulationHandler->CanInteract_Local(this, Interactor))
			return false;
	}

	return true;
}

// ============================================================
// MULTICAST RPCs
// ============================================================

void UInteractableComponent::Multicast_OnInteractionBegun_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	
	OnBeginInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionBegun called on %s"), *GetOwner()->GetName());

	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionFinished_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnFinishInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionFinished called on %s"), *GetOwner()->GetName());
	
	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionCancelled_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnCancelInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionCancelled called on %s"), *GetOwner()->GetName());

	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_FocusGained_Implementation(UInteractorComponent* Interactor)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::Multicast_FocusLost_Implementation(UInteractorComponent* Interactor)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalVisualHandler : GlobalVisualHandlers)
		if(GlobalVisualHandler)
			GlobalVisualHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::Multicast_UpdateProgress_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
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