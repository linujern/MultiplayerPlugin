#include "InteractorComponent.h"
#include "InteractableComponent.h"
#include "InteractionRuleset.h"
#include "InteractionTracer.h"
#include "LogInteractionSystem.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

/*
 * Lifecycle
 */

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only locally controlled pawns need a tracer
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!OwningPawn || !OwningPawn->IsLocallyControlled())
		return;

	if (InteractionTracerClass)
		Tracer = NewObject<UInteractionTracer>(this, InteractionTracerClass);
	else
	{
		// TODO: fallback to the UInteractionSettings DefaultTracerClass
		UE_LOG(LogInteract, Log, TEXT("InteractorComponent on %s has no InteractionTracerClass set!"), *GetOwner()->GetName());
	}
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!OwningPawn || !OwningPawn->IsLocallyControlled())
		return;
	
	TickTrace();

	if(!IsValid(CurrentInteractingWith))
		return;
	
	if(bIsDraining)
		DrainTimer(DeltaTime);
	else
		AdvanceTimer(DeltaTime);
}

/*
 * Public Interface
 */

void UInteractorComponent::StartInteracting()
{
	if(!IsValid(CurrentFocusedInteractable))
		return;
	if(IsValid(CurrentInteractingWith) || IsValid(PendingInteractable))
		return;

	PendingInteractable = CurrentFocusedInteractable;

	PendingInteractable->OnBeginInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractBegun);
	PendingInteractable->OnFinishInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractFinished);
	PendingInteractable->OnCancelInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractCancelled);
	
	Server_StartInteracting(PendingInteractable);
	UE_LOG(LogInteract, Log, TEXT("StartInteracting called on %s, requesting interaction with: "), *GetOwner()->GetName());
}

void UInteractorComponent::StopInteracting()
{
	if(IsValid(PendingInteractable) && !IsValid(CurrentInteractingWith))
	{
		UnbindDelegatesFrom(PendingInteractable);
		PendingInteractable = nullptr;
		Server_CancelInteraction();
		return;
	}

	if(!IsValid(CurrentInteractingWith))
		return;

	const UInteractionRuleset* Ruleset = CurrentInteractingWith->GetRuleset();

	if(Ruleset && Ruleset->TimerDeductionRate > 0.f && InteractionProgress > 0.f)
		bIsDraining = true; // Draintimer will send the cancel RPC once progress hits zero.
	else
		Server_CancelInteraction();
}

/*
 * Local logic
 */

void UInteractorComponent::TickTrace()
{
	if(!Tracer)
		return;
	
	UInteractableComponent* Found = Tracer->FindBestInteractable(GetOwner());
	UpdateCurrentFocusedInteractable(Found);
}

void UInteractorComponent::UpdateCurrentFocusedInteractable(UInteractableComponent* NewFocused)
{
	if (CurrentFocusedInteractable == NewFocused)
		return;

	if (IsValid(CurrentFocusedInteractable))
		CurrentFocusedInteractable->FocusLost(this);

	CurrentFocusedInteractable = NewFocused;

	if(IsValid(CurrentFocusedInteractable))
		CurrentFocusedInteractable->FocusGained(this);
}


void UInteractorComponent::AdvanceTimer(float DeltaTime)
{
	const UInteractionRuleset* Ruleset = CurrentInteractingWith->GetRuleset();
	if(!IsValid(Ruleset) || Ruleset->SecondsToTrigger <= 0.f)
	{
		// submit immediately if no timer is required
		SubmitInteraction();
		return;
	}

	InteractionProgress += DeltaTime / Ruleset->SecondsToTrigger;
	InteractionProgress = FMath::Clamp(InteractionProgress, 0.f, 1.f);

	CurrentInteractingWith->UpdateProgress(this, InteractionProgress);

	if(InteractionProgress >= 1.f)
		SubmitInteraction();
}

void UInteractorComponent::DrainTimer(float DeltaTime)
{
	const UInteractionRuleset* Ruleset = CurrentInteractingWith->GetRuleset();
	if(!IsValid(Ruleset))
	{
		// no ruleset - just cancel
		bIsDraining = false;
		Server_CancelInteraction();
		return;
	}

	// TimerDeductionRate is a multiplier on the fill rate: 1.0 means it drains as quickly as it fills, 2.0 drains twice as fast
	InteractionProgress -= DeltaTime * Ruleset->TimerDeductionRate / Ruleset->SecondsToTrigger;
	InteractionProgress = FMath::Clamp(InteractionProgress, 0.f, 1.f);

	// TODO: call interactionupdate on interactablecomponent
	if(InteractionProgress <= 0.f)
	{
		bIsDraining = false;
		Server_CancelInteraction();
	}
}

