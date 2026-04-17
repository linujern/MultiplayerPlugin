#pragma once
#include "CoreMinimal.h"
#include "InteractionRegulationHandler.h"
#include "CooldownRegulationHandler.generated.h"

class UInteractorComponent;
class UCooldownLocalStateComponent;

/**
 * UCooldownRegulationComponent
 *
 * A regulation component that prevents re-interaction after a successful interaction.
 * Supports two independent cooldown modes:
 *
 * Global cooldown: Locks the interactable for ALL players for a set duration.
 * State is stored as a replicated property on this component, keeping all
 * clients in sync without any additional replication infrastructure.
 *
 * Per-player cooldown: Locks the interactable only for the player who triggered it.
 * State is managed via UCooldownLocalStateComponent, a satellite component spawned onto the player's pawn.
 * This component is entirely self-contained and invisible to the core interaction system.
 *
 * Either or both cooldown types can be used simultaneously.
 * Set a duration to 0 to disable that cooldown type.
 */

UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class EXTENSIBLEINTERACTIONSYSTEM_API UCooldownRegulationHandler : public UInteractionRegulationHandler
{
	GENERATED_BODY()

public:
	UCooldownRegulationHandler();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --------------------------------------------------------------------------------------------
	//	Configuration
	// --------------------------------------------------------------------------------------------

	// Duration in seconds all players are locked out after an interaction. 0 = disabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooldown")
	float GlobalCooldownSeconds = 5.f;

	// Duration in seconds the interacting player is locked out after an interaction. 0 = disabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooldown")
	float PerPlayerCooldownSeconds = 0.f;

	// --------------------------------------------------------------------------------------------
	//	UInteractionRegulationComponent Interface
	// --------------------------------------------------------------------------------------------
	
	virtual bool CanInteract_Global_Implementation(const UInteractableComponent* Interactable, UInteractorComponent* Interactor, FInteractionDeniedContext& DeniedContext) override;
	virtual bool CanInteract_Local_Implementation(const UInteractableComponent* Interactable, UInteractorComponent* Interactor, FInteractionDeniedContext& DeniedContext) override;
	virtual void OwnerInteractFinish_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor) override;

private:
	// --------------------------------------------------------------------------------------------
	//	Global Cooldown State
	//	Replicated so CanBeFocused_Global produces consistent results on all clients.
	// --------------------------------------------------------------------------------------------

	// World time at which the global cooldown expires. 0 means no active cooldown.
	UPROPERTY(ReplicatedUsing=OnRep_GlobalCooldownExpiry)
	float GlobalCooldownExpiry = 0.f;

	UFUNCTION()
	void OnRep_GlobalCooldownExpiry() const;

	bool IsGlobalCooldownActive() const;

	// --------------------------------------------------------------------------------------------
	//	Per-Player Cooldown
	//	State is managed entirely within the satellite UCooldownLocalStateComponent helper component on the player pawn.
	//	The UCooldownStateComponent is used only in this 
	// --------------------------------------------------------------------------------------------

	// Notifies all clients of the per-player lock. Each client checks if they own the
	// interactor and if so, creates or updates their UCooldownLocalStateComponent.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_NotifyPerPlayerLock(UInteractorComponent* Interactor, float Duration);

	// Finds or creates UCooldownLocalStateComponent on the interactor's pawn.
	static UCooldownLocalStateComponent* GetOrCreateLocalState(const UInteractorComponent* Interactor);
};

// ================================================================================================
//	UCOOLDOWNLOCALSTATECOMPONENT
//
//	Satellite component spawned onto player pawns by UCooldownRegulationComponent.
//	Tracks per-player cooldown expiry times keyed by the regulation component that set them.
//	Self-destructs when all tracked cooldowns have expired.
//
//	This component is defined here alongside its owner for encapsulation —
//	nothing outside UCooldownRegulationComponent should need to reference it directly.
// ================================================================================================

UCLASS()
class EXTENSIBLEINTERACTIONSYSTEM_API UCooldownLocalStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCooldownLocalStateComponent() { PrimaryComponentTick.bCanEverTick = false; }

	// Records a cooldown lock from a specific regulation component.
	// Schedules self-cleanup after the lock expires.
	void RecordLock(UCooldownRegulationHandler* Source, float ExpiryTime);

	// Returns true if this pawn is currently locked out by the given regulation component.
	bool IsLockedFor(const UCooldownRegulationHandler* Source) const;

private:
	// Keyed by the regulation component that imposed the lock — one pawn can be locked
	// by multiple interactables simultaneously, each tracked independently.
	// TMap is safe here — this component is never replicated.
	TMap<TObjectPtr<UCooldownRegulationHandler>, float> LockExpiries;

	FTimerHandle CleanupTimerHandle;

	// Removes expired entries. Destroys this component if none remain.
	void TryCleanup();
};