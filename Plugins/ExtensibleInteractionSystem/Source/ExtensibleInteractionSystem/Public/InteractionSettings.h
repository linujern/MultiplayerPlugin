#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InteractionSettings.generated.h"

class UInteractionRegulationHandler;
class UInteractionRuleset;
class UInteractionFocusHandler;
class UInteractionProgressHandler;
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
	// The default Local FocusHandler class to instantiate. Controls what the local player sees on focus.
	// Always a UInteractionFocusHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionFocusHandler> DefaultLocalFocusHandlerClass;
	// The default Global FocusHandler class to instantiate. Controls what all non-local players see on local player focus.
	// Always a UInteractionFocusHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionFocusHandler> DefaultGlobalFocusHandlerClass;
	// The default Local ProgressHandler class to instantiate. Controls what the local player sees on interaction progress.
	// Always a UInteractionProgressHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionProgressHandler> DefaultLocalProgressHandlerClass;
	// The default Global ProgressHandler class to instantiate. Controls what all non-local players see while the local player interacts.
	// Always a UInteractionProgressHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionProgressHandler> DefaultGlobalProgressHandlerClass;
	// The default RegulationHandler class to instantiate. Controls whether interactions and focus can occur.
	// Always a UInteractionRegulationHandler subclass.
	UPROPERTY(Config, EditAnywhere, Category="Interaction")
	TSoftClassPtr<UInteractionRegulationHandler> DefaultRegulationHandlerClass;
	
	UFUNCTION()
	UInteractionRuleset* GetDefaultRuleset() const;
	UFUNCTION()
	TSubclassOf<UInteractionTracer> GetDefaultTracerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionFocusHandler> GetDefaultLocalFocusHandlerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionFocusHandler> GetDefaultGlobalFocusHandlerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionProgressHandler> GetDefaultLocalProgressHandlerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionProgressHandler> GetDefaultGlobalProgressHandlerClass() const;
	UFUNCTION()
	TSubclassOf<UInteractionRegulationHandler> GetDefaultRegulationHandlerClass() const;
};
