#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopMonsterStrikeTelegraph.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

// MONSTER_ENEMIES_PROGRESS.md Phase B. A flat red ground ring an ACoopMonsterCharacter spawns at
// its target's feet during a melee-strike windup (MonsterAttackWindupSeconds), then destroys when
// the strike resolves. CLAUDE.md §5's "a coloured ring on the ground ... is a spell effect" and
// "telegraphs are flat decals on the ground" -- a flat unlit engine Plane + M_TargetRing, no VFX.
//
// The shape mirrors ACoopTargetRing's constructor MINUS its per-player cursor Tick and local-only
// flag: this one is SERVER-SPAWNED and bReplicates = true so all five clients see the same tell at
// the same instant (CLAUDE.md §4.5). It holds no gameplay state -- "about to be hit" is transient
// AI state tracked by the monster's own bWindingUp bool, not an FGameplayTag, so no
// docs/abilities.md tag change. Position is set once at spawn and never moves (no Tick): if the
// target walks off it during the windup, PerformStrike's own range re-check whiffs.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopMonsterStrikeTelegraph : public AActor
{
	GENERATED_BODY()

public:
	ACoopMonsterStrikeTelegraph();

	// Called once by ACoopMonsterCharacter right after spawning this, on the server. RadiusUnits
	// scales the plane mesh to match the monster's melee reach; the ring tints itself red via
	// M_TargetRing's "Color" param. Replicates the radius so remote clients scale it in BeginPlay.
	void Initialize(float RadiusUnits);

protected:
	virtual void BeginPlay() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Scale the plane from TelegraphRadius and force the ring's M_TargetRing MID to red. Called from
	// both BeginPlay (every machine) and Initialize (the server, after BeginPlay has already run) --
	// idempotent, so the double-call on a listen-server host is harmless.
	void ApplyTelegraphVisual();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RingMesh;

	// M_TargetRing (the same ring material ACoopTargetRing uses). Left unset is valid -- the plane
	// just renders its default material until M_TargetRing is wired on BP_MonsterStrikeTelegraph's
	// CDO, same content-wiring pattern as every other asset reference in this project.
	UPROPERTY(EditDefaultsOnly, Category = "Telegraph")
	TObjectPtr<UMaterialInterface> RingMaterial;

	// Set by Initialize() on the server; replicated so clients can scale the plane in BeginPlay.
	UPROPERTY(Replicated)
	float TelegraphRadius = 0.0f;
};
