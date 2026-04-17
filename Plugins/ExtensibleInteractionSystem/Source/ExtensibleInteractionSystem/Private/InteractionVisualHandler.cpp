#include "InteractionVisualHandler.h"

#include "InteractionDeniedContext.h"

void UInteractionVisualHandler::HandleFocusGained_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor) {}
void UInteractionVisualHandler::HandleFocusLost_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor) {}

void UInteractionVisualHandler::HandleProgressUpdate_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent) {}
void UInteractionVisualHandler::HandleInteractionStart_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent) {}
void UInteractionVisualHandler::HandleInteractionFinished_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent) {}
void UInteractionVisualHandler::HandleInteractionCancelled_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor, float ProgressPercent) {}

void UInteractionVisualHandler::HandleInteractionStateChanged_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor, bool bCanBeFocused, bool bCanInteract, const FInteractionDeniedContext& Context) {}

void UInteractionVisualHandler::HandleInteractionDenied_Implementation
(UInteractableComponent* Interactable, UInteractorComponent* Interactor, const FInteractionDeniedContext& Context) {}
