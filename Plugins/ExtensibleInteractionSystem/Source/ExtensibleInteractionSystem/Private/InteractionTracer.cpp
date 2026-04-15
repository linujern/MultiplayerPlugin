#include "InteractionTracer.h"

// Default implementation does no tracing and returns null. Override this in Blueprint or C++ to provide actual functionality.
UInteractableComponent* UInteractionTracer::FindBestInteractable_Implementation(AActor* Owner, UInteractorComponent* Interactor)
{
	return nullptr;
}