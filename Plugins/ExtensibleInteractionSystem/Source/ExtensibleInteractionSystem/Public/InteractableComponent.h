#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

class UInteractorComponent;
class UInteractionRuleset;
class UInteractionFocusHandler;
class UInteractionProgressHandler;
class UInteractionRegulationHandler;

// ============================================================
// ENUMS
// ============================================================

// Represents the persistent replicated state of this interactable.
// Note that finished and cancelled are not states but transient events delivered via NetMulticastRPCs to ensure they are
// received by clients even if the interaction state changes back to idle immediately after.
UENUM(BlueprintType)
enum class EInteractionState : uint8
{
	Idle,
	Interacting
};

// ============================================================
// Delegates
// ============================================================

// All delegates pass the instigating UInteractorComponent so listeners can apply player-specific logic without needing to query the component separately.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginInteract,	UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFinishInteract,	UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCancelInteract,	UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusGained,		UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusLost,		UInteractorComponent*, Interactor);


// ============================================================
// UINTERACTABLECOMPONENT
// ============================================================

/**
 * UInteractableComponent
 *
 * Add to any actor that should be interactable. Manages interaction state replication,
 * broadcasts delegates for gameplay logic, and dispatches visual events to Focus and Progress handlers.
 *
 * Interaction is initiated and terminated by UInteratorComponent via server-side entry points.
 * Focus events are local-only and never replicated.
 *
 * Visual behaviour is configured via handler class arrays:
 *   - Local handlers run only on the focusing/interacting client's side.
 *   - Global handlers run on all clients via NetMulticast.
 */

UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent) )
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractableComponent();
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ============================================================
	// Server-Side Entry Points
	// Called by UInteractorComponent via its ServerRPCs. Must execute on the server.
	// ============================================================
	
	// Called by UInteractionComponent::Server_StartInteracting. Server-side only
	void BeginInteraction(UInteractorComponent* Interactor, float ProgressPercent);
	// Called by UInteractorComponent::Server_RequestFinishInteraction. Server-side only
	void FinishInteraction(UInteractorComponent* Interactor, float ProgressPercent);
	// Called by UInteractionComponent::Server_CancelInteraction. Server-side only
	void CancelInteraction(UInteractorComponent* Interactor, float ProgressPercent);

	// ============================================================
	// Local-Only Entry Points
	// Called by UInteractorComponent on the local client. Never replicated.
	// ============================================================
	
	// Called by UInteractionComponent when focus initiates. Local only
	void FocusGained(UInteractorComponent* Interactor);
	// Called by UInteractionComponent when focus ends. Local only
	void FocusLost(UInteractorComponent* Interactor);

	void InteractBegin(UInteractorComponent* Interactor, float ProgressPercent);
	void InteractCancel(UInteractorComponent* Interactor, float ProgressPercent);
	void InteractFinish(UInteractorComponent* Interactor, float ProgressPercent);

	// Called by UInteractionComponent each tick while interacting. Local only
	// ProgressPercent is in the unit interval [0-1].
	void UpdateProgress(UInteractorComponent* Interactor, float ProgressPercent);

	// ============================================================
	// Ruleset
	// ============================================================
	
	// Returns the active ruleset for this component resolving in priority order:
	// 1. InstanceRuleset (per-object override)
	// 2. UInteractionSettings::DefaultRuleset (project-wide default)
	// 3. UInteractionRuleset CDO (hardcoded fallback - always valid)
	UFUNCTION(BlueprintPure, Category = "InteractionSystem")
	const UInteractionRuleset* GetRuleset() const;

	// Returns whether this component can be focused, checking 1: Local override, 2: Ruleset Override, 3: RegulationHandler
	UFUNCTION(BlueprintCallable, Category = "InteractionSystem")
	bool IsFocusable() const;

	UFUNCTION(BlueprintCallable, Category = "InteractionSystem")
	bool IsInteractable() const;

	// ============================================================
	// Delegates
	// Broadcast on all clients via OnRep (Begin), or NetMulticast (Finish, Cancel).
	// Focus delegates broadcast locally only
	// ============================================================
	
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnBeginInteract OnBeginInteraction;
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnFinishInteract OnFinishInteraction;
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnCancelInteract OnCancelInteraction;
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnFocusGained OnFocusGained;
	UPROPERTY(BlueprintAssignable, Category = "InteractionSystem")
	FOnFocusLost OnFocusLost;
	
