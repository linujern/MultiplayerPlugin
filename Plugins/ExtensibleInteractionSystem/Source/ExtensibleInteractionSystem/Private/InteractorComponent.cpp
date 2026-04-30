#include "InteractorComponent.h"
#include "InteractableComponent.h"
#include "InteractionDeniedContext.h"
#include "InteractionRuleset.h"
#include "InteractionSettings.h"
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

	if (!InteractionTracer)
	{
		const UInteractionSettings* Settings = GetDefault<UInteractionSettings>();
		InteractionTracer = NewObject<UInteractionTracer>(this, Settings->GetDefaultTracerClass());
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
	// Exit early if there is nothing to interact with, or if an interaction is already pending.
	if (!IsValid(CurrentFocusedInteractable) || IsValid(PendingInteractable))
		return;

	// Clear delegate bindings from previously started interactions, if any exist.
	if (IsValid(CurrentInteractingWith))
		UnbindDelegatesFrom(CurrentInteractingWith);

	// Perform interaction gating checks on the server before allowing the interaction to begin, to ensure consistent rules enforcement and prevent cheating.
	FInteractionDeniedContext DeniedContext;
	const bool bAllowed = CurrentFocusedInteractable->EvaluateInteractionGates(this, DeniedContext, true);

	if (!bAllowed)
	{
		UE_LOG(LogInteract, Verbose, TEXT("StartInteracting: %s was denied interaction with %s, reason: %s"), *GetOwner()->GetName(), *PendingInteractable->GetName(), *DeniedContext.Reason.ToString());
		return;
	}

	PendingInteractable = CurrentFocusedInteractable;

	UE_LOG(LogInteract, Verbose, TEXT("StartInteracting called on %s, requesting interaction with: %s"), *GetOwner()->GetName(), *PendingInteractable->GetName());
	
	// Bind to callbacks which ensure the interactable has handled its logic properly.
	PendingInteractable->OnBeginInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractBegun);
	PendingInteractable->OnFinishInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractFinished);
	PendingInteractable->OnCancelInteraction.AddDynamic(this, &UInteractorComponent::OnLocalInteractCancelled);

	Server_StartInteracting(CurrentFocusedInteractable);
}

void UInteractorComponent::StopInteracting()
{
	// Still waiting on server confirmation of the interaction - just cancel the pending request and unbind delegates.
	if(IsValid(PendingInteractable) && !IsValid(CurrentInteractingWith))
	{
		UE_LOG(LogInteract, Verbose, TEXT("StopInteracting called on %s, cancelled pending interaction with: %s"), *GetOwner()->GetName(), *PendingInteractable->GetName());
		UnbindDelegatesFrom(PendingInteractable);
		PendingInteractable = nullptr;
		Server_CancelInteraction(InteractionProgress);
		return;
	}

	// Nothing to stop interacting with
	if(!IsValid(CurrentInteractingWith))
		return;

	const UInteractionRuleset* Ruleset = CurrentInteractingWith->GetRuleset();

	// If the ruleset specifies a deduction rate, drain progress gradually before cancelling.
	// This gives interactions a "sticky" feel - releasing briefly doesn't reset all progress.
	// DrainTimer will send Server_CancelInteraction once progress reaches zero.
	if(Ruleset && Ruleset->TimerDeductionRate > 0.f && InteractionProgress > 0.f)
		bIsDraining = true;
	else
		Server_CancelInteraction(InteractionProgress);

	UE_LOG(LogInteract, Verbose, TEXT("StopInteracting called on %s"), *GetOwner()->GetName());
}

// ============================================================
// LOCAL LOGIC
// ============================================================

void UInteractorComponent::TickTrace()
{
	if(!InteractionTracer)
		return;
	
	UInteractableComponent* Found = InteractionTracer->FindBestInteractable(GetOwner(), this);
	UpdateCurrentFocusedInteractable(Found);
}

void UInteractorComponent::UpdateCurrentFocusedInteractable(UInteractableComponent* NewFocused)
{
	// If the new focused is the old focused then there is no change and no need to do anything.
	if (CurrentFocusedInteractable == NewFocused)
		return;

	FInteractionDeniedContext DeniedContext = FInteractionDeniedContext();

	// If the new focused cannot be focused, ignore the input.
	if(IsValid(NewFocused) && !NewFocused->IsFocusable(this, DeniedContext))
		return;

	UE_LOG(LogInteract, Verbose, TEXT
		("Updating focused interactable for %s. Old focused: %s. New focused: %s"),
		*GetOwner()->GetName(),
		IsValid(CurrentFocusedInteractable) ? *CurrentFocusedInteractable->GetName() : TEXT("None(NullObject)"),
		IsValid(NewFocused) ? *NewFocused->GetName() : TEXT("None(NullObject)"));
	
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
		Server_CancelInteraction(InteractionProgress);
		UE_LOG(LogInteract, VeryVerbose, TEXT
			("Focus changed while waiting for server confirmation of interaction. Cancelled pending interaction with: %s"),
			*PendingInteractable->GetName());
	}
	else if(IsValid(CurrentInteractingWith))
	{
		bIsDraining = false;
		Server_CancelInteraction(InteractionProgress);
		UE_LOG(LogInteract, VeryVerbose, TEXT
			("Focus changed while interacting. Cancelled interaction with: %s"),
			*CurrentInteractingWith->GetName());
		
	}
	// The old focused interactable is now fully handled.

	// Update the new focused interactable.
	CurrentFocusedInteractable = NewFocused;

	if(IsValid(NewFocused))
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
		Server_CancelInteraction(InteractionProgress);
		return;
	}

	// TimerDeductionRate is a multiplier on the fill rate: 1.0 means it drains as quickly as it fills, 2.0 drains twice as fast
	TimerUpdate(-DeltaTime * Ruleset->TimerDeductionRate / Ruleset->SecondsToTrigger, Ruleset->GlobalProgressUpdateThreshold);
	
	if(InteractionProgress <= 0.f)
	{
		bIsDraining = false;
		Server_CancelInteraction(InteractionProgress);
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
		return;

	// Once the timer has finished - submit the interaction to the server, which will trigger the authority logic and broadcast the result back to clients.
	Server_RequestFinishInteraction(InteractionProgress);
	InteractionProgress = 0.f;

	UE_LOG(LogInteract, Verbose, TEXT("SubmitInteraction called on %s, interacting with: %s"), *GetOwner()->GetName(), *CurrentInteractingWith->GetName());
}

