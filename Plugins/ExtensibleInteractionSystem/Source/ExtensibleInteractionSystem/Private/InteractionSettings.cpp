#include "InteractionSettings.h"
#include "InteractionRuleset.h"
#include "InteractionTracer.h"
#include "InteractionFocusHandler.h"
#include "InteractionProgressHandler.h"

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

TSubclassOf<UInteractionFocusHandler> UInteractionSettings::GetDefaultFocusHandlerClass() const
{
	return DefaultFocusHandlerClass.LoadSynchronous();
}

TSubclassOf<UInteractionProgressHandler> UInteractionSettings::GetDefaultProgressHandlerClass() const
{
	return DefaultProgressHandlerClass.LoadSynchronous();
}