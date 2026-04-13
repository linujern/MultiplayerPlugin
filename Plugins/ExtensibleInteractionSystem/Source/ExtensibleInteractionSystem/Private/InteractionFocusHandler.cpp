#include "InteractionFocusHandler.h"
#include "InteractorComponent.h"
#include "InteractableComponent.h"
#include "LogInteractionSystem.h"

// Overwrite in Blueprints to implement custom focus behavior. By default, they just log focus changes to demonstrate functionality.
void UInteractionFocusHandler::HandleFocusGained_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{
	UE_LOG(LogInteract, Log, TEXT("Focus Gained: %s is now focusing on %s"), *Interactor->GetOwner()->GetName(), *GetOuter()->GetName());
}

// Overwrite in Blueprints or  to implement custom focus behavior. By default, they just log focus changes to demonstrate functionality.
void UInteractionFocusHandler::HandleFocusLost_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{
	UE_LOG(LogInteract, Log, TEXT("Focus Lost: %s is no longer focusing on %s"), *Interactor->GetOwner()->GetName(), *GetOuter()->GetName());
}