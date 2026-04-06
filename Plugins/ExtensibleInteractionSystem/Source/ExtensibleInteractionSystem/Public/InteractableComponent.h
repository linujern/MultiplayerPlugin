#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

class UInteractorComponent;
class UInteractionRuleset;
class UInteractionFocusHandler;
class UInteractionProgressHandler;

UENUM(BlueprintType)
enum class EInteractionState : uint8
{
	Idle,
	Interacting
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginInteract,	UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFinishInteract,	UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCancelInteract,	UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusGained,		UInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusLost,		UInteractorComponent*, Interactor);


UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent) )
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractableComponent();
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*
	 * Serverside interaction calls
	 */
	// Called by UInteractionComponent. Server-side only
	void BeginInteraction(UInteractorComponent* Interactor);
	// Called by UInteractionComponent. Server-side only
	void FinishInteraction(UInteractorComponent* Interactor);
	// Called by UInteractionComponent. Server-side only
	void CancelInteraction(UInteractorComponent* Interactor);

	/*
	 * Local-side interaction calls
	 */
	// Called by UInteractionComponent. Local only
	void FocusGained(UInteractorComponent* Interactor);
	// Called by UInteractionComponent. Local only
	void FocusLost(UInteractorComponent* Interactor);

	void UpdateProgress(UInteractorComponent* Interactor, float ProgressPercent);

	/*
	 * Ruleset
	 */
	// Returns the ruleset (UInteractionRuleset) data used by this component instance.
	UFUNCTION(BlueprintPure, Category = "InteractionSystem")
	const UInteractionRuleset* GetRuleset() const;

	/*
	 * Public delegates
	 */
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

	// Can be toggled at runtime to enable/disable interaction with this component. Does not affect focus events. Overrides ruleset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanInteract = true;
	
protected:

	/*
	 * Ruleset
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractionRuleset> InstanceRuleset;

	/*
	 * Focus Handler
	 */
	// The class which dictates behaviour related to the focused state of this component. Selected class must inherit UInteractionFocusHandler.
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UInteractionFocusHandler> FocusHandlerClass;
	
	UPROPERTY()
	TObjectPtr<UInteractionFocusHandler> FocusHandler;

	/*
	 * Progress Handler
	 */
	// The class which dictates behaviour related to the completion progress interactors experience when interacting with this component. Selected class must inherit UInteractionProgressHandler.
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UInteractionProgressHandler> ProgressHandlerClass;

	UPROPERTY()
	TObjectPtr<UInteractionProgressHandler> ProgressHandler;

private:
	/*
	 * Replicated State
	 */
	UPROPERTY(ReplicatedUsing=OnRep_InteractState)
	EInteractionState InteractState = EInteractionState::Idle;

	UPROPERTY(Replicated)
	TObjectPtr<UInteractorComponent> CurrentInteractor; //TODO: make into array of interactors

	// OnRep
	UFUNCTION()
	void OnRep_InteractState();

	/*
	 * Multicast RPCs
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnInteractionFinished(UInteractorComponent* Interactor);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnInteractionCancelled(UInteractorComponent* Interactor);
	
};