void UInteractorComponent::UnbindDelegatesFrom(UInteractableComponent* Target)
{
	if(!IsValid(Target))
		return;

	Target->OnBeginInteraction.RemoveDynamic(this, &UInteractorComponent::OnLocalInteractBegun);
	Target->OnFinishInteraction.RemoveDynamic(this, &UInteractorComponent::OnLocalInteractFinished);
	Target->OnCancelInteraction.RemoveDynamic(this, &UInteractorComponent::OnLocalInteractCancelled);
	
	UE_LOG(LogInteract, Verbose, TEXT("Unbinding %s's delegates from %s"), *Target->GetName(), *GetOwner()->GetName());
}

void UInteractorComponent::ResetInteractionState()
{
	InteractionProgress = 0.f;
	LastSentProgress = 0.f;
	bIsDraining = false;
	CurrentInteractingWith = nullptr;
	PendingInteractable = nullptr;

	UE_LOG(LogInteract, Verbose, TEXT("ResetInteractionState called on %s"), *GetOwner()->GetName());
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

	// Exit early if no Interactable is being considered for interaction.
	if(!IsValid(PendingInteractable))
		return;
	
	// Server has confirmed the interaction - move from pending to active.
	CurrentInteractingWith = PendingInteractable;
	PendingInteractable = nullptr;
	// Is not draining when interaction is just starting.
	bIsDraining = false;

	// Notify CurrentInteractingWith that the interaction has passed all checks, so it can trigger its own local visual handlers and other logic.
	CurrentInteractingWith->InteractBegin(this, InteractionProgress);

	UE_LOG(LogInteract, Verbose, TEXT("OnLocalInteractBegun called on %s, now interacting with: %s"), *GetOwner()->GetName(), *CurrentInteractingWith->GetName());
}

void UInteractorComponent::OnLocalInteractFinished(UInteractorComponent* Interactor)
{
	if(Interactor != this || !IsValid(CurrentInteractingWith))
		return;

	CurrentInteractingWith->InteractFinish(this, InteractionProgress);
	
	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();

	UE_LOG(LogInteract, Verbose, TEXT("OnLocalInteractFinished called on %s"), *GetOwner()->GetName());
}

void UInteractorComponent::OnLocalInteractCancelled(UInteractorComponent* Interactor)
{
	if(Interactor != this  || !IsValid(CurrentInteractingWith))
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

	FInteractionDeniedContext DeniedContext;
	const bool bAllowed = Target->EvaluateInteractionGates(this, DeniedContext, false);

	if (!bAllowed)
	{
		Client_InteractionRejected();
		return;
	}
	
	UE_LOG(LogInteract, Verbose, TEXT("Server_StartInteracting called on %s, requested interaction with: %s"), *GetOwner()->GetName(), *Target->GetName());

	Target->BeginInteraction(this, InteractionProgress);
	CurrentInteractingWith = Target;
}

void UInteractorComponent::Server_RequestFinishInteraction_Implementation(float Progress)
{
	if(!IsValid(CurrentInteractingWith))
		return;

	CurrentInteractingWith->FinishInteraction(this, Progress);
	CurrentInteractingWith = nullptr;
}

void UInteractorComponent::Server_CancelInteraction_Implementation(float Progress)
{
	if(!IsValid(CurrentInteractingWith))
		return;

	CurrentInteractingWith->CancelInteraction(this, Progress);
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
	UE_LOG(LogInteract, Verbose, TEXT("Client_InteractionRejected called on %s targeting %s or %s"),
		*GetOwner()->GetName(),
		IsValid(PendingInteractable) ? *PendingInteractable->GetName() : TEXT("null"),
		IsValid(CurrentInteractingWith) ? *CurrentInteractingWith->GetName() : TEXT("null"));
	
	// Clean up whichever of the two might have been set at the time of rejection.
	UnbindDelegatesFrom(PendingInteractable);
	UnbindDelegatesFrom(CurrentInteractingWith);
	ResetInteractionState();
}
