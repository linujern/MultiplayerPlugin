#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InteractionRuleset.generated.h"

UCLASS(BlueprintType, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionRuleset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Overrides RegulationHandler if true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	bool bDisableInteraction = false;

	// Overrides RegulationHandler if true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	bool bDisableFocus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float SecondsToTrigger = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float TimerDeductionRate = 1.f;

	// If True, interactions get cut off midway in the case where the interactable object becomes non-interactable during the interaction process.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	bool bCancelOnInteractabilityLost = true;

	// Minimum progress change required to trigger a progress update event.
	// Prevents flooding with small updates when progress is changing very frequently (e.g. every tick).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float GlobalProgressUpdateThreshold = .05f;
};
