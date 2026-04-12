#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InteractionRuleset.generated.h"

UCLASS(BlueprintType, ClassGroup=(Interaction))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionRuleset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	bool bIsInteractable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	bool bCanBeFocused = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float SecondsToTrigger = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float TimerDeductionRate = 1.f;

	// Minimum progress change required to trigger a progress update event.
	// Prevents flooding with small updates when progress is changing very frequently (e.g. every tick).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float GlobalProgressUpdateThreshold = .05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float CooldownSeconds = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	int32 AllowedTriggers = -1; // negative values for infinite
	
};
