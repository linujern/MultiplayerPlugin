#include "InteractorComponent.h"
#include "InteractableComponent.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}