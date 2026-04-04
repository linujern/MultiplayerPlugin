#include "InteractableComponent.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}