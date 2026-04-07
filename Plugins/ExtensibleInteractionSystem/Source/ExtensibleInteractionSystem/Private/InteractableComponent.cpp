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

	// TODO: figure out how to use a lambda here instead of copying code 4 times

	// --- Set up LOCAL Focus Handler(s) ---
	if(LocalFocusHandlerClasses.Num() > 0)
		for (const TSubclassOf<UInteractionFocusHandler>& FocusHandlerClass : LocalFocusHandlerClasses)
			if (IsValid(FocusHandlerClass))
				LocalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, FocusHandlerClass));
			
	else 
	{// falls back to settings default if none is set
		if (const UInteractionSettings* Settings = GetDefault<UInteractionSettings>())
			if (TSubclassOf<UInteractionFocusHandler> DefaultFocusHandler = Settings->GetDefaultFocusHandlerClass())
				LocalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, DefaultFocusHandler));
	}
	
	// --- Set up GLOBAL Focus Handler(s) ---
	if(GlobalFocusHandlerClasses.Num() > 0)
		for (const TSubclassOf<UInteractionFocusHandler>& FocusHandlerClass : GlobalFocusHandlerClasses)
			if(IsValid(FocusHandlerClass))
				GlobalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, FocusHandlerClass));
	else 
	{// falls back to settings default if none is set
		if (const UInteractionSettings* Settings = GetDefault<UInteractionSettings>())
			if (TSubclassOf<UInteractionFocusHandler> DefaultFocusHandler = Settings->GetDefaultFocusHandlerClass())
				GlobalFocusHandlers.Add(NewObject<UInteractionFocusHandler>(this, DefaultFocusHandler));
	}

	// --- Set up LOCAL Progress Handler(s) ---
	if(LocalProgressHandlerClasses.Num() > 0)
		for (const TSubclassOf<UInteractionProgressHandler>& ProgressHandlerClass : LocalProgressHandlerClasses)
			if(IsValid(ProgressHandlerClass))
				LocalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, ProgressHandlerClass));
	else 
	{// falls back to settings default if none is set
		if (const UInteractionSettings* Settings = GetDefault<UInteractionSettings>())
			if (TSubclassOf<UInteractionProgressHandler> DefaultProgressHandler = Settings->GetDefaultProgressHandlerClass())
				LocalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, DefaultProgressHandler));
	}
	
	// --- Set up GLOBAL Progress Handler(s) ---
	if(GlobalProgressHandlerClasses.Num() > 0)
		for (const TSubclassOf<UInteractionProgressHandler>& ProgressHandlerClass : GlobalProgressHandlerClasses)
			if(IsValid(ProgressHandlerClass))
				GlobalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, ProgressHandlerClass));
	else 
	{// falls back to settings default if none is set
		if (const UInteractionSettings* Settings = GetDefault<UInteractionSettings>())
			if (TSubclassOf<UInteractionProgressHandler> DefaultProgressHandler = Settings->GetDefaultProgressHandlerClass())
				GlobalProgressHandlers.Add(NewObject<UInteractionProgressHandler>(this, DefaultProgressHandler));
	}
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

void UInteractableComponent::InteractBegin(UInteractorComponent* Interactor, float ProgressPercent)
{
	for (const auto& ProgressHandler : LocalProgressHandlers)
		ProgressHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractCancel(UInteractorComponent* Interactor, float ProgressPercent)
{
	for (const auto& LocalProgressHandler  : LocalProgressHandlers)
		LocalProgressHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::InteractFinish(UInteractorComponent* Interactor, float ProgressPercent)
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

void UInteractableComponent::Multicast_OnInteractionFinished_Implementation(UInteractorComponent* Interactor, float ProgressPercent)
{
	OnFinishInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionFinished called on %s"), *GetOwner()->GetName());
	
	// skip on the owning client - they already ran local handlers directly
	if (IsValid(Interactor))
		if (const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner()))
			if (OwningPawn->IsLocallyControlled())
				return;
	
	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		GlobalProgressHandler->HandleInteractionFinished(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionCancelled_Implementation(UInteractorComponent* Interactor, float ProgressPercent)
{
	OnCancelInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionCancelled called on %s"), *GetOwner()->GetName());

	// skip on the owning client - they already ran local handlers directly
	if (IsValid(Interactor))
		if (const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner()))
			if (OwningPawn->IsLocallyControlled())
				return;
	
	for(const auto& GlobalProgressHandler : GlobalProgressHandlers)
		GlobalProgressHandler->HandleInteractionCancelled(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_FocusGained_Implementation(UInteractorComponent* Interactor)
{
	// skip on the owning client - they already ran local handlers directly
	if (IsValid(Interactor))
		if (const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner()))
			if (OwningPawn->IsLocallyControlled())
				return;
	
	for (const auto& FocusHandler : GlobalFocusHandlers)
		FocusHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::Multicast_FocusLost_Implementation(UInteractorComponent* Interactor)
{
	// skip on the owning client - they already ran local handlers directly
	if (IsValid(Interactor))
		if (const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner()))
			if (OwningPawn->IsLocallyControlled())
				return;
	
	for (const auto& FocusHandler : GlobalFocusHandlers)
		FocusHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::Multicast_UpdateProgress_Implementation(UInteractorComponent* Interactor, const float ProgressPercent)
{
	// skip on the owning client - they already ran local handlers directly
	if (IsValid(Interactor))
		if (const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner()))
			if (OwningPawn->IsLocallyControlled())
				return;
	
	for (const auto& ProgressHandler : GlobalProgressHandlers)
		ProgressHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
}

void UInteractableComponent::Multicast_OnInteractionBeginning_Implementation(UInteractorComponent* Interactor, float ProgressPercent)
{
	// skip on the owning client - they already ran local handlers directly
	if (IsValid(Interactor))
		if (const APawn* OwningPawn = Cast<APawn>(Interactor->GetOwner()))
			if (OwningPawn->IsLocallyControlled())
				return;
	
	for (const auto& ProgressHandler : GlobalProgressHandlers)
		ProgressHandler->HandleInteractionStart(this, Interactor, ProgressPercent);
}

