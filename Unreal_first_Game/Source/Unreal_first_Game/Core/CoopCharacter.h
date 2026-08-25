#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
	// Cosmetic-only, per CLAUDE.md §5/DECISIONS.md: per-player identification is a Dynamic
	// Material Instance colour tint, computed identically on every client from the replicated
	// PlayerId -- no Server RPC needed, this never touches gameplay state.
	void ApplyPlayerColorTint();

	static FLinearColor GetColorForPlayerId(int32 PlayerId);

	UPROPERTY(VisibleAnywhere, Category = "Health")
	TObjectPtr<UCoopHealthComponent> HealthComponent;
};
