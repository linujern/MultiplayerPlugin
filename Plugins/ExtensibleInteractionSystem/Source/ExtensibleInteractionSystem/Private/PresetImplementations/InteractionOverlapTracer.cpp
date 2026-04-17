#include "PresetImplementations/InteractionOverlapTracer.h"
#include "InteractableComponent.h"
#include "InteractionDeniedContext.h"
#include "LogInteractionSystem.h"
#include "Camera/CameraComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UInteractableComponent* UInteractionOverlapTracer::FindBestInteractable_Implementation(AActor* Owner, UInteractorComponent* Interactor)
{
    if(!IsValid(Owner) || !IsValid(Interactor))
        return nullptr;


    const UWorld* WorldRef = Owner->GetWorld();
    
    if(!WorldRef)
        return nullptr;
    
    FVector Origin;
    FVector Forward;
    GetTraceOriginAndDirection(Owner, Origin, Forward);

    // Sphere overlap to gather all candidates in range
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Owner);
    
    WorldRef->OverlapMultiByChannel(
        Overlaps,
        Origin,
        FQuat::Identity,
        TraceChannel,
        FCollisionShape::MakeSphere(OverlapRadius),
        QueryParams
    );

    UInteractableComponent* BestCandidate = nullptr;
    float BestDot = -1.f;
    float MinDot = FMath::Cos(FMath::DegreesToRadians(MaxAngleDegrees));

    for(FOverlapResult& Overlap : Overlaps)
    {
        AActor* OverlapActor = Overlap.GetActor();
        if(!IsValid(OverlapActor))
            continue;

        UInteractableComponent* Interactable = OverlapActor->FindComponentByClass<UInteractableComponent>();
        if(!IsValid(Interactable))
            continue;

        // Respect the focusable flag
        FInteractionDeniedContext UnusedContext;
        if(!Interactable->IsFocusable(Interactor, UnusedContext))
            continue;

        // Direction from trace origin to the candidate's root location
        FVector ToTarget = (OverlapActor->GetActorLocation() - Origin).GetSafeNormal();
        float Dot = FVector::DotProduct(Forward, ToTarget);

        // Filter by cone angle
        if(Dot < MinDot) continue;

        // Among candidates within the cone, prefer the most forward-aligned one
        if(Dot > BestDot)
        {
            BestDot = Dot;
            BestCandidate = Interactable;
        }
    }

    return BestCandidate;
}

void UInteractionOverlapTracer::GetTraceOriginAndDirection(AActor* Owner, FVector& OutOrigin, FVector& OutForward) const
{
	// Establish defaults so every branch only needs to override what it handles
    OutOrigin = Owner->GetActorLocation();
    OutForward = Owner->GetActorForwardVector();

    const APawn* Pawn = Cast<APawn>(Owner);
    const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    const UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();

    switch (LocationSource)
    {
        case ETracerOriginSource::ActorLocation:
            OutOrigin = Owner->GetActorLocation();
            break;

        case ETracerOriginSource::CameraLocation:
            if(Camera)
                OutOrigin = Camera->GetComponentLocation();
            else
                UE_LOG(LogInteract, Warning, TEXT("UInteractionOverlapTracer: OriginSource is CameraLocation but no UCameraComponent was found on %s. "
												  "Falling back to actor location."), *Owner->GetName());
            break;

        case ETracerOriginSource::ControllerLocation:
            if(PC)
            {
                FRotator ViewRotation;
                PC->GetPlayerViewPoint(OutOrigin, ViewRotation);
            }
            else
                UE_LOG(LogInteract, Warning, TEXT("UInteractionOverlapTracer: OriginSource is ControllerLocation but no APlayerController was found on %s. "
												  "Falling back to actor location."),
                    *Owner->GetName());
            break;
    }

    switch (RotationSource)
    {
        case ETracerDirectionSource::ActorForward:
            OutForward = Owner->GetActorForwardVector();
            break;

        case ETracerDirectionSource::CameraForward:
            if(Camera)
                OutForward = Camera->GetForwardVector();
            else
                UE_LOG(LogInteract, Warning, TEXT("UInteractionOverlapTracer: DirectionSource is CameraForward but no UCameraComponent was found on %s. "
												  "Falling back to actor forward."),
                    *Owner->GetName());
            break;

        case ETracerDirectionSource::ControllerForward:
            if(PC)
            {
                FVector ViewLocation;
                FRotator ViewRotation;
                PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
                OutForward = ViewRotation.Vector();
            }
            else
                UE_LOG(LogInteract, Warning, TEXT("UInteractionOverlapTracer: DirectionSource is ControllerForward but no APlayerController was found on %s. "
												  "Falling back to actor forward."),
                    *Owner->GetName());
            break;
    }
}
