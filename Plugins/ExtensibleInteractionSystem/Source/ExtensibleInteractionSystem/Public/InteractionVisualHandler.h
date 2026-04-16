#pragma once
#include "CoreMinimal.h"
#include "InteractionDeniedReason.h"
#include "UObject/Object.h"
#include "InteractionVisualHandler.generated.h"

class UInteractableComponent;
class UInteractorComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionVisualHandler : public UObject
{
	GENERATED_BODY()

public:

	// ============================================================
	// Focus Events
	// ============================================================
	
	// Overwrite in blueprints or C++ to implement custom focus behavior. By default, just logs focus changes to demonstrate functionality.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionFocusHandler")
	void HandleFocusGained(UInteractableComponent* Interactable, UInteractorComponent* Interactor);
	// Overwrite in blueprints or C++ to implement custom focus behavior. By default, just logs focus changes to demonstrate functionality.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionFocusHandler")
	void HandleFocusLost(UInteractableComponent* Interactable, UInteractorComponent* Interactor);

	// ============================================================
	// Interaction Events
	// ============================================================
	
	// Overwrite in blueprints or C++ to implement custom behaviour when an interactor's interaction progress is updated.
	// ProgressPercent is a value between 0 and 1 representing the completion percentage of the interaction.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleProgressUpdate(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent);
	// Overwrite in blueprints or C++ to implement custom behaviour when an interactor's interaction progress is started.
	// ProgressPercent is a value between 0 and 1 representing the completion percentage of the interaction at time of start (typically 0).
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleInteractionStart(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent);
	// Overwrite in blueprints or C++ to implement custom behaviour when an interactor's interaction progress is finished.
	// ProgressPercent is a value between 0 and 1 representing the completion percentage of the interaction at time of finish (typically 1).
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleInteractionFinished(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent);
	// Overwrite in blueprints or C++ to implement custom behaviour when an interactor's interaction progress is cancelled.
	// ProgressPercent is a value between 0 and 1 representing the completion percentage of the interaction at the time of cancellation. 
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleInteractionCancelled(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent);

	// ============================================================
	// State change
	// ============================================================

	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleInteractionStateChanged(UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract);
	
	// ============================================================
	// Denied
	// ============================================================

	// TODO: implement HandleInteractionDenied from Interactable or Interactor
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleInteractionDenied(UInteractableComponent* Interactable, UInteractorComponent* Interactor, EInteractionDeniedReason Reason);
};