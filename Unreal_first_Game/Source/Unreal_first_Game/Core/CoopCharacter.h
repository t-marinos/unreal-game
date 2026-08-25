#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "CoopCharacter.generated.h"

class UCoopHealthComponent;

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

	// Server-only. Normally fires from ApplyStatusTag's own expiry timer; exists as a separate
	// callable in case a future ability needs to clear a tag early (e.g. a cleanse).
	void RemoveStatusTag(FGameplayTag Tag);

	// Build 1, M7: server-only cooldown gate for CoopTankAbilities::ApplyShield. Not replicated --
	// Build 1's ability cards have no cooldown-remaining display yet, so no client needs to see this.
	float GetShieldCooldownEndServerTime() const { return ShieldCooldownEndServerTime; }
	void SetShieldCooldownEndServerTime(float ServerTime) { ShieldCooldownEndServerTime = ServerTime; }

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

	UPROPERTY(VisibleAnywhere, Category = "Health")
	TObjectPtr<UCoopHealthComponent> HealthComponent;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Status")
	FGameplayTagContainer ActiveStatusTags;

	// Server-only bookkeeping, not replicated -- clients only ever need the tag itself
	// (ActiveStatusTags above), never the exact expiry instant. Kept around (rather than discarded
	// once the timer is set) purely so it's reflection-readable for debugging, per CLAUDE.md §4.3.
	UPROPERTY(VisibleAnywhere, Category = "Status")
	TMap<FGameplayTag, float> StatusTagExpiryServerTime;

	TMap<FGameplayTag, FTimerHandle> StatusTagExpiryTimers;

	float ShieldCooldownEndServerTime = -1.0f;
};
