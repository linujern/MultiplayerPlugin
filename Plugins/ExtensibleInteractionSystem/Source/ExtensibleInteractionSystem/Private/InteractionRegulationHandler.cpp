#include "InteractionRegulationHandler.h"
#include "InteractorComponent.h"

bool UInteractionRegulationHandler::CanBeFocused_Implementation()
{
	return true;
}

bool UInteractionRegulationHandler::CanInteract_Implementation()
{
	return true;
}

void UInteractionRegulationHandler::OwnerFocusGained_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{}

void UInteractionRegulationHandler::OwnerFocusLost_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{}

void UInteractionRegulationHandler::OwnerInteractProgress_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float Progress)
{}

void UInteractionRegulationHandler::OwnerInteractStart_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{}

void UInteractionRegulationHandler::OwnerInteractFinish_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{}

void UInteractionRegulationHandler::OwnerInteractCancel_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{}