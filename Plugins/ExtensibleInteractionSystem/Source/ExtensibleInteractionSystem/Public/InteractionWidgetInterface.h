#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionWidgetInterface.generated.h"

UINTERFACE()
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionWidgetInterface : public UInterface
{
    GENERATED_BODY()
};

class IInteractionWidgetInterface
{
    GENERATED_BODY()
    
public: 
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void InteractionUpdate(float CompletionPercent);
};
