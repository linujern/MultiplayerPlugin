#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"

class UInteractableComponent;
class UInteractionTracer;

UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractorComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Called from the owning actor (e.g. from the player on key-down). Internally handles all logic to start interaction.
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void StartInteracting();

	// Called from the owning actor (e.g. from the player on key-release). Internally handles all logic to stop interaction.
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void StopInteracting();
	
protected:	
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	TSubclassOf<UInteractionTracer> InteractionTracerClass;

private:
	UPROPERTY()
	TObjectPtr<UInteractionTracer> Tracer;

	// The InteractableComponent being focused, local only.
	UPROPERTY()
	TObjectPtr<UInteractableComponent> CurrentFocusedInteractable;

	// The InteractableComponent about to be interacted with, pending server confirmation
	UPROPERTY()
	TObjectPtr<UInteractableComponent> PendingInteractable;

	// The InteractableComponent being interacted with, as confirmed by the server
	UPROPERTY()
	TObjectPtr<UInteractableComponent> CurrentInteractingWith;


	float InteractionProgress = 0.0f;
	bool bIsDraining = false;
	
	/*
	 * Local Logic
	 */
	void TickTrace();
	void UpdateCurrentFocusedInteractable(UInteractableComponent* NewFocused);
	void AdvanceTimer(float DeltaTime);
	void DrainTimer(float DeltaTime);
	void SubmitInteraction();
	void UnbindDelegatesFrom(UInteractableComponent* Target);
	void ResetInteractionState();

	/*
	 * Delegate callbacks
	 */
	UFUNCTION()
	void OnLocalInteractBegun(UInteractorComponent* Interactor);

	UFUNCTION()
	void OnLocalInteractFinished(UInteractorComponent* Interactor);

	UFUNCTION()
	void OnLocalInteractCancelled(UInteractorComponent* Interactor);

	/*
	 * Server RPCs
	 */
	UFUNCTION(Server, Reliable)
	void Server_StartInteracting(UInteractableComponent* Target);

	UFUNCTION(Server, Reliable)
	void Server_RequestFinishInteraction();

	UFUNCTION(Server, Reliable)
	void Server_CancelInteraction();

	/*
	 * Client RPCs
	 */
	UFUNCTION(Client, Reliable)
	void Client_InteractionRejected();
	
};