protected:

	// ============================================================
	// Ruleset
	// ============================================================

	// Overrides the project default ruleset for this specific component instance. Leave empty to fall through to project default or CDO fallback.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractionRuleset> InstanceRuleset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	bool bFallBackToDefaultHandlers = true;
	
	// Toggles interactability at runtime regardless of ruleset. Overrides RegulationHandler if true. Does not affect focus events.
	// Use this to gate interaction on a per-instance basis via gameplay logic (e.g. a puzzle prerequisite).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bDisableInteraction = false;

	// Toggles focusability at runtime regardless of ruleset. Overrides RegulationHandler if true.
	// Use this to gate interaction on a per-instance basis via gameplay logic (e.g. a puzzle prerequisite).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bDisableFocus = false;

	// ============================================================
	// Focus Handlers
	//
	// FocusHandlers Receive callback when this interactable gains or loses focus.
	// Local handlers fire only on the focus client (e.g. stencil outline).
	// Global handlers fire on all clients via NetMulticast (e.g. world-space widget appearing).
	//
	// If no classes are set, the project default from UInteractionSettings is used.
	// ============================================================

	// The classes which dictate behaviour related to the focused state of this component, as seen by the local player.
	// (such as highlighting that the object can be interacted with).
	UPROPERTY(EditAnywhere, Category = "Interaction", Instanced)
	TArray<TObjectPtr<UInteractionFocusHandler>> LocalFocusHandlers;

	// The classes which dictate behaviour related to the focused state of this component, as seen by *ALL* players.
	// (such as a world-space widget appearing above the object when focused).
	UPROPERTY(EditAnywhere, Category = "Interaction", Instanced)
	TArray<TObjectPtr<UInteractionFocusHandler>> GlobalFocusHandlers;

	// ============================================================
	// Progress Handlers
	//
	// ProgressHandlers Receive callbacks as interaction progresses.
	// Local handlers fire only on the focus client (e.g. progress bar widget).
	// Global handlers fire on all clients via NetMulticast (e.g. shared world-space progress).
	//
	// If no classes are set, the project default from UInteractionSettings is used.
	// ============================================================
	
	// The classes which dictate behaviour related to the completion progress the interactor experiences when interacting with this component.
	// Handle visual events related to the progression of interaction for the interacting player only
	// (such as a progress bar widget filling towards 100% as the player holds the interact key).
	UPROPERTY(EditAnywhere, Category = "Interaction", Instanced)
	TArray<TObjectPtr<UInteractionProgressHandler>> LocalProgressHandlers;

	// The classes which dictate behaviour related to the completion progress the interactor experiences when interacting with this component.
	// Handle visual events related to the progression of interaction that all players can see
	// (such as a progress bar widget filling towards 100% as one player holds the interact key).
	UPROPERTY(EditAnywhere, Category = "Interaction", Instanced)
	TArray<TObjectPtr<UInteractionProgressHandler>> GlobalProgressHandlers;

	// ============================================================
	// RegulationHandler
	//
	//
	// ============================================================
	
	UPROPERTY(EditAnywhere, Category = "Interaction", Instanced)
	TObjectPtr<UInteractionRegulationHandler> RegulationHandler;

private:
	// ============================================================
	// Replicated State
	// ============================================================

	// EIntgeractionState replicates the persistent state of this interactable.
	UPROPERTY(Replicated)
	EInteractionState InteractState = EInteractionState::Idle;

	// Replicated so clients receiving OnRep_InteractState can identify the interactor.
	// Must be set before InteractState to avoid a null interactor on the broadcast.
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UInteractorComponent>> CurrentInteractors; 

	// ============================================================
	// Helpers
	// ============================================================

	UFUNCTION()
	static bool IsLocallyInstigated(const UInteractorComponent* Interactor);
	
	// ============================================================
	// Multicast RPCs
	// ============================================================

	// Called by BeginInteraction
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnInteractionBegun(UInteractorComponent* Interactor, float ProgressPercent);
	
	// Called by FinishInteraction.
	// Must be Reliable because it represents authoritative state changes that must not be missed.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnInteractionFinished(UInteractorComponent* Interactor, float ProgressPercent);

	// Called by CancelInteraction.
	// Must be Reliable because it represents authoritative state changes that must not be missed.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnInteractionCancelled(UInteractorComponent* Interactor, float ProgressPercent);
	
public:
	// Called by UInteractorComponent::Server_NotifyFocusGained
	// Unreliable since focus can change frequently and missed packages are harmless
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_FocusGained(UInteractorComponent* Interactor);

	// Called by UInteractorComponent::Server_NotifyFocusLost
	// Unreliable since focus can change frequently and missed packages are harmless
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_FocusLost(UInteractorComponent* Interactor);
	
	// Called by UInteractorComponent::Server_NotifyProgress
	// Unreliable since progress will update frequently and missed packages are harmless
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_UpdateProgress(UInteractorComponent* Interactor, const float ProgressPercent);
	
};
