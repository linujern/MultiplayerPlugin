#pragma once
#include "CoreMinimal.h"
#include "InteractableComponent.h"
#include "InteractionTracer.generated.h"

UCLASS(Abstract, Blueprintable, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionTracer : public UObject
{
	GENERATED_BODY()
	
public:
	virtual UInteractableComponent* FindBestInteractable(AActor* Owner) { return nullptr; }
};