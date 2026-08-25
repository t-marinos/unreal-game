#include "Core/CoopHealthComponent.h"
#include "Core/GameConstants.h"
#include "Core/CoopPlayerState.h"
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

	const bool bWasAboveZero = CurrentHealth > 0.0f;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

	if (bWasAboveZero && CurrentHealth <= 0.0f)
	{
		OnHealthDepleted.Broadcast();
	}
}

void UCoopHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCoopHealthComponent, CurrentHealth);
	DOREPLIFETIME(UCoopHealthComponent, MaxHealth);
}
