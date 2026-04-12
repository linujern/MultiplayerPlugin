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

// ============================================================
// LIFECYCLE
// ============================================================

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Tracing is only needed on the locally controlled pawn. Simulated proxies and
	// the server do not run traces - interaction authority is handled via RPCs.
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

	// Only locally controlled pawns drive tracing and timers.
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

// ============================================================
// PUBLIC INTERFACE
// ============================================================

void UInteractorComponent::StartInteracting()
{
	if(!IsValid(CurrentFocusedInteractable))
		return;
	if(IsValid(CurrentInteractingWith) || IsValid(PendingInteractable))
		return;

	PendingInteractable = CurrentFocusedInteractable;

	UE_LOG(LogInteract, Log, TEXT("StartInteracting called on %s, requesting interaction with: %s"), *GetOwner()->GetName(), *PendingInteractable->GetName());

	// Bind before the server call in preparation for OnRep firing which broadcasts OnBeginInteraction
	// - which is what moves PendingInteractable to CurrentInteractingWith.
	PendingInteractable->OnBeginInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractBegun);
	PendingInteractable->OnFinishInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractFinished);
	PendingInteractable->OnCancelInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractCancelled);
	
	Server_StartInteracting(PendingInteractable);
}

void UInteractorComponent::StopInteracting()
{
	// Still waiting on server confirmation of the interaction - just cancel the pending request and unbind delegates.
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

	// If the ruleset specifies a deduction rate, drain progress gradually before cancelling.
	// This gives interactions a "sticky" feel - releasing briefly doesn't reset all progress.
	// DrainTimer will send Server_CancelInteraction once progress reaches zero.
	if(Ruleset && Ruleset->TimerDeductionRate > 0.f && InteractionProgress > 0.f)
		bIsDraining = true;
	else
		Server_CancelInteraction();
}

// ============================================================
// LOCAL LOGIC
// ============================================================

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
	
	// Notify the old interactable it lost focus, and the new one that it gained focus.
	// Focus is purely local, so this doesn't need to go through the server,
	// but the server is still notified so it can update other clients via NetMulticast.
	if (IsValid(CurrentFocusedInteractable))
	{
		CurrentFocusedInteractable->FocusLost(this);
		Server_NotifyFocusLost(CurrentFocusedInteractable);
	}

	// Reset interaction when focus is lost.
	if(IsValid(PendingInteractable) && !IsValid(CurrentInteractingWith))
	{
		UnbindDelegatesFrom(PendingInteractable);
		PendingInteractable = nullptr;
		Server_CancelInteraction();
	}
	else if(IsValid(CurrentInteractingWith))
	{
		bIsDraining = false;
		Server_CancelInteraction();
	}
	
	CurrentFocusedInteractable = NewFocused;

	if(IsValid(CurrentFocusedInteractable))
	{
		CurrentFocusedInteractable->FocusGained(this);
		Server_NotifyFocusGained(CurrentFocusedInteractable);
	}
}

void UInteractorComponent::AdvanceTimer(float DeltaTime)
{
	const UInteractionRuleset* Ruleset = CurrentInteractingWith->GetRuleset();

	// SecondsToTrigger <= 0 means instant interaction - submit without accumulating progress.
	if(!IsValid(Ruleset) || Ruleset->SecondsToTrigger <= 0.f)
	{
		SubmitInteraction();
		return;
	}

	TimerUpdate(DeltaTime / Ruleset->SecondsToTrigger, Ruleset->GlobalProgressUpdateThreshold);
	
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
	TimerUpdate(-DeltaTime * Ruleset->TimerDeductionRate / Ruleset->SecondsToTrigger, Ruleset->GlobalProgressUpdateThreshold);
	
	if(InteractionProgress <= 0.f)
	{
		bIsDraining = false;
		Server_CancelInteraction();
	}
}

// Helper for AdvanceTimer and DrainTimer to update progress and send updates to global progress handlers through the server.
void UInteractorComponent::TimerUpdate(float AddedValue, float UpdateThreshold)
{
	InteractionProgress += AddedValue;
	InteractionProgress = FMath::Clamp(InteractionProgress, 0.f, 1.f);

	CurrentInteractingWith->UpdateProgress(this, InteractionProgress);

	// --- Throttles frequency of progress updates sent through the sever ---
	const float ProgressDelta = FMath::Abs(InteractionProgress - LastSentProgress);
	if(ProgressDelta >= UpdateThreshold)
	{
		Server_NotifyProgress(CurrentInteractingWith, InteractionProgress);
		LastSentProgress = InteractionProgress;
	}
}

