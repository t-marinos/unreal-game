#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "CoopMonsterCharacter.generated.h"

class UCoopHealthComponent;
class UCoopFixateRetargetComponent;
class UStaticMeshComponent;
class UGameConstants;
class ACoopCharacter;

// Build 1, M11: a basic trash monster for Hold the Gate (docs/scenes/HOLD_THE_GATE.md) --
// DECISIONS.md's "Monster combat inside Hold the Gate" entry. Health via M6's UCoopHealthComponent,
// targeting via the reusable UCoopFixateRetargetComponent, and a simple periodic direct-damage
// attack against its current target. No movement, no AAIController, no pathfinding -- that entry
// explicitly does not authorize adaptive AI/pathfinding anywhere; monsters are stationary ranged
// harassers that Tank's Shield/Fortress exist to counter via the existing tag-based damage negation
// (CoopHealthComponent.cpp), not via any new physical blocking mechanic.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopMonsterCharacter : public AActor
{
	GENERATED_BODY()

public:
	ACoopMonsterCharacter();

	UFUNCTION(BlueprintPure, Category = "Monster")
	UCoopHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Monster")
	UCoopFixateRetargetComponent* GetTargetingComponent() const { return TargetingComponent; }

	// Build 1: minimal tag support so Damage's Execution (docs/abilities.md) has something to read
	// Status.Vulnerable.Physical off of -- monsters are the only enemy actor that exists yet.
	// Mirrors ACoopCharacter::HasStatusTag/ApplyStatusTag/RemoveStatusTag's shape (server-timestamped
	// expiry, no client-ticked countdown) but is a separate, smaller implementation rather than a
	// shared base class, per CLAUDE.md §4.6's "explicit code per class over a premature shared
	// abstraction" philosophy -- monsters don't need ApplyPersistentStatusTag or the full
	// per-tag-expiry-timestamp bookkeeping ACoopCharacter keeps for debug-dump purposes.
	UFUNCTION(BlueprintPure, Category = "Monster")
	bool HasStatusTag(FGameplayTag Tag) const { return ActiveStatusTags.HasTag(Tag); }

	// Server-only (CLAUDE.md §4.1).
	void ApplyStatusTag(FGameplayTag Tag, float DurationSeconds);
	void RemoveStatusTag(FGameplayTag Tag);

	// Server-only. Called once by ACoopMonsterSpawner right after spawning: gives this monster its
	// initial fixate candidate pool, overrides its health from GameConstants->MonsterHealth (the
	// component's own BeginPlay already ran by construction order and set the player-oriented
	// DefaultMaxHealth default), binds to the initial target's Downed delegate, and starts the
	// attack timer.
	void InitializeMonster(const TArray<AActor*>& InitialCandidates);

protected:
	virtual void BeginPlay() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void BindToTargetDownedDelegate();
	void UnbindFromTargetDownedDelegate();

	UFUNCTION()
	void HandleTargetDownedStateChanged();

	void PerformRetarget();
	void PerformAttackTick();

	UFUNCTION()
	void HandleHealthDepleted();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCoopHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCoopFixateRetargetComponent> TargetingComponent;

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10 -- set via BP_MonsterCharacter's
	// CDO, same CDO-persistence reasoning as every other class holding an EditDefaultsOnly asset
	// reference.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	// Tracks which ACoopCharacter's OnDownedStateChanged this monster is currently bound to, so a
	// retarget can cleanly unbind from the old target before binding the new one.
	UPROPERTY()
	TWeakObjectPtr<ACoopCharacter> BoundTargetForDelegate;

	FTimerHandle AttackTimerHandle;
	FTimerHandle RetargetDelayTimerHandle;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Monster")
	FGameplayTagContainer ActiveStatusTags;

	TMap<FGameplayTag, FTimerHandle> StatusTagExpiryTimers;
};
