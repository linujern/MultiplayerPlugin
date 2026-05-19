#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"

class UInteractableComponent;
class UInteractionTracer;

// ============================================================
// Delegates
// ============================================================

// Delegates pass the target UInteractableComponent so listeners can apply object-specific logic without needing to query the component separately.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractCancelled, UInteractableComponent*, Interactable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractFinished,  UInteractableComponent*, Interactable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractRejected,  UInteractableComponent*, Interactable);

// ============================================================
// UINTERACTORCOMPONENT
// ============================================================

/**
 * UInteractorComponent
 *
 * Add to any pawn (such as the player) that should be able to interact with UInteractableComponents.
 * Handles tracing for interactable targets, focus management, hold-timer logic, and the full RPC flow
 * required for server-authoritative multiplayer interaction.
 *
 * Input entry points are StartInteracting() and StopInteracting(),
 * intended to be called from the owning actor's input bindings (e.g. hold E / release E)
 *
 * Tracing strategy is swappable via UInteractionTracer subclass, configurable per-pawn in the
 * editor or defaulting to the project setting in UInteractionSettings.
 *
 * The hold timer runs locally on the client for responsiveness.
 * The server validates completion when Server_RequestFinishInteraction is called,
 * and can cancel the interaction if the timer should have been drained in the meantime
 * (e.g. due to focus loss or a ruleset change).
 */
UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractorComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================
	// Public Interface
	// These functions should be wired to player (or other pawn) input - e.g. key-down calls to StartInteracting, key-up StopInteracting
	// ============================================================

	// Called from the owning actor (e.g. from the player on key-down). Internally handles all logic to start interaction.
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void StartInteracting();
	// Called from the owning actor (e.g. from the player on key-release). Internally handles all logic to stop interaction.
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void StopInteracting();

	// ============================================================
	// Interaction Cancellation
	// ============================================================

	// Called by InteractableComponent when interaction cannot continue.
	void RequestCancelInteraction(UInteractableComponent* Source);

protected:

	// ============================================================
	// Tracer
	// ============================================================
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", Instanced)
	TObjectPtr<UInteractionTracer> InteractionTracer;
	
private:

	// ============================================================
	// State
	// ============================================================

	// The currently focused InteractableComponent, updated each tick from the Tracer. Focus is purely local and never replicated.
	UPROPERTY()
	TObjectPtr<UInteractableComponent> CurrentFocusedInteractable;

	// The target sent by Server_StartInteracting, awaiting server confirmation.
	// Cleared once the server confirms via OnBeginInteraction broadcast (OnLocalInteractBegun).
	UPROPERTY()
	TObjectPtr<UInteractableComponent> PendingInteractable;

	// The confirmed active interaction target. Set when the server confirms via OnRep.
	// All timer logic and server RPCs should drive off this pointer, not PendingInteractable.
	UPROPERTY()
	TObjectPtr<UInteractableComponent> CurrentInteractingWith;

	// Progress in the unit interval [0-1]. Driven locally each tick by AdvanceTimer / DrainTimer.
	float InteractionProgress = 0.0f;
	// True while the player has released input but the progress hasn't drained to zero yet.
	bool bIsDraining = false;

	// Last progress value sent to the server for global progress handler updates.
	// Used to throttle Server_NotifyProgress calls.
	float LastSentProgress = 0.0f;
	
	// ============================================================
	// Local Logic
	// ============================================================

	// Calls the InteractionTracer's FindBestInteractable function every tick.
	void TickTrace();
	// Detects when the focused interactable has changed and calls the appropriate focus gained/lost functions.
	void UpdateCurrentFocusedInteractable(UInteractableComponent* NewFocused);
	
	void AdvanceTimer(float DeltaTime);
	void DrainTimer(float DeltaTime);
	void TimerUpdate(float AddedValue, float UpdateThreshold);
	// Calls Server_RequestFinishInteraction and resets local timer state. Called when the hold timer completes.
	void SubmitInteraction();
	// Unbinds callback delegates from the target interactable.
	void UnbindDelegatesFrom(UInteractableComponent* Target);
	// Resets Interactable pointers, bDraining and Progress floats.
	void ResetInteractionState();

	// ============================================================
	// Delegates
	// ============================================================

	// Broadcast from InteractorComponent::OnLocalInteractCancelled when an interaction is cancelled.
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnInteractCancelled OnInteractCancelled;

	// Broadcast from InteractorComponent::OnLocalInteractFinished when an interaction finishes.
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnInteractFinished OnInteractFinished;

	// Broadcast from InteractorComponent::Client_InteractionRejected when the server rejects the interaction request (e.g. invalid target, failed validation, etc.).
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnInteractRejected OnInteractRejected;
	
	// ============================================================
	// Delegate Callbacks
	// Bound to the interactable's delegates in StartInteracting.
	// Must be FUNCTION for dynamic multicast binding.
	// Each checks Interactor == this to ignore events from other interactors.
	// ============================================================
	
	UFUNCTION()
	void OnLocalInteractBegun(UInteractorComponent* Interactor);

	UFUNCTION()
	void OnLocalInteractFinished(UInteractorComponent* Interactor);

	UFUNCTION()
	void OnLocalInteractCancelled(UInteractorComponent* Interactor);

	// ============================================================
	// Server RPCs
	// All interaction authority lives on the server.
	// Clients initiate via these RPCs and receive results via OnRep or Client RPCs.
	// ============================================================

	// Asks the server to begin interaction with Target. Server validates and calls
	// Target->BeginInteraction(), which replicates state back to all clients.
	UFUNCTION(Server, Reliable)
	void Server_StartInteracting(UInteractableComponent* Target);

	// Informs the server that the local hold timer has completed.
	// Server validates and calls Target->FinishInteraction
	UFUNCTION(Server, Reliable)
	void Server_RequestFinishInteraction(float Progress);

	// Informs the server that the interaction was cancelled locally.
	// Also called automatically by DrainTimer once progress reaches zero.
	UFUNCTION(Server, Reliable)
	void Server_CancelInteraction(float Progress);

	// Informs the server that focus was gained on Target. Server relays to all clients via multicast.
	UFUNCTION(Server, Unreliable)
	void Server_NotifyFocusGained(UInteractableComponent* Target);

	// Informs the server that focus was lost on a Target. Server relays to all clients via multicast.
	UFUNCTION(Server, Unreliable)
	void Server_NotifyFocusLost(UInteractableComponent* Target);

	// Informs the server of interaction progress on a Target. Server relays to all clients via multicast.
	UFUNCTION(Server, Unreliable)
	void Server_NotifyProgress(UInteractableComponent* Target, const float Progress);

	// ============================================================
	// Client RPCs
	// ============================================================

	// Sent by the server if interaction was rejected (target invalid, not interactable, etc.).
	// Resets local state so the client doesn't get stuck in a pending interaction.
	UFUNCTION(Client, Reliable)
	void Client_InteractionRejected();
	
};
