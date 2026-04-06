#pragma once
#include "CoreMinimal.h"
#include "InteractableComponent.h"
#include "UObject/Object.h"
#include "InteractionProgressHandler.generated.h"

class UInteractorComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionProgressHandler : public UObject
{
	GENERATED_BODY()

public:
	// Overwrite in blueprints or C++ to implement custom behaviour when an interactor's interaction progress is updated.
	// ProgressPercent is a value between 0 and 1 representing the completion percentage of the interaction.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleProgressUpdate(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent);
	// Overwrite in blueprints or C++ to implement custom behaviour when an interactor's interaction progress is finished.
	// ProgressPercent is a value between 0 and 1 representing the completion percentage of the interaction at time of finish (typically 1 at the finish).
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleInteractionFinished(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent);
	// Overwrite in blueprints or C++ to implement custom behaviour when an interactor's interaction progress is updated.
	// ProgressPercent is a value between 0 and 1 representing the completion percentage of the interaction at the time of cancellation. 
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionProgressHandler")
	void HandleInteractionCancelled(UInteractableComponent* Interactable, UInteractorComponent * Interactor, float ProgressPercent);
};