#include "InteractionVisualHandler.h"

void UInteractionVisualHandler::HandleFocusGained_Implementation		 (UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract) {}
void UInteractionVisualHandler::HandleFocusLost_Implementation			 (UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract) {}

void UInteractionVisualHandler::HandleProgressUpdate_Implementation		 (UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract, float ProgressPercent) {}
void UInteractionVisualHandler::HandleInteractionStart_Implementation	 (UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract, float ProgressPercent) {}
void UInteractionVisualHandler::HandleInteractionFinished_Implementation (UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract, float ProgressPercent) {}
void UInteractionVisualHandler::HandleInteractionCancelled_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract, float ProgressPercent) {}

void UInteractionVisualHandler::HandleInteractionDenied_Implementation	 (UInteractableComponent* Interactable, UInteractorComponent* Interactor, EInteractionDeniedReason Reason) {}
