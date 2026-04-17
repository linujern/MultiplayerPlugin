#include "PresetImplementations/CooldownRegulationHandler.h"
#include "InteractionDeniedContext.h"
#include "InteractionDeniedGameplayTags.h"
#include "InteractorComponent.h"
#include "LogInteractionSystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

// ================================================================================================
//	UCOOLDOWNREGULATIONCOMPONENT
// ================================================================================================

UCooldownRegulationHandler::UCooldownRegulationHandler()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UCooldownRegulationHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCooldownRegulationHandler, GlobalCooldownExpiry);
}

// ------------------------------------------------------------------------------------------------
//	Gating Queries
// ------------------------------------------------------------------------------------------------

bool UCooldownRegulationHandler::CanInteract_Global_Implementation(const UInteractableComponent* Interactable, UInteractorComponent* Interactor, FInteractionDeniedContext& DeniedContext)
{
	// Reads replicated GlobalCooldownExpiry — consistent result on all clients.
	if (IsGlobalCooldownActive())
	{
		DeniedContext = FInteractionDeniedContext(
			TAG_Interaction_Denied_Cooldown_Global,
			NSLOCTEXT("ExtensibleInteractionSystem", "GlobalCooldown", "Not available yet"),
			this);
		return false;
	}
	return true;
}

bool UCooldownRegulationHandler::CanInteract_Local_Implementation(const UInteractableComponent* Interactable, UInteractorComponent* Interactor, FInteractionDeniedContext& DeniedContext)
{
	if (PerPlayerCooldownSeconds <= 0.f)
		return true;

	// Reads local satellite state on this player's pawn — only valid on the local client.
	const UCooldownLocalStateComponent* State = Interactor->GetOwner()->FindComponentByClass<UCooldownLocalStateComponent>();
	if (State && State->IsLockedFor(this))
	{
		DeniedContext = FInteractionDeniedContext(
			TAG_Interaction_Denied_Cooldown_PerPlayer,
			NSLOCTEXT("ExtensibleInteractionSystem", "LockedFor", "Not available yet"),
			this);
		return false;
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
//	Notification Callbacks
// ------------------------------------------------------------------------------------------------

void UCooldownRegulationHandler::OwnerInteractFinish_Implementation(UInteractableComponent* Interactable, UInteractorComponent* Interactor)
{
	const float Now = GetWorld()->GetTimeSeconds();

	// --- Global cooldown ---
	if (GlobalCooldownSeconds > 0.f)
	{
		// Written server-side, replicated to all clients automatically.
		GlobalCooldownExpiry = Now + GlobalCooldownSeconds;

		UE_LOG(LogInteract, Log, TEXT("%s: Global cooldown set for %.1fs"), *GetOwner()->GetName(), GlobalCooldownSeconds);
	}

	// --- Per-player cooldown ---
	if (PerPlayerCooldownSeconds > 0.f)
		// Notify all clients — each decides locally whether the interactor belongs to them.
		Multicast_NotifyPerPlayerLock(Interactor, PerPlayerCooldownSeconds);
}

// ------------------------------------------------------------------------------------------------
//	OnRep
// ------------------------------------------------------------------------------------------------

void UCooldownRegulationHandler::OnRep_GlobalCooldownExpiry() const
{
	UE_LOG(LogInteract, Log, TEXT("%s: GlobalCooldownExpiry replicated — expires in %.1fs"),
		*GetOwner()->GetName(), GlobalCooldownExpiry - GetWorld()->GetTimeSeconds());
}

// ------------------------------------------------------------------------------------------------
//	Internal
// ------------------------------------------------------------------------------------------------

bool UCooldownRegulationHandler::IsGlobalCooldownActive() const
{
	return GlobalCooldownExpiry > 0.f && GetWorld()->GetTimeSeconds() < GlobalCooldownExpiry;
}

void UCooldownRegulationHandler::Multicast_NotifyPerPlayerLock_Implementation(UInteractorComponent* Interactor, float Duration)
{
	if (!IsValid(Interactor))
		return;

	// Only the machine that locally controls this pawn acts on this notification.
	// Other clients discard it — they have no interest in someone else's cooldown.
	const APawn* Pawn = Cast<APawn>(Interactor->GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
		return;

	UCooldownLocalStateComponent* State = GetOrCreateLocalState(Interactor);
	State->RecordLock(this, GetWorld()->GetTimeSeconds() + Duration);

	UE_LOG(LogInteract, Log, TEXT("%s: Per-player cooldown set on %s for %.1fs"), *GetOwner()->GetName(), *Pawn->GetName(), Duration);
}

// Get every time instead of caching because of one-to-many relationship. The lookup is cheap, anyway.
UCooldownLocalStateComponent* UCooldownRegulationHandler::GetOrCreateLocalState(const UInteractorComponent* Interactor)
{
	AActor* PawnActor = Interactor->GetOwner();

	UCooldownLocalStateComponent* State = PawnActor->FindComponentByClass<UCooldownLocalStateComponent>();

	if (!State)
	{
		State = NewObject<UCooldownLocalStateComponent>(PawnActor);
		State->RegisterComponent();
	}

	return State;
}

// ================================================================================================
//	UCOOLDOWNLOCALSTATECOMPONENT
// ================================================================================================

void UCooldownLocalStateComponent::RecordLock(UCooldownRegulationHandler* Source, float ExpiryTime)
{
	LockExpiries.Add(Source, ExpiryTime);

	// Schedule cleanup after this lock expires.
	// If multiple locks are recorded, this resets to the most recent —
	// TryCleanup will reschedule itself if earlier entries are still live.
	const float TimeUntilExpiry = ExpiryTime - GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		this,
		&UCooldownLocalStateComponent::TryCleanup,
		FMath::Max(TimeUntilExpiry, 0.1f),
		false);
}

bool UCooldownLocalStateComponent::IsLockedFor(const UCooldownRegulationHandler* Source) const
{
	const float* Expiry = LockExpiries.Find(Source);
	return Expiry && GetWorld()->GetTimeSeconds() < *Expiry;
}

void UCooldownLocalStateComponent::TryCleanup()
{
	const float Now = GetWorld()->GetTimeSeconds();

	float LatestExpiry = 0.f;

	for (auto It = LockExpiries.CreateIterator(); It; ++It)
	{
		if (Now >= It->Value)
			It.RemoveCurrent();
		else
			// Track the latest remaining expiry to reschedule cleanup
			LatestExpiry = FMath::Max(LatestExpiry, It->Value);
	}

	if (LockExpiries.IsEmpty())
	{
		// All locks expired — no reason to keep this component around
		UE_LOG(LogInteract, Log, TEXT("UCooldownLocalStateComponent on %s: all locks expired, self-destructing"), *GetOwner()->GetName());

		DestroyComponent();
	}
	else
	{
		// Some locks still live — reschedule for the next expiry
		const float NextCleanup = LatestExpiry - Now;
		GetWorld()->GetTimerManager().SetTimer(
			CleanupTimerHandle,
			this,
			&UCooldownLocalStateComponent::TryCleanup,
			FMath::Max(NextCleanup, 0.1f),
			false);
	}
}