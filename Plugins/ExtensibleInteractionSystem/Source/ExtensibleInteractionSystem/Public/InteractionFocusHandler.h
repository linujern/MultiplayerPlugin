#pragma once
#include "CoreMinimal.h"
#include "InteractorComponent.h"
#include "UObject/Object.h"
#include "InteractionFocusHandler.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionFocusHandler : public UObject
{
	GENERATED_BODY()

public:
	// Overwrite in blueprints or C++ to implement custom focus behavior. By default, just logs focus changes to demonstrate functionality.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionFocusHandler")
	void HandleFocusGained(UInteractorComponent* Interactor);
	// Overwrite in blueprints or C++ to implement custom focus behavior. By default, just logs focus changes to demonstrate functionality.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionFocusHandler")
	void HandleFocusLost(UInteractorComponent* Interactor);
};