void UInteractorComponent::SubmitInteraction()
{
	if(!IsValid(CurrentFocusedInteractable))
	{
		UE_LOG(LogInteract, Warning, TEXT("SubmitInteraction called on %s but CurrentFocusedInteractable is invalid!"), *GetOwner()->GetName());
		return;
	}

	UE_LOG(LogInteract, Log, TEXT("SubmitInteraction called on %s, interacting with: %s"), *GetOwner()->GetName(), *CurrentInteractingWith->GetName());
	
	InteractionProgress = 0.f;
	Server_RequestFinishInteraction();
}

void UInteractorComponent::UnbindDelegatesFrom(UInteractableComponent* Target)
{
	if(!IsValid(Target))
		return;

	Target->OnBeginInteraction.RemoveDynamic(this, &UInteractorComponent::OnLocalInteractBegun);
	Target->OnFinishInteraction.RemoveDynamic(this, &UInteractorComponent::OnLocalInteractFinished);
	Target->OnCancelInteraction.RemoveDynamic(this, &UInteractorComponent::OnLocalInteractCancelled);
}

void UInteractorComponent::ResetInteractionState()
{
	InteractionProgress = 0.f;
	bIsDraining = false;
	CurrentInteractingWith = nullptr;
	PendingInteractable = nullptr;
	UE_LOG(LogInteract, Log, TEXT("ResetInteractionState called on %s"), *GetOwner()->GetName());
}

/*
 * Delegate Callbacks
 */

void UInteractorComponent::OnLocalInteractBegun(UInteractorComponent* Interactor)
{
	CurrentInteractingWith = PendingInteractable;
	PendingInteractable = nullptr;
	InteractionProgress = 0.f;
	bIsDraining = false;

	UE_LOG(LogInteract, Log, TEXT("OnLocalInteractBegun called on %s, now interacting with: %s"), *GetOwner()->GetName(), *CurrentInteractingWith->GetName());
}

void UInteractorComponent::OnLocalInteractFinished(UInteractorComponent* Interactor)
{
	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();

	UE_LOG(LogInteract, Log, TEXT("OnLocalInteractFinished called on %s"), *GetOwner()->GetName());
}

void UInteractorComponent::OnLocalInteractCancelled(UInteractorComponent* Interactor)
{
	if(Interactor != this)
		return;

	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();

	UE_LOG(LogInteract, Log, TEXT("OnLocalInteractCancelled called on %s"), *GetOwner()->GetName());
}

/*
 * Server RPCs
 */

void UInteractorComponent::Server_StartInteracting_Implementation(UInteractableComponent* Target)
{
	if(!IsValid(Target))
	{
		Client_InteractionRejected();
		return;
	}

	const UInteractionRuleset* Ruleset = Target->GetRuleset();

	if(Ruleset && !Ruleset->bIsInteractable)
	{
		Client_InteractionRejected();
		return;
	}

	UE_LOG(LogInteract, Log, TEXT("Server_StartInteracting called on %s, requested interaction with: %s"), *GetOwner()->GetName(), *Target->GetName());

	// TODO: validation logic for whether this interaction should be allowed, based on the ruleset and current game state
	// TODO: allowedtriggers check

	Target->BeginInteraction(this);

	// Server must set this directly rather than relying on the OnLocalInteractBegun callback, as the server doesn't receive that callback via the multicast RPC
	CurrentInteractingWith = Target;
}

void UInteractorComponent::Server_RequestFinishInteraction_Implementation()
{
	if(!IsValid(CurrentInteractingWith))
		return;
	// interaction-rejected?

	CurrentInteractingWith->FinishInteraction(this);
	CurrentInteractingWith = nullptr;
}

void UInteractorComponent::Server_CancelInteraction_Implementation()
{
	if(!IsValid(CurrentInteractingWith))
		return;

	CurrentInteractingWith->CancelInteraction(this);
	CurrentInteractingWith = nullptr;
}

/*
 * Client RPCs
 */

void UInteractorComponent::Client_InteractionRejected_Implementation()
{
	UnbindDelegatesFrom(PendingInteractable);
	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();

	UE_LOG(LogInteract, Log, TEXT("Client_InteractionRejected called on %s"), *GetOwner()->GetName());
}
