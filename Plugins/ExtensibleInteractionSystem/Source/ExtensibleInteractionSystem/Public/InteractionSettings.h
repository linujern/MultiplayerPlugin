#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InteractionSettings.generated.h"

class UInteractionVisualHandler;
class UInteractionRegulationHandler;
class UInteractionRuleset;
class UInteractionTracer;

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

	// TODO: make default VisualHandlers into arrays to support several selections at once.
	// The default Local VisualHandler class to instantiate. Controls visuals seen by the local player on focus and interaction events.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionVisualHandler> DefaultLocalVisualHandlerClass;
	// The default Global FocusHandler class to instantiate. Controls visuals seen by all non-local players see on local player focus and interaction events.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionVisualHandler> DefaultGlobalVisualHandlerClass;
	// The default RegulationHandler class to instantiate. Controls whether interactions and focus can occur.
	// Always a UInteractionRegulationHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionRegulationHandler> DefaultRegulationHandlerClass;
	
	UFUNCTION()
	UInteractionRuleset* GetDefaultRuleset() const;
	UFUNCTION()
	TSubclassOf<UInteractionTracer> GetDefaultTracerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionVisualHandler> GetDefaultLocalVisualHandlerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionVisualHandler> GetDefaultGlobalVisualHandlerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionRegulationHandler> GetDefaultRegulationHandlerClass() const;
};
