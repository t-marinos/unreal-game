#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoopHealthComponent.generated.h"

class UGameConstants;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepleted);

// Build 1, M6. Health foundation shared by every ACoopCharacter -- Tank's Shield damage negation
// (M7) and Downed's "0 HP" trigger (M9) both need a concept of HP, which didn't exist anywhere in
// Build 0. Server-authoritative per CLAUDE.md §4.1: ApplyDamage() (server-only) is the only thing
// that ever changes CurrentHealth; every client just reads the replicated value.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_FIRST_GAME_API UCoopHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoopHealthComponent();

	// Server-only (CLAUDE.md §4.1) -- the only way CurrentHealth ever changes. Clamps to
	// [0, MaxHealth]. Broadcasts OnHealthDepleted exactly once per depletion (detected as "was
	// above 0 before this call, at or below 0 after it"), so repeated damage against an
	// already-0-HP actor doesn't refire it, while a future heal/revive naturally re-arms it without
	// needing a separate reset call -- M9's downed component subscribes to this to enter Downed.
	void ApplyDamage(float DamageAmount);

	// Server-only (CLAUDE.md §4.1). Build 1, M9: restores CurrentHealth to Percent * MaxHealth,
	// clamped to [0, MaxHealth]. Used by Downed's revive -- unlike ApplyDamage, this never
	// broadcasts OnHealthDepleted (that only fires on a fresh crossing into 0, never on a heal).
	void Revive(float HealthPercent);

	// Server-only. Build 1, M11: overrides MaxHealth after BeginPlay's DefaultMaxHealth-based
	// initialization, for owners whose max HP isn't the player default (M11's monsters use
	// GameConstants->MonsterHealth instead). Also resets CurrentHealth to match -- only ever meant
	// to be called once, immediately after spawn, before any damage.
	void SetMaxHealth(float NewMaxHealth);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDepleted OnHealthDepleted;

protected:
	virtual void BeginPlay() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10. This is a component subobject on
	// ACoopCharacter, so the CDO-persistence gotcha applies the same way it does to any other new
	// C++ class's EditDefaultsOnly reference -- set this via BP_PlayerCharacter's component
	// defaults (the Blueprint wrapper), not directly on the C++ CDO.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Health")
	float MaxHealth = 100.0f;
};
