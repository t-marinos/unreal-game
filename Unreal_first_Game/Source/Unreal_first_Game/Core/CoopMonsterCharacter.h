#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "CoopMonsterCharacter.generated.h"

class UCoopHealthComponent;
class UCoopFixateRetargetComponent;
class UGameConstants;
class ACoopCharacter;

// Build 1, M11: a basic trash monster for Hold the Gate (docs/scenes/HOLD_THE_GATE.md) --
// DECISIONS.md's "Monster combat inside Hold the Gate" entry. Health via M6's UCoopHealthComponent,
// targeting via the reusable UCoopFixateRetargetComponent, and a periodic direct-damage attack
// against its current target.
//
// MONSTER_ENEMIES_PROGRESS.md (2026-09-04): reparented AActor -> ACharacter. It now has a capsule,
// the stock Mannequin body (wired on BP_MonsterCharacter), and a CharacterMovementComponent driven
// by ACoopMonsterAIController's straight-line steering toward the fixate target. The attack is
// melee-range-gated (Phase A) and gets a telegraphed windup + knockback (Phase B). "No pathfinding"
// from the DECISIONS.md entry still holds -- the controller uses AddMovementInput, never a navmesh
// query or a behaviour tree. Body-blocking the monster with Tank's capsule, and knocking it away
// with Shield, are the intended counterplay.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopMonsterCharacter : public ACharacter
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
	// DefaultMaxHealth default), overrides its walk speed from GameConstants->MonsterMoveSpeed,
	// binds to the initial target's Downed delegate, and starts the attack timer.
	void InitializeMonster(const TArray<AActor*>& InitialCandidates);

	// Read by ACoopMonsterAIController each tick (its straight-line stop distance) and by
	// PerformAttackTick's own range gate. GameConstants->MonsterMeleeRangeUnits, with a fallback if
	// the data asset isn't assigned.
	float GetMeleeRangeUnits() const;

protected:
	virtual void BeginPlay() override;

	// Destroy the possessing ACoopMonsterAIController when this monster goes away -- covers both
	// death (HandleHealthDepleted) and the scene-reset sweep (CoopHoldTheGateScene::ResetScene
	// calls Destroy() directly), so no orphan controller leaks per dead monster.
	virtual void Destroyed() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void BindToTargetDownedDelegate();
	void UnbindFromTargetDownedDelegate();

	UFUNCTION()
	void HandleTargetDownedStateChanged();

	void PerformRetarget();

	// MONSTER_ENEMIES_PROGRESS.md Phase B: the repeating MonsterAttackIntervalSeconds tick. When the
	// target is in melee range and no windup is already in flight, it starts one -- spawns
	// ActiveTelegraph and sets WindupTimerHandle -> PerformStrike -- rather than hitting instantly.
	void PerformAttackTick();

	// Fired once by WindupTimerHandle, MonsterAttackWindupSeconds after PerformAttackTick started a
	// windup. Clears the windup state + telegraph, then (if the target is still valid + in melee
	// range + not Downed) applies MonsterAttackDamage and knockback. Out of range -> whiff.
	void PerformStrike();

	UFUNCTION()
	void HandleHealthDepleted();

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
	FTimerHandle WindupTimerHandle;

	// MONSTER_ENEMIES_PROGRESS.md Phase B: telegraphed melee strike. PerformAttackTick sets
	// bWindingUp + spawns ActiveTelegraph + arms WindupTimerHandle; PerformStrike (and HandleHealth-
	// Depleted / Destroyed) clear all three. bWindingUp gates PerformAttackTick re-entry while a
	// windup is in flight. "About to hit" is transient AI state -- deliberately NOT an FGameplayTag,
	// nothing else reads it, so no docs/abilities.md change.
	bool bWindingUp = false;

	UPROPERTY()
	TWeakObjectPtr<AActor> ActiveTelegraph;

	// The flat red ground-ring actor (ACoopMonsterStrikeTelegraph) spawned during a strike windup --
	// set to BP_MonsterStrikeTelegraph on BP_MonsterCharacter's CDO, same content-wiring pattern as
	// every other TSubclassOf reference. Left unset just means no visible telegraph; the strike
	// still resolves.
	UPROPERTY(EditDefaultsOnly, Category = "Monster")
	TSubclassOf<AActor> StrikeTelegraphClass;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Monster")
	FGameplayTagContainer ActiveStatusTags;

	TMap<FGameplayTag, FTimerHandle> StatusTagExpiryTimers;
};
