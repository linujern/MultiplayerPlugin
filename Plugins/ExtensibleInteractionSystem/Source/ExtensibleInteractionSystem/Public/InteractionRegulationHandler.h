#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InteractionRegulationHandler.generated.h"

class UInteractableComponent;
class UInteractorComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionRegulationHandler : public UObject
{
	GENERATED_BODY()

public:

	// ============================================================
	// Gating Functions
	// ============================================================
	
	// Overwrite in blueprints or C++ to implement custom behaviour that regulates whether an interaction can occur.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Queries")
	bool CanInteract();

	// Overwrite in blueprints or C++ to implement custom behaviour that regulates focus can be applied.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Queries")
	bool CanBeFocused();

	// ============================================================
	// Local Only
	// ============================================================

	// Overwrite in blueprints or C++ to implement custom behaviour for this handler when focus is gained.
	// For example: start a cooldown before allowing interaction once focus is gained.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Notifies")
	void OwnerFocusGained(UInteractableComponent* Interactable, UInteractorComponent* Interactor);

	// Overwrite in blueprints or C++ to implement custom behaviour for this handler when focus is lost.
	// For example: disable focusing after focus is lost once using a bool that CanFocus checks.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Notifies")
	void OwnerFocusLost(UInteractableComponent* Interactable, UInteractorComponent* Interactor);

	// Overwrite in blueprints or C++ to implement custom behaviour for this handler when focus is lost.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Notifies")
	void OwnerInteractProgress(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float Progress);
	
	// ============================================================
	// Server-Side Only
	// ============================================================
	
	// Overwrite in blueprints or C++ to implement custom behaviour for this handler when an interaction is started.
	// For example: prevent other interactors from interacting while this interactor is interacting, by setting a boolean that CanInteract checks.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Notifies")
	void OwnerInteractStart(UInteractableComponent* Interactable, UInteractorComponent* Interactor);

	// Overwrite in blueprints or C++ to implement custom behaviour for this handler when an interaction is finished.
	// For example: decrement a RemainingInteractionsAllowed integer.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Notifies")
	void OwnerInteractFinish(UInteractableComponent* Interactable, UInteractorComponent* Interactor);

	// Overwrite in blueprints or C++ to implement custom behaviour for this handler when an interaction is cancelled.
	// For example: rollback something changed during interaction start.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler|Notifies")
	void OwnerInteractCancel(UInteractableComponent* Interactable, UInteractorComponent* Interactor);
};
