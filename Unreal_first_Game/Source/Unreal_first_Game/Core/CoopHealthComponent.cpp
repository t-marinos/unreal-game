#include "Core/CoopHealthComponent.h"
#include "Core/GameConstants.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopCharacter.h"
#include "Tags/CoopGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

UCoopHealthComponent::UCoopHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCoopHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only the server ever writes these -- clients receive them via replication, per CLAUDE.md
	// §4.1 (gameplay state is written only where HasAuthority() is true).
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const float ResolvedMaxHealth = GameConstants ? GameConstants->DefaultMaxHealth : 100.0f;
	if (!GameConstants)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCoopHealthComponent::BeginPlay: GameConstants not set on %s, falling back to 100.0."), *GetNameSafe(GetOwner()));
	}

	MaxHealth = ResolvedMaxHealth;
	CurrentHealth = ResolvedMaxHealth;
}

void UCoopHealthComponent::ApplyDamage(float DamageAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || DamageAmount <= 0.0f)
	{
		return;
	}

	// M6: wires the dev-mode god-mode toggle stubbed on ACoopPlayerState back in Build 0 -- that
	// property's own comment flagged this as the exact spot to check once a damage system exists.
	// Not every owner is a possessed player pawn (M11's monsters will use this same component with
	// no PlayerState at all), so this only gates when one is actually present.
	if (const APawn* OwningPawn = Cast<APawn>(GetOwner()))
	{
		if (const ACoopPlayerState* CoopPS = OwningPawn->GetPlayerState<ACoopPlayerState>())
		{
			if (CoopPS->IsInvulnerable())
			{
				return;
			}
		}
	}

	// Build 1, M7: Tank's Shield negates all incoming damage while Status.Shielded is active.
	// MONSTER_ENEMIES_PROGRESS.md Phase B: Status.Fortress -- the Stabilize upgrade of Shield --
	// negates it the same way. Fortress was written by Stabilize and shown on the status badge, but
	// until monster strikes existed nothing actually READ it for defence (the gap this pass closes).
	// The Shield -> Fortress distinction lives elsewhere: Fortress covers a radius of teammates (not
	// just Tank's cone) AND resists knockback (ACoopMonsterCharacter::PerformStrike); Status.Shielded
	// does neither. Simplification unchanged from M7 -- negates unconditionally, no damage-source
	// facing check (ApplyDamage still has no source location to check against).
	if (const ACoopCharacter* OwningCharacter = Cast<ACoopCharacter>(GetOwner()))
	{
		if (OwningCharacter->HasStatusTag(CoopGameplayTags::Status_Shielded)
			|| OwningCharacter->HasStatusTag(CoopGameplayTags::Status_Fortress))
		{
			return;
		}
	}

	const bool bWasAboveZero = CurrentHealth > 0.0f;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

	if (bWasAboveZero && CurrentHealth <= 0.0f)
	{
		OnHealthDepleted.Broadcast();
	}
}

void UCoopHealthComponent::Revive(float HealthPercent)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentHealth = FMath::Clamp(MaxHealth * HealthPercent, 0.0f, MaxHealth);
}

void UCoopHealthComponent::SetMaxHealth(float NewMaxHealth)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	MaxHealth = NewMaxHealth;
	CurrentHealth = NewMaxHealth;
}

void UCoopHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCoopHealthComponent, CurrentHealth);
	DOREPLIFETIME(UCoopHealthComponent, MaxHealth);
}
