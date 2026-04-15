#pragma once
#include "CoreMinimal.h"
#include "InteractionTracer.h"
#include "InteractionOverlapTracer.generated.h"

class UInteractorComponent;

UENUM(BlueprintType)
enum class ETracerDirectionSource : uint8
{
	ActorForward,       // Uses the owner actor's forward vector
	CameraForward,      // Uses the player camera's forward vector
	ControllerForward   // Uses the controller's view rotation as a forward vector
};

UENUM(BlueprintType)
enum class ETracerOriginSource : uint8
{
	ActorLocation,      // Uses the owner actor's root location
	CameraLocation,     // Uses the player camera's world position
	ControllerLocation  // Uses the controller's pawn view location
};

/**
 * UInteractionOverlapTracer
 *
 * Finds the best interactable by performing a sphere overlap around the owner,
 * then selecting the candidate most aligned with the owner's forward vector,
 * within a configurable angle and range.
 *
 * More forgiving than a line trace — suitable for third-person or gamepad play
 * where pixel-perfect aiming is not expected.
 */

UCLASS(EditInlineNew)
class EXTENSIBLEINTERACTIONSYSTEM_API UInteractionOverlapTracer : public UInteractionTracer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tracer")
	ETracerDirectionSource RotationSource = ETracerDirectionSource::ActorForward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tracer")
	ETracerOriginSource LocationSource = ETracerOriginSource::ActorLocation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tracer")
	float OverlapRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tracer")
	float MaxAngleDegrees = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tracer")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	virtual UInteractableComponent* FindBestInteractable_Implementation(AActor* Owner, UInteractorComponent* Interactor) override;

private:
	void GetTraceOriginAndDirection(AActor* Owner, FVector& OutOrigin, FVector& OutForward) const;
};
