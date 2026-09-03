#include "Core/CoopDownedComponent.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Core/CoopGameState.h"
#include "Core/CoopPlayerController.h"
#include "Core/GameConstants.h"
#include "Tags/CoopGameplayTags.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UCoopDownedComponent::UCoopDownedComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCoopDownedComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACoopCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (UCoopHealthComponent* Health = OwnerCharacter->GetHealthComponent())
	{
		Health->OnHealthDepleted.AddDynamic(this, &UCoopDownedComponent::HandleHealthDepleted);
	}

	if (USphereComponent* TriggerVolume = OwnerCharacter->GetReviveTriggerVolume())
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &UCoopDownedComponent::OnReviveTriggerBeginOverlap);
	}
}

bool UCoopDownedComponent::IsDowned() const
{
	return OwnerCharacter && OwnerCharacter->HasStatusTag(CoopGameplayTags::Status_Downed);
}

void UCoopDownedComponent::HandleHealthDepleted()
{
	SetDowned(true);
}

void UCoopDownedComponent::SetDowned(bool bNewDowned)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	USphereComponent* TriggerVolume = OwnerCharacter->GetReviveTriggerVolume();
	ACoopGameState* CoopGameState = OwnerCharacter->GetWorld() ? OwnerCharacter->GetWorld()->GetGameState<ACoopGameState>() : nullptr;
	const float RadiusUnits = GameConstants ? GameConstants->ReviveRadiusUnits : 150.0f;

	if (bNewDowned)
	{
		OwnerCharacter->ApplyPersistentStatusTag(CoopGameplayTags::Status_Downed);

		if (Movement)
		{
			Movement->DisableMovement();
		}

		// Enable the revive trigger only while actually Downed -- matches ACoopButton's
		// OverlapAllDynamic profile so any other character's capsule can walk into it.
		if (TriggerVolume)
		{
			TriggerVolume->SetSphereRadius(RadiusUnits);
			TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		}

		if (CoopGameState)
		{
			CoopGameState->IncrementDownedCount();
		}
	}
	else
	{
		OwnerCharacter->RemoveStatusTag(CoopGameplayTags::Status_Downed);

		if (Movement)
		{
			Movement->SetMovementMode(MOVE_Walking);
		}

		if (TriggerVolume)
		{
			TriggerVolume->SetCollisionProfileName(TEXT("NoCollision"));
		}

		if (CoopGameState)
		{
			CoopGameState->DecrementDownedCount();
		}
	}

	OnDownedStateChanged.Broadcast();
}

void UCoopDownedComponent::OnReviveTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Deliberately NOT ACoopButton's "only the locally-controlled client fires the RPC" filter --
	// ACoopButton is bReplicates=false and exists independently on every machine, so each client
	// needs IsLocallyControlled() to pick out its own pawn among N client-local overlap events.
	// This delegate is only ever bound on the server (BeginPlay's HasAuthority() gate above), so
	// there is exactly one overlap evaluation, already server-authoritative -- IsLocallyControlled()
	// here would read true only for the listen server's own host pawn, silently dropping every
	// other (remote) player's revive attempt. Caught during M9 verification before it could hide
	// behind a false "tooling gap" diagnosis.
	ACoopCharacter* Reviver = Cast<ACoopCharacter>(OtherActor);
	if (!Reviver || Reviver == OwnerCharacter)
	{
		return;
	}

	if (ACoopPlayerController* PC = Cast<ACoopPlayerController>(Reviver->GetController()))
	{
		PC->Server_AttemptRevive();
	}
}

void UCoopDownedComponent::BeginRevive(ACoopCharacter* Reviver)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !IsDowned() || !Reviver)
	{
		return;
	}

	if (ReviverInProgress.IsValid())
	{
		return;
	}

	ReviverInProgress = Reviver;

	const float Duration = GameConstants ? GameConstants->ReviveDurationSeconds : 3.0f;
	OwnerCharacter->GetWorldTimerManager().SetTimer(ReviveTimerHandle, this, &UCoopDownedComponent::CompleteRevive, Duration, false);
}

void UCoopDownedComponent::CompleteRevive()
{
	ACoopCharacter* Reviver = ReviverInProgress.Get();
	ReviverInProgress = nullptr;

	// Re-validate rather than trust the state from Duration seconds ago: the reviver may have
	// walked away, died, or gone Downed themselves mid-channel.
	if (!OwnerCharacter || !IsDowned() || !Reviver || Reviver->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	const float RadiusUnits = GameConstants ? GameConstants->ReviveRadiusUnits : 150.0f;
	const float DistanceSq = FVector::DistSquared(OwnerCharacter->GetActorLocation(), Reviver->GetActorLocation());
	if (DistanceSq > FMath::Square(RadiusUnits))
	{
		return;
	}

	SetDowned(false);

	const float RestorePercent = GameConstants ? GameConstants->ReviveHealthRestorePercent : 0.5f;
	if (UCoopHealthComponent* Health = OwnerCharacter->GetHealthComponent())
	{
		Health->Revive(RestorePercent);
	}
}

void UCoopDownedComponent::ForceRevive()
{
	if (!IsDowned() || !OwnerCharacter)
	{
		return;
	}

	OwnerCharacter->GetWorldTimerManager().ClearTimer(ReviveTimerHandle);
	ReviverInProgress = nullptr;

	SetDowned(false);

	// Full heal on a full-party scene reset -- distinct from BeginRevive's partial
	// ReviveHealthRestorePercent, since a scene reset is meant to send everyone back in fresh.
	if (UCoopHealthComponent* Health = OwnerCharacter->GetHealthComponent())
	{
		Health->Revive(1.0f);
	}
}
