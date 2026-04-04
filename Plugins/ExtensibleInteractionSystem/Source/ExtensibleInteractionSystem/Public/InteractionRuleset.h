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
	float SecondsToTrigger = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float Cooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float TimerDeductionRating = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction Ruleset")
	float AllowedTriggers = -1.f;
	
};
