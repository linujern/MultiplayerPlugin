#pragma once
#include "CoreMinimal.h"
#include "InteractorComponent.h"
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
    void InteractionUpdate(UInteractorComponent* Interactor, float CompletionPercent);
};
