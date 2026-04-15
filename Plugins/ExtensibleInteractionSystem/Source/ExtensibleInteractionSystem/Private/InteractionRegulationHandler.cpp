#include "InteractionRegulationHandler.h"

UInteractionRegulationHandler::UInteractionRegulationHandler()
{
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bCanEverTick = false;
}

bool UInteractionRegulationHandler::CanBeFocused_Local_Implementation	(const UInteractableComponent* Interactable, UInteractorComponent* Interactor) { return true; }
bool UInteractionRegulationHandler::CanInteract_Local_Implementation	(const UInteractableComponent* Interactable, UInteractorComponent* Interactor) { return true; }
bool UInteractionRegulationHandler::CanBeFocused_Global_Implementation	(const UInteractableComponent* Interactable, UInteractorComponent* Interactor) { return true; }
bool UInteractionRegulationHandler::CanInteract_Global_Implementation	(const UInteractableComponent* Interactable, UInteractorComponent* Interactor) { return true; }

void UInteractionRegulationHandler::OwnerFocusGained_Implementation		(UInteractableComponent* Interactable, UInteractorComponent* Interactor) {}
void UInteractionRegulationHandler::OwnerFocusLost_Implementation		(UInteractableComponent* Interactable, UInteractorComponent* Interactor) {}
void UInteractionRegulationHandler::OwnerInteractProgress_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float Progress) {}
void UInteractionRegulationHandler::OwnerInteractStart_Implementation	(UInteractableComponent* Interactable, UInteractorComponent* Interactor) {}
void UInteractionRegulationHandler::OwnerInteractFinish_Implementation	(UInteractableComponent* Interactable, UInteractorComponent* Interactor) {}
void UInteractionRegulationHandler::OwnerInteractCancel_Implementation	(UInteractableComponent* Interactable, UInteractorComponent* Interactor) {}