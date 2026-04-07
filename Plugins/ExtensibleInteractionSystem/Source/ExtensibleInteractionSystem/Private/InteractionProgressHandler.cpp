#include "InteractionProgressHandler.h"

void UInteractionProgressHandler::HandleProgressUpdate_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent)
{
	// Default implementation does nothing
}

void UInteractionProgressHandler::HandleInteractionStart_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent)
{
	// Default implementation does nothing
}

void UInteractionProgressHandler::HandleInteractionFinished_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent)
{
	// Default implementation does nothing
}

void UInteractionProgressHandler::HandleInteractionCancelled_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent)
{
	// Default implementation does nothing
}
