#include "InteractableComponent.h"
#include "InteractorComponent.h"
#include "InteractionRuleset.h"
#include "InteractionFocusHandler.h"
#include "InteractionProgressHandler.h"
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

	// Maybe this could all be a clean lambda. I couldn't figure it out though, so it'll have to be repetitive for now. At least it's straightforward to read.
	const UInteractionSettings* Settings = GetDefault<UInteractionSettings>();

	// Local Focus Handlers
	for (const TSubclassOf<UInteractionFocusHandler>& Class : LocalFocusHandlerClasses)
		if (Class) LocalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, Class));
	if (LocalFocusHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultLocalFocusHandlerClass())
			LocalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, Default));

	// Global Focus Handlers
	for (const TSubclassOf<UInteractionFocusHandler>& Class : GlobalFocusHandlerClasses)
		if (Class) GlobalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, Class));
	if (GlobalFocusHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultGlobalFocusHandlerClass())
			GlobalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, Default));

	// Local Progress Handlers
	for (const TSubclassOf<UInteractionProgressHandler>& Class : LocalProgressHandlerClasses)
		if (Class) LocalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, Class));
	if (LocalProgressHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultLocalProgressHandlerClass())
			LocalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, Default));

	// Global Progress Handlers
	for (const TSubclassOf<UInteractionProgressHandler>& Class : GlobalProgressHandlerClasses)
		if (Class) GlobalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, Class));
	if (GlobalProgressHandlers.IsEmpty() && Settings)
		if (auto Default = Settings->GetDefaultGlobalProgressHandlerClass())
			GlobalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, Default));
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractableComponent, InteractState);
	DOREPLIFETIME(UInteractableComponent, CurrentInteractor);
}

// ============================================================
// SERVER-SIDE ENTRY POINTS
// These functions must only be called from the server,
// via UInteractorComponent's Server RPCs
// ============================================================

void UInteractableComponent::BeginInteraction(UInteractorComponent* Interactor)
{
	// CurrentInteractor must be set before the InteractState to ensure it arrives first in the
	// replication bunch, since OnRep_InteractState reads it.
	CurrentInteractor = Interactor;
	InteractState = EInteractionState::Interacting;

	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		GlobalProgressHandler->HandleInteractionStart(this, Interactor, 0.f);

	// OnRep does not fire on server, so the delegate is broadcast directly for any server-side listeners.
	OnBeginInteraction.Broadcast(Interactor);

	Multicast_OnInteractionBeginning(Interactor, 0.f);
}

void UInteractableComponent::FinishInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractor = nullptr;
	InteractState = EInteractionState::Idle;

	// TODO: Decrement AllowedTriggers and begin Cooldown timer

	// Multicast fires on server and all clients, no separate server broadcast is needed.
	Multicast_OnInteractionFinished(Interactor, ProgressPercent);
}

void UInteractableComponent::CancelInteraction(UInteractorComponent* Interactor, float ProgressPercent)
{
	CurrentInteractor = nullptr;
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
		LocalFocusHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::FocusLost(UInteractorComponent* Interactor)
{
	OnFocusLost.Broadcast(Interactor);
	
	for (const auto& LocalFocusHandler : LocalFocusHandlers)
		LocalFocusHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::InteractBegin(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& ProgressHandler : LocalProgressHandlers)
		ProgressHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractCancel(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& LocalProgressHandler  : LocalProgressHandlers)
		LocalProgressHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractFinish(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& ProgressHandler : LocalProgressHandlers)
		ProgressHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::UpdateProgress(UInteractorComponent* Interactor, const float ProgressPercent)
{
	for (const auto& ProgressHandler : LocalProgressHandlers)
		ProgressHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
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
// ONREP
// ============================================================

void UInteractableComponent::OnRep_InteractState()
{
	switch (InteractState)
	{
		case EInteractionState::Idle:
			// Deliberately empty - Finished and Cancelled arrive via multicast RPCs rather than state change
			break;
		case EInteractionState::Interacting:
			// CurrentInteractor should be valid here as it replicates alongside InteractState.
			// In rare cases it might not, meaning the broadcast can fire with nullptr - handlers should be robust to this possibility.
			OnBeginInteraction.Broadcast(CurrentInteractor);
			break;
	}
	UE_LOG(LogInteract, Log, TEXT("OnRep_InteractState called on %s, new state: %s"), *GetOwner()->GetName(), *UEnum::GetValueAsString(InteractState));
}

// ============================================================
// MULTICAST RPCs
// ============================================================

void UInteractableComponent::Multicast_OnInteractionFinished_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnFinishInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionFinished called on %s"), *GetOwner()->GetName());
	
	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		GlobalProgressHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionCancelled_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	OnCancelInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionCancelled called on %s"), *GetOwner()->GetName());

	if(IsLocallyInstigated(Interactor))
		return;
	
	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		GlobalProgressHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_FocusGained_Implementation(UInteractorComponent* Interactor)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& FocusHandler : GlobalFocusHandlers)
		FocusHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::Multicast_FocusLost_Implementation(UInteractorComponent* Interactor)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& FocusHandler : GlobalFocusHandlers)
		FocusHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::Multicast_UpdateProgress_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& ProgressHandler : GlobalProgressHandlers)
		ProgressHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionBeginning_Implementation(UInteractorComponent* Interactor, float ProgressPercent)
{
	if(IsLocallyInstigated(Interactor))
		return;
	
	for (const auto& ProgressHandler : GlobalProgressHandlers)
		ProgressHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

// ============================================================
// Helpers
// ============================================================

bool UInteractableComponent::IsLocallyInstigated(const UInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
		return false;
	const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner());
	return OwningPawn->IsLocallyControlled();
}