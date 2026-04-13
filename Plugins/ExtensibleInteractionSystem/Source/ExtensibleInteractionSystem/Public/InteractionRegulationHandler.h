#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InteractionRegulationHandler.generated.h"

UCLASS()
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionRegulationHandler : public UObject
{
	GENERATED_BODY()

public:
	// Overwrite in blueprints or C++ to implement custom behaviour that regulates whether an interaction can occur.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler")
	bool CanInteract();

	// Overwrite in blueprints or C++ to implement custom behaviour that regulates focus can be applied.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionRegulationHandler")
	bool CanBeFocused();
};
