#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent) )
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractableComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
protected:
	virtual void BeginPlay() override;

};
