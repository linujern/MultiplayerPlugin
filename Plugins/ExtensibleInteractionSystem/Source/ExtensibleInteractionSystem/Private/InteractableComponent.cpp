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

/*
 * Lifecycle
 */

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	const bool bIsLocal = GetNetMode() != NM_DedicatedServer;

	if (!bIsLocal)
		return;
	
	if(FocusHandlerClass)
		FocusHandler = NewObject<UInteractionFocusHandler>(this, FocusHandlerClass);
	else
		{}// TODO: fallback to UInteractionSettings DefaultFocusHandlerClass
	if(ProgressHandlerClass)
		ProgressHandler = NewObject<UInteractionProgressHandler>(this, ProgressHandlerClass);
	else
		{}// TODO: fallback to UInteractionSettings DefaultProgressHandlerClass
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractableComponent, InteractState);
	DOREPLIFETIME(UInteractableComponent, CurrentInteractor);
}

/*
 * Server-side entry points
 */

void UInteractableComponent::BeginInteraction(UInteractorComponent* Interactor)
{
	CurrentInteractor = Interactor;
	InteractState = EInteractionState::Interacting;

	// OnRep does not fire on server - call directly
	OnBeginInteraction.Broadcast(Interactor);
}

void UInteractableComponent::FinishInteraction(UInteractorComponent* Interactor)
{
	CurrentInteractor = nullptr;
	InteractState = EInteractionState::Idle;

	// TODO: handle allowedtriggers and cooldown

	Multicast_OnInteractionFinished(Interactor);
}

void UInteractableComponent::CancelInteraction(UInteractorComponent* Interactor)
{
	CurrentInteractor = nullptr;
	InteractState = EInteractionState::Idle;
	
	Multicast_OnInteractionCancelled(Interactor);
}

/*
 * Local-only entry points
 */

void UInteractableComponent::FocusGained(UInteractorComponent* Interactor)
{
	OnFocusGained.Broadcast(Interactor);

	if(FocusHandler)
		FocusHandler->HandleFocusGained(this, Interactor);
}

void UInteractableComponent::FocusLost(UInteractorComponent* Interactor)
{
	OnFocusLost.Broadcast(Interactor);

	if(FocusHandler)
		FocusHandler->HandleFocusLost(this, Interactor);
}

void UInteractableComponent::UpdateProgress(UInteractorComponent* Interactor, const float ProgressPercent)
{
	if(ProgressHandler)
		ProgressHandler->HandleProgressUpdate(this, Interactor, ProgressPercent);
}

/*
 * Ruleset
 */

const UInteractionRuleset* UInteractableComponent::GetRuleset() const
{
	// Step 1: Check for instance ruleset override
	if(InstanceRuleset)
		return InstanceRuleset;

	// Step 2: Get project default from settings
	if (const UInteractionSettings* Settings = GetDefault<UInteractionSettings>())
		if (const UInteractionRuleset* ProjectDefault = Settings->GetDefaultRuleset())
			return ProjectDefault;

	//Step 3: hard-coded CDO fallback
	return GetDefault<UInteractionRuleset>();
}

/*
 * OnRep
 */

void UInteractableComponent::OnRep_InteractState()
{
	switch (InteractState)
	{
		case EInteractionState::Idle:
			// deliberately empty - finished and cancelled arrive via multicast rpcs rather than state change
			break;
		case EInteractionState::Interacting:
			OnBeginInteraction.Broadcast(CurrentInteractor);
			break;
	}
	UE_LOG(LogInteract, Log, TEXT("OnRep_InteractState called on %s, new state: %s"), *GetOwner()->GetName(), *UEnum::GetValueAsString(InteractState));
}

/*
 * Multicast RPCs
 */

void UInteractableComponent::Multicast_OnInteractionFinished_Implementation(UInteractorComponent* Interactor)
{
	OnFinishInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionFinished called on %s"), *GetOwner()->GetName());
	// TODO: trigger appropriate functions in handlers
}

void UInteractableComponent::Multicast_OnInteractionCancelled_Implementation(UInteractorComponent* Interactor)
{
	OnCancelInteraction.Broadcast(Interactor);

	UE_LOG(LogInteract, Log, TEXT("Multicast_OnInteractionCancelled called on %s"), *GetOwner()->GetName());
	// TODO: trigger appropriate functions in handlers
}