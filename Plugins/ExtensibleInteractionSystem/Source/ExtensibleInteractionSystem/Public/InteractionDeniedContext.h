#pragma once
#include "GameplayTagContainer.h"
#include "CoreMinimal.h"
#include "InteractionDeniedContext.generated.h"

USTRUCT(BlueprintType)
struct EXTENSIBLEINTERACTIONSYSTEM_API FInteractionDeniedContext
{
	GENERATED_BODY()
	
	// Hierarchical reason — matches against Interaction.Denied.* hierarchy.
	// The plugin defines core tags. Projects define their own freely.
	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FGameplayTag Reason;

	// Localizable display message for UI. Set by the denying component.
	// "You need a key", "On cooldown", "Door is sealed" etc.
	// Left empty if the display handler derives text from the tag itself.
	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FText DisplayMessage;

	// The component that denied the interaction.
	// Display handlers can cast this and query additional context if needed.
	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	TObjectPtr<UActorComponent> DenyingComponent;

	// Convenience constructor for the common case
	FInteractionDeniedContext() = default;
	FInteractionDeniedContext(FGameplayTag InReason, FText InMessage, UActorComponent* InSource)
	: Reason(InReason), DisplayMessage(InMessage), DenyingComponent(InSource) {}

	bool IsValid() const { return Reason.IsValid(); }
};