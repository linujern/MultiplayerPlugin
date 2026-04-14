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
    void FocusGained(UInteractableComponent* Interactable, UInteractorComponent* Interactor);
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void FocusLost(UInteractableComponent* Interactable, UInteractorComponent* Interactor);
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void InteractProgress(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float Progress);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void InteractStart(UInteractableComponent* Interactable, UInteractorComponent* Interactor);
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void InteractFinish(UInteractableComponent* Interactable, UInteractorComponent* Interactor);
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void InteractCancel(UInteractableComponent* Interactable, UInteractorComponent* Interactor);
};
