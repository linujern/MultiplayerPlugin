#pragma once
UENUM(BlueprintType)
enum class EInteractionDeniedReason : uint8
{
	Unknown,
	InstanceDisabled,	// bDisableInteraction flag
	RulesetDisabled,	// Ruleset flag
	RegulationHandler	// denied by regulation-component, reason is handler-specific
};