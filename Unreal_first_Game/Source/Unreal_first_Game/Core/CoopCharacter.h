#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "CoopCharacter.generated.h"

class UCoopHealthComponent;
class UCoopDownedComponent;
class USphereComponent;
class UWidgetComponent;
class UGameConstants;
class UAnimMontage;

// Mesh/animation come from a Blueprint reparent of BP_ThirdPersonCharacter (M4), keeping its
// already-working setup -- this class only adds the per-player colour tint on top.
// Default CharacterMovementComponent prediction is left untouched here and stays on
// deliberately, per CLAUDE.md §4.2: movement prediction is the one accepted exception
// to "no client-side prediction of gameplay state."
UCLASS()
class UNREAL_FIRST_GAME_API ACoopCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACoopCharacter();

	// Build 1, M6. Every ACoopCharacter gets one -- Shield's damage negation (M7) and Downed's
	// 0-HP trigger (M9) both read/subscribe through this.
	UFUNCTION(BlueprintPure, Category = "Health")
	UCoopHealthComponent* GetHealthComponent() const { return HealthComponent; }

	// Build 1, M9. Every ACoopCharacter gets one -- subscribes to HealthComponent's 0-HP delegate
	// to enter Downed instead of dying (CLAUDE.md §6.6).
	UFUNCTION(BlueprintPure, Category = "Downed")
	UCoopDownedComponent* GetDownedComponent() const { return DownedComponent; }

	// Build 1, M9: overlap-only sphere (mirrors ACoopButton's TriggerVolume pattern) that
	// UCoopDownedComponent enables/resizes only while this character is Downed -- the capsule's own
	// collision blocks other pawns by default, so it never fires the overlap this needs.
	UFUNCTION(BlueprintPure, Category = "Downed")
	USphereComponent* GetReviveTriggerVolume() const { return ReviveTriggerVolume; }

	// Build 1: overhead status badge (CLAUDE.md §5) -- every ACoopCharacter gets one, same
	// reasoning as HealthComponent/DownedComponent above: a teammate caught in Shield's cone or
	// Fortress's radius needs the same badge as the Tank/Control who cast the ability, not just the
	// caster. See UCoopStatusBarWidget.
	UFUNCTION(BlueprintPure, Category = "Status")
	UWidgetComponent* GetStatusBarWidgetComponent() const { return StatusBarWidgetComponent; }

	// WoW-style movement (DECISIONS.md's "Camera follows the player"/movement-direction entry):
	// BP_PlayerCharacter's Move function scales the backward AddMovementInput axis by this before
	// CharacterMovementComponent's analog input modifier turns it into an effective speed cap, so
	// walking backward is slower than forward/strafing. 1.0 (no fallback GameConstants) means "no
	// backpedal penalty" rather than silently capping speed if the data asset is unassigned.
	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetBackpedalSpeedMultiplier() const;

	// Build 1, M7: replicated status-effect state, shared by every tag the game applies --
	// Status.Shielded now, Status.Fortress (M8) and Status.Downed (M9) reuse the same mechanism
	// rather than each growing its own bespoke container/timer pair.
	UFUNCTION(BlueprintPure, Category = "Status")
	bool HasStatusTag(FGameplayTag Tag) const { return ActiveStatusTags.HasTag(Tag); }

	// Server-only (CLAUDE.md §4.1). Adds Tag to the replicated container -- idempotent, so
	// reapplying an already-active tag is safe -- and (re)starts a timer that removes it after
	// DurationSeconds, timed from GetServerWorldTimeSeconds() per CLAUDE.md §4.5, never a
	// client-ticked countdown.
	void ApplyStatusTag(FGameplayTag Tag, float DurationSeconds);

	// Server-only. Same as ApplyStatusTag, but with no expiry timer at all -- for state that only
	// clears via an explicit RemoveStatusTag call (Build 1, M9: Status.Downed, cleared only by a
	// teammate's revive or a full-party scene reset), not a timed buff like Shield/Fortress.
	void ApplyPersistentStatusTag(FGameplayTag Tag);

	// Server-only. Normally fires from ApplyStatusTag's own expiry timer; exists as a separate
	// callable in case a future ability needs to clear a tag early (e.g. a cleanse), and is how
	// ApplyPersistentStatusTag's tags (which have no timer of their own) are ever cleared.
	void RemoveStatusTag(FGameplayTag Tag);

	// Build 1, M7: cooldown gate for CoopTankAbilities::ApplyShield. Authored server-side only (the
	// Set...() below is only ever called from the ability namespaces), but replicated to the owning
	// client (COND_OwnerOnly, see the field declarations below) so that client's own action bar can
	// draw a WoW-style cooldown sweep.
	float GetShieldCooldownEndServerTime() const { return ShieldCooldownEndServerTime; }
	void SetShieldCooldownEndServerTime(float ServerTime) { ShieldCooldownEndServerTime = ServerTime; }

	// Build 1, M8: same shape, for CoopControlAbilities::ResolveStabilize. Lives here rather than
	// only on Control-role characters because role is a runtime PlayerState value, not a
	// compile-time subclass -- every ACoopCharacter carries every ability's cooldown state, same
	// as ShieldCooldownEndServerTime above.
	float GetStabilizeCooldownEndServerTime() const { return StabilizeCooldownEndServerTime; }
	void SetStabilizeCooldownEndServerTime(float ServerTime) { StabilizeCooldownEndServerTime = ServerTime; }

	// Build 1: same shape as Shield/Stabilize above, one dedicated field per ability rather than a
	// generic per-ability map (established precedent -- every ACoopCharacter carries every
	// ability's cooldown state, since role is a runtime PlayerState value, not a compile-time
	// subclass).
	float GetSpeedCooldownEndServerTime() const { return SpeedCooldownEndServerTime; }
	void SetSpeedCooldownEndServerTime(float ServerTime) { SpeedCooldownEndServerTime = ServerTime; }

	float GetDashCooldownEndServerTime() const { return DashCooldownEndServerTime; }
	void SetDashCooldownEndServerTime(float ServerTime) { DashCooldownEndServerTime = ServerTime; }

	float GetExecutionCooldownEndServerTime() const { return ExecutionCooldownEndServerTime; }
	void SetExecutionCooldownEndServerTime(float ServerTime) { ExecutionCooldownEndServerTime = ServerTime; }

	// Ability kit expansion: same shape as the five above, for Tank's Armor Break
	// (CoopTankAbilities::ResolveArmorBreak) and Damage's Overload (CoopDamageAbilities::ResolveOverload).
	// Every ACoopCharacter carries every ability's cooldown -- role is a runtime PlayerState value,
	// not a compile-time subclass.
	float GetArmorBreakCooldownEndServerTime() const { return ArmorBreakCooldownEndServerTime; }
	void SetArmorBreakCooldownEndServerTime(float ServerTime) { ArmorBreakCooldownEndServerTime = ServerTime; }

	float GetOverloadCooldownEndServerTime() const { return OverloadCooldownEndServerTime; }
	void SetOverloadCooldownEndServerTime(float ServerTime) { OverloadCooldownEndServerTime = ServerTime; }

	// Build 1: one cast-animation Montage reference per ability, set via BP_PlayerCharacter's CDO
	// (same content-wiring pattern as every other EditDefaultsOnly asset reference on this class).
	// Every ACoopCharacter carries all of them regardless of the player's current role, same
	// reasoning as the cooldown fields above -- only the relevant one is ever fetched, by whichever
	// ability namespace function resolves for that role.
	UAnimMontage* GetShieldCastMontage() const { return ShieldCastMontage; }
	UAnimMontage* GetSpeedCastMontage() const { return SpeedCastMontage; }
	UAnimMontage* GetDashCastMontage() const { return DashCastMontage; }
	UAnimMontage* GetStabilizeCastMontage() const { return StabilizeCastMontage; }
	UAnimMontage* GetExecutionCastMontage() const { return ExecutionCastMontage; }
	UAnimMontage* GetArmorBreakCastMontage() const { return ArmorBreakCastMontage; }
	UAnimMontage* GetOverloadCastMontage() const { return OverloadCastMontage; }

	// Server-only. Plays Montage (via ABP_Unarmed's existing "DefaultSlot" AnimGraphNode_Slot, no
	// AnimBP changes needed) on every client's copy of this character, including the server's own
	// listen-host -- ability resolution is server-only and has no reason to predict a cast
	// animation client-side (CLAUDE.md §4.2: this is server-initiated cosmetic feedback, not client
	// prediction). No-ops if Montage is unset, so an ability with no assigned cast animation yet
	// just silently skips this rather than needing a null-check at every call site.
	void PlayCastMontage(UAnimMontage* Montage);

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;

	// Server-only (per Pawn.h: "Only called on the server (or in standalone)"), and this is where
	// Super::PossessedBy sets PlayerState on the pawn. BeginPlay alone is not enough: for a pawn
	// spawned via AGameModeBase::RestartPlayer, BeginPlay fires during SpawnActor, *before*
	// RestartPlayer's later Possess() call sets PlayerState -- so on the server, every pawn except
	// one spawned with PlayerState already valid would stay untinted forever, since OnRep_PlayerState
	// (the other trigger) never fires on the server, only on remote clients receiving replication.
	virtual void PossessedBy(AController* NewController) override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Cosmetic-only, per CLAUDE.md §5/DECISIONS.md: per-player identification is a Dynamic
	// Material Instance colour tint, computed identically on every client from the replicated
	// PlayerId -- no Server RPC needed, this never touches gameplay state.
	void ApplyPlayerColorTint();

	static FLinearColor GetColorForPlayerId(int32 PlayerId);

	// Replicated to every client so the cast animation actually plays for everyone, not just
	// whoever's watching the server -- see PlayCastMontage's comment above.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCastMontage(UAnimMontage* Montage);

	UPROPERTY(VisibleAnywhere, Category = "Health")
	TObjectPtr<UCoopHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Downed")
	TObjectPtr<UCoopDownedComponent> DownedComponent;

	UPROPERTY(VisibleAnywhere, Category = "Downed")
	TObjectPtr<USphereComponent> ReviveTriggerVolume;

	// Build 1: Screen space (EWidgetSpace::Screen), not world space -- ACoopOrbitCamera is spawned
	// per-client, local-only, non-replicated, so there is no single centrally-accessible "the
	// camera" a world-space billboard could reference. Screen space projects the widget at this
	// actor's world position but always draws it screen-facing, satisfying CLAUDE.md §5's
	// "billboarded to camera" with no camera reference needed at all. Known, accepted tradeoff:
	// screen-space widget components don't occlude behind world geometry (will show through walls)
	// -- the "ugly is correct" call for a prototype, same treatment as Shield's own "negates all
	// damage, not just directional" simplification.
	UPROPERTY(VisibleAnywhere, Category = "Status")
	TObjectPtr<UWidgetComponent> StatusBarWidgetComponent;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Status")
	FGameplayTagContainer ActiveStatusTags;

	// Server-only bookkeeping, not replicated -- clients only ever need the tag itself
	// (ActiveStatusTags above), never the exact expiry instant. Kept around (rather than discarded
	// once the timer is set) purely so it's reflection-readable for debugging, per CLAUDE.md §4.3.
	UPROPERTY(VisibleAnywhere, Category = "Status")
	TMap<FGameplayTag, float> StatusTagExpiryServerTime;

	TMap<FGameplayTag, FTimerHandle> StatusTagExpiryTimers;

	// Replicated to the owning client only (COND_OwnerOnly -- see GetLifetimeReplicatedProps) so
	// that client's own action bar (UCoopAbilitySlotWidget) can draw a cooldown sweep. Still
	// authored server-side only, via the Set...() setters called from the ability namespaces -- the
	// owning client receives these read-only and never writes them. Other players have no need for
	// each other's cooldowns, so this stays off the general replication path.
	// VisibleInstanceOnly so the absolute end-time is reflection-readable in a live PIE session
	// (CLAUDE.md §4.3 "state must always be printable"): P9's cooldown-sweep verification needs to
	// read these off a running pawn, and reflection tooling can only see edit/visible UPROPERTYs.
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
	float ShieldCooldownEndServerTime = -1.0f;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
	float StabilizeCooldownEndServerTime = -1.0f;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
	float SpeedCooldownEndServerTime = -1.0f;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
	float DashCooldownEndServerTime = -1.0f;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
	float ExecutionCooldownEndServerTime = -1.0f;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
	float ArmorBreakCooldownEndServerTime = -1.0f;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
	float OverloadCooldownEndServerTime = -1.0f;

	// Build 1: cast-animation Montages, one per ability, set on BP_PlayerCharacter's CDO. Left
	// unset (None) is a valid, supported state -- PlayCastMontage no-ops rather than requiring
	// every ability to have art before it can be gameplay-tested.
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAnimMontage> ShieldCastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAnimMontage> SpeedCastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAnimMontage> DashCastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAnimMontage> StabilizeCastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAnimMontage> ExecutionCastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAnimMontage> ArmorBreakCastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAnimMontage> OverloadCastMontage;

	// Build 1: needed for StatusBarHeightOffsetUnits. Every tunable lives in DA_GameConstants per
	// CLAUDE.md §10 -- set via BP_PlayerCharacter's CDO, same CDO-persistence pattern (and the same
	// gotcha, see UCoopHealthComponent::GameConstants's own comment) as every other class holding
	// an EditDefaultsOnly asset reference.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;
};
