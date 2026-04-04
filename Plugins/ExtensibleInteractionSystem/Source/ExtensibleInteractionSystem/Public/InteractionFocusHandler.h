#pragma once
#include "CoreMinimal.h"
#include "InteractorComponent.h"
#include "UObject/Object.h"
#include "InteractionFocusHandler.generated.h"

UCLASS(Abstract, Blueprintable, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionFocusHandler : public UObject
{
	GENERATED_BODY()

public:
	virtual void FocusGained(UInteractorComponent* Interactor) {};
	virtual void FocusLost(UInteractorComponent* Interactor) {};
};
