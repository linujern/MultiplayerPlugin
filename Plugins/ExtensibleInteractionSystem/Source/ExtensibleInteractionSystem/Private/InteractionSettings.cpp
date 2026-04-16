#include "InteractionSettings.h"
#include "InteractionRuleset.h"
#include "InteractionTracer.h"
#include "InteractionVisualHandler.h"
#include "InteractionRegulationHandler.h"

UInteractionSettings::UInteractionSettings() {}

UInteractionRuleset* UInteractionSettings::GetDefaultRuleset() const
{
	return Cast<UInteractionRuleset>(DefaultRuleset.TryLoad());
}

TSubclassOf<UInteractionTracer> UInteractionSettings::GetDefaultTracerClass() const
{
	return DefaultTracerClass.LoadSynchronous();
}

TSubclassOf<UInteractionVisualHandler> UInteractionSettings::GetDefaultLocalVisualHandlerClass() const
{
	return DefaultLocalVisualHandlerClass.LoadSynchronous();
}

TSubclassOf<UInteractionVisualHandler> UInteractionSettings::GetDefaultGlobalVisualHandlerClass() const
{
	return DefaultGlobalVisualHandlerClass.LoadSynchronous();
}

TSubclassOf<UInteractionRegulationHandler> UInteractionSettings::GetDefaultRegulationHandlerClass() const
{
	return DefaultRegulationHandlerClass.LoadSynchronous();
}