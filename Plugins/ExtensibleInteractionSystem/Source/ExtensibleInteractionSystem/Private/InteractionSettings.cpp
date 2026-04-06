#include "InteractionSettings.h"

UInteractionSettings::UInteractionSettings()
{
	
}

UInteractionRuleset* UInteractionSettings::GetDefaultRuleset() const
{
	return DefaultRuleset.TryLoad() ? Cast<UInteractionRuleset>(DefaultRuleset.ResolveObject()) : nullptr;
}
