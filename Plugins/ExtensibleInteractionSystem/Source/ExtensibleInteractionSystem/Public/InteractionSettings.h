#pragma once
#include "CoreMinimal.h"
#include "InteractionFocusHandler.h"
#include "InteractionProgressHandler.h"
#include "InteractionRuleset.h"
#include "InteractionTracer.h"
#include "Engine/DeveloperSettings.h"
#include "InteractionSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, Blueprintable, meta=(DisplayName="Interaction Settings"))
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UInteractionSettings();
	
	// The default ruleset data instance. Must be a UInteractionRuleset DataAsset
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	FSoftObjectPath DefaultRuleset;
	// The default Tracer class to instantiate. Always a UInteractionTracer subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionTracer> DefaultTracerClass;
	// The default FocusHandler class to instantiate. Always a UInteractionFocusHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionFocusHandler> DefaultFocusHandlerClass;
	// The default ProgressHandler class to instantiate. Always a UInteractionProgressHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionProgressHandler> DefaultProgressHandlerClass;

	UFUNCTION()
	UInteractionRuleset* GetDefaultRuleset() const;
};
