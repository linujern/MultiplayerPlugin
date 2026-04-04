#pragma once
#include "CoreMinimal.h"
#include "InteractableComponent.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractorComponent();

protected:
	virtual void BeginPlay() override;
	
public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
