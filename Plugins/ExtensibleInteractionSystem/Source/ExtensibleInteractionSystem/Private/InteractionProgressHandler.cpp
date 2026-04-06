#include "InteractionProgressHandler.h"

void UInteractionProgressHandler::HandleProgressUpdate_Implementation(UInteractorComponent* Interactor, float ProgressPercent)
{
	// Default implementation does nothing
}

void UInteractionProgressHandler::HandleInteractionFinished_Implementation(UInteractorComponent* Interactor, float ProgressPercent)
{
	// Default implementation does nothing
}

void UInteractionProgressHandler::HandleInteractionCancelled_Implementation(UInteractorComponent* Interactor, float ProgressPercent)
{
	// Default implementation does nothing
}
