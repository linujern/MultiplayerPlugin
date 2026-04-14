#pragma once
#include "CoreMinimal.h"
#include "InteractionTracer.generated.h"

class UInteractorComponent;
class UInteractableComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionTracer : public UObject
{
	GENERATED_BODY()
	
public:
	// This function should be overwritten in child classes to return a suitable InteractableComponent pointer.
	// Default implementation does no tracing and returns null. Override this in Blueprint (or the _Implementation in C++) to provide actual functionality.
	UFUNCTION(BlueprintNativeEvent, Category = "InteractionTracer")
	UInteractableComponent* FindBestInteractable(AActor* Owner, UInteractorComponent* Interactor);
};