#include "InteractionSettings.h"
#include "InteractionRuleset.h"
#include "InteractionTracer.h"
#include "InteractionFocusHandler.h"
#include "InteractionProgressHandler.h"
#include "InteractionRegulationHandler.h"

UInteractionSettings::UInteractionSettings()
{
	
}

UInteractionRuleset* UInteractionSettings::GetDefaultRuleset() const
{
	return Cast<UInteractionRuleset>(DefaultRuleset.TryLoad());
}

TSubclassOf<UInteractionTracer> UInteractionSettings::GetDefaultTracerClass() const
{
	return DefaultTracerClass.LoadSynchronous();
}

TSubclassOf<UInteractionFocusHandler> UInteractionSettings::GetDefaultLocalFocusHandlerClass() const
{
	return DefaultLocalFocusHandlerClass.LoadSynchronous();
}

TSubclassOf<UInteractionFocusHandler> UInteractionSettings::GetDefaultGlobalFocusHandlerClass() const
{
	return DefaultGlobalFocusHandlerClass.LoadSynchronous();
}

TSubclassOf<UInteractionProgressHandler> UInteractionSettings::GetDefaultLocalProgressHandlerClass() const
{
	return DefaultLocalProgressHandlerClass.LoadSynchronous();
}

TSubclassOf<UInteractionProgressHandler> UInteractionSettings::GetDefaultGlobalProgressHandlerClass() const
{
	return DefaultGlobalProgressHandlerClass.LoadSynchronous();
}

TSubclassOf<UInteractionRegulationHandler> UInteractionSettings::GetDefaultRegulationHandlerClass() const
{
	return DefaultRegulationHandlerClass.LoadSynchronous();
}