void UInteractorComponent::SubmitInteraction()
{
	if(!IsValid(CurrentInteractingWith))
	{
		UE_LOG(LogInteract, Warning, TEXT("SubmitInteraction called on %s but CurrentInteractingWith is invalid!"), *GetOwner()->GetName());
		return;
	}

	UE_LOG(LogInteract, Log, TEXT("SubmitInteraction called on %s, interacting with: %s"), *GetOwner()->GetName(), *CurrentInteractingWith->GetName());
	
	Server_RequestFinishInteraction();
	InteractionProgress = 0.f;
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
	LastSentProgress = 0.f;
	bIsDraining = false;
	CurrentInteractingWith = nullptr;
	PendingInteractable = nullptr;
	UE_LOG(LogInteract, Log, TEXT("ResetInteractionState called on %s"), *GetOwner()->GetName());
}

// ============================================================
// DELEGATE CALLBACKS
// Bound to the target interactable's delegates in StartInteracting.
// The Interactor != this guard ensures responses are only handled for
// events instigated by this component. 
// ============================================================

void UInteractorComponent::OnLocalInteractBegun(UInteractorComponent* Interactor)
{
	if(Interactor != this)
		return;

	// Server has confirmed the interaction - move form pending to active.
	CurrentInteractingWith = PendingInteractable;
	PendingInteractable = nullptr;
	InteractionProgress = 0.0f;
	bIsDraining = false;

	CurrentInteractingWith->InteractBegin(this, InteractionProgress);

	UE_LOG(LogInteract, Log, TEXT("OnLocalInteractBegun called on %s, now interacting with: %s"), *GetOwner()->GetName(), *CurrentInteractingWith->GetName());
}

void UInteractorComponent::OnLocalInteractFinished(UInteractorComponent* Interactor)
{
	if(Interactor != this)
		return;

	CurrentInteractingWith->InteractFinish(this, InteractionProgress);
	
	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();

	UE_LOG(LogInteract, Log, TEXT("OnLocalInteractFinished called on %s"), *GetOwner()->GetName());
}

void UInteractorComponent::OnLocalInteractCancelled(UInteractorComponent* Interactor)
{
	if(Interactor != this)
		return;

	CurrentInteractingWith->InteractCancel(this, InteractionProgress);

	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();

	UE_LOG(LogInteract, Log, TEXT("OnLocalInteractCancelled called on %s"), *GetOwner()->GetName());
}

// ============================================================
// SERVER RPCs
// ============================================================

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

	// The server doesn't receive OnRep callbacks, so CurrentInteractingWith must be set directly here to allow
	// Server_RequestFinishInteraction and Server_CancelInteraction to route correctly.
	CurrentInteractingWith = Target;
}

void UInteractorComponent::Server_RequestFinishInteraction_Implementation()
{
	if(!IsValid(CurrentInteractingWith))
		return;

	CurrentInteractingWith->FinishInteraction(this, InteractionProgress);
	CurrentInteractingWith = nullptr;
}

void UInteractorComponent::Server_CancelInteraction_Implementation()
{
	if(!IsValid(CurrentInteractingWith))
		return;

	CurrentInteractingWith->CancelInteraction(this, InteractionProgress);
	CurrentInteractingWith = nullptr;
}

void UInteractorComponent::Server_NotifyFocusGained_Implementation(UInteractableComponent* Target)
{
	if(!IsValid(Target))
		return;

	Target->Multicast_FocusGained(this);
}

void UInteractorComponent::Server_NotifyFocusLost_Implementation(UInteractableComponent* Target)
{
	if(!IsValid(Target))
		return;

	Target->Multicast_FocusLost(this);
}

void UInteractorComponent::Server_NotifyProgress_Implementation(UInteractableComponent* Target, const float Progress)
{
	if(!IsValid(Target))
		return;
	
	Target->Multicast_UpdateProgress(this, Progress);
}

// ============================================================
// CLIENT RPCs
// ============================================================

void UInteractorComponent::Client_InteractionRejected_Implementation()
{
	// Clean up whichever of the two might have been set at the time of rejection.
	UnbindDelegatesFrom(PendingInteractable);
	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();

	UE_LOG(LogInteract, Log, TEXT("Client_InteractionRejected called on %s"), *GetOwner()->GetName());
}
