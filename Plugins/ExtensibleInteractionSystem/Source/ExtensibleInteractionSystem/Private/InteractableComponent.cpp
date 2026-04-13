#include "InteractableComponent.h"
#include "InteractorComponent.h"
#include "InteractionRuleset.h"
#include "InteractionFocusHandler.h"
#include "InteractionProgressHandler.h"
#include "InteractionRegulationHandler.h"
#include "InteractionSettings.h"
#include "LogInteractionSystem.h"
#include "Net/UnrealNetwork.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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
	
	// Maybe this could all be a clean lambda. I couldn't figure it out though, so it'll have to be repetitive for now. At least it's straightforward to read.
	const UInteractionSettings* Settings = GetDefault<UInteractionSettings>();

	// Local Focus Handler
	if (LocalFocusHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultLocalFocusHandlerClass())
			LocalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, Default));

	// Global Focus Handler
	if (GlobalFocusHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultGlobalFocusHandlerClass())
			GlobalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, Default));

	// Local Progress Handler
	if (LocalProgressHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultLocalProgressHandlerClass())
			LocalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, Default));

	// Global Progress Handler
	if (GlobalProgressHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultGlobalProgressHandlerClass())
			GlobalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, Default));

	// Regulation Handler
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

// ============================================================
// SERVER-SIDE ENTRY POINTS
// These functions must only be called from the server,
// via UInteractorComponent's Server RPCs
// ============================================================

void UInteractableComponent::BeginInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	// CurrentInteractor must be set before the InteractState to ensure it arrives first in the
	// replication bunch, since OnRep_InteractState reads it.
	CurrentInteractors.AddUnique(Interactor);
	if(CurrentInteractors.Num() == 1)
		InteractState = EInteractionState::Interacting;

	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		if(GlobalProgressHandler)
			GlobalProgressHandler->HandleInteractionStart(this, Interactor, 0.f);
	
	OnBeginInteraction.Broadcast(Interactor);
}

void UInteractableComponent::FinishInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractors.Remove(Interactor);
	
	if(CurrentInteractors.IsEmpty())
		InteractState = EInteractionState::Idle;
	
	Multicast_OnInteractionFinished(Interactor, ProgressPercent);
}

void UInteractableComponent::CancelInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractors.Remove(Interactor);
	
	if(CurrentInteractors.IsEmpty())
		InteractState = EInteractionState::Idle;
	
	Multicast_OnInteractionCancelled(Interactor, ProgressPercent);
}

// ============================================================
// LOCAL-ONLY ENTRY POINTS
// Called by UInteractorComponent on the local client. Never involve the server or replication.
// ============================================================

void UInteractableComponent::FocusGained(UInteractorComponent* Interactor)
{
	OnFocusGained.Broadcast(Interactor);

	for (const auto& LocalFocusHandler : LocalFocusHandlers)
		if (LocalFocusHandler)
			LocalFocusHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::FocusLost(UInteractorComponent* Interactor)
{
	OnFocusLost.Broadcast(Interactor);
	
	for (const auto& LocalFocusHandler : LocalFocusHandlers)
		if (LocalFocusHandler)
			LocalFocusHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::InteractBegin(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& LocalProgressHandler : LocalProgressHandlers)
		if (LocalProgressHandler)
			LocalProgressHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractCancel(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& LocalProgressHandler  : LocalProgressHandlers)
		if (LocalProgressHandler)
			LocalProgressHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractFinish(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& LocalProgressHandler : LocalProgressHandlers)
		if (LocalProgressHandler)
			LocalProgressHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::UpdateProgress(UInteractorComponent* Interactor, const float ProgressPercent)
{
	UE_LOG(LogInteract, Log, TEXT("LocalProgressHandler index 0 is: %s."), LocalProgressHandlers[0] ? *LocalProgressHandlers[0]->GetName() : TEXT("null"));
	for (const auto& LocalProgressHandler : LocalProgressHandlers)
		if (LocalProgressHandler)
			LocalProgressHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
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

// ============================================================
// MULTICAST RPCs
// ============================================================

void UInteractableComponent::Multicast_OnInteractionBegun_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnBeginInteraction.Broadcast(Interactor);	

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionBegun called on %s"), *GetOwner()->GetName());

	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalProgressHandler : GlobalProgressHandlers)
		if(GlobalProgressHandler)
			GlobalProgressHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionFinished_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnFinishInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionFinished called on %s"), *GetOwner()->GetName());
	
	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		if(GlobalProgressHandler)
			GlobalProgressHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionCancelled_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnCancelInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionCancelled called on %s"), *GetOwner()->GetName());

	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		if(GlobalProgressHandler)
			GlobalProgressHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_FocusGained_Implementation(UInteractorComponent* Interactor)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalFocusHandler : GlobalFocusHandlers)
		if(GlobalFocusHandler)
			GlobalFocusHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::Multicast_FocusLost_Implementation(UInteractorComponent* Interactor)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalFocusHandler : GlobalFocusHandlers)
		if(GlobalFocusHandler)
			GlobalFocusHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::Multicast_UpdateProgress_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& GlobalProgressHandler : GlobalProgressHandlers)
		if(GlobalProgressHandler)
			GlobalProgressHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
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