#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopMonsterAIController.h"
#include "Core/CoopHealthComponent.h"
#include "Core/CoopFixateRetargetComponent.h"
#include "Core/CoopDownedComponent.h"
#include "Core/CoopCharacter.h"
#include "Core/GameConstants.h"
#include "Scenes/CoopMonsterStrikeTelegraph.h"
#include "Tags/CoopGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

ACoopMonsterCharacter::ACoopMonsterCharacter()
{
	// The pawn itself doesn't tick -- ACoopMonsterAIController ticks and feeds AddMovementInput;
	// the inherited CharacterMovementComponent ticks itself to consume that input.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Capsule sized to the stock Mannequin (matches BP_PlayerCharacter). The capsule BLOCKS pawns
	// by default -- that is deliberate: it's how the Tank body-blocks a monster off its target
	// with no extra code (MONSTER_ENEMIES_PROGRESS.md, docs/scenes/HOLD_THE_GATE.md).
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	// The skeletal mesh + AnimBP (ABP_Unarmed, same as players/dummies) + the Z=-89/yaw=270
	// capsule offset are wired on BP_MonsterCharacter, mirroring BP_PlayerCharacter -- same C++/BP
	// split as every other character in this project (CLAUDE.md §3.2). Nothing to set here.

	// Straight-line steering (ACoopMonsterAIController) plus "face where you walk".
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;  // overridden from GameConstants in InitializeMonster
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	// The engine spawns + possesses this controller during SpawnActor, before the spawner's
	// InitializeMonster() call. Server-only (AI controllers never exist on clients).
	AIControllerClass = ACoopMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	HealthComponent = CreateDefaultSubobject<UCoopHealthComponent>(TEXT("HealthComponent"));
	TargetingComponent = CreateDefaultSubobject<UCoopFixateRetargetComponent>(TEXT("TargetingComponent"));
}

void ACoopMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Cursor-targeting: the reparent AActor -> ACharacter dropped the cone's BlockAllDynamic static
	// mesh, so the inherited Pawn-profile capsule now ignores ECC_Visibility. Without this,
	// ACoopPlayerController::SelectTargetUnderCursor's Visibility trace passes straight through a
	// monster and it can't be click-selected -- which the target-required abilities (Armor Break,
	// Execution, Overload) need. Same one-liner ACoopCharacter::BeginPlay applies for players; done
	// in BeginPlay for the same reason (a BP-serialized override on the inherited component would
	// otherwise shadow a constructor setting).
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Dark red tint to read as a threat, distinct from the players' per-role tints. The Mannequin
	// materials expose "Paint Tint" (not "Color") -- same param ACoopCharacter::ApplyPlayerColorTint
	// uses -- and there are two slots (body + extras), so tint every slot for a consistent look.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const int32 NumMaterials = MeshComp->GetNumMaterials();
		for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
		{
			if (UMaterialInstanceDynamic* DynamicMaterial = MeshComp->CreateAndSetMaterialInstanceDynamic(SlotIndex))
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("Paint Tint"), FLinearColor(0.55f, 0.03f, 0.03f));
			}
		}
	}

	if (HasAuthority())
	{
		HealthComponent->OnHealthDepleted.AddDynamic(this, &ACoopMonsterCharacter::HandleHealthDepleted);
	}
}

void ACoopMonsterCharacter::InitializeMonster(const TArray<AActor*>& InitialCandidates)
{
	if (!HasAuthority())
	{
		return;
	}

	const float MaxHealth = GameConstants ? GameConstants->MonsterHealth : 50.0f;
	HealthComponent->SetMaxHealth(MaxHealth);

	// Walk speed toward the fixate target. Overrides the constructor default with the tunable
	// (CLAUDE.md §10); a bit below player run speed so Tank/Runner can outpace them.
	if (GameConstants)
	{
		GetCharacterMovement()->MaxWalkSpeed = GameConstants->MonsterMoveSpeed;
	}

	TargetingComponent->PickInitialTarget(InitialCandidates);
	BindToTargetDownedDelegate();

	const float AttackIntervalSeconds = GameConstants ? GameConstants->MonsterAttackIntervalSeconds : 2.0f;
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ACoopMonsterCharacter::PerformAttackTick, AttackIntervalSeconds, true);
}

float ACoopMonsterCharacter::GetMeleeRangeUnits() const
{
	return GameConstants ? GameConstants->MonsterMeleeRangeUnits : 160.0f;
}

void ACoopMonsterCharacter::Destroyed()
{
	// MONSTER_ENEMIES_PROGRESS.md Phase B: a monster destroyed mid-windup (killed via
	// HandleHealthDepleted, or swept by CoopHoldTheGateScene::ResetScene calling Destroy() directly)
	// must take its pending strike timer and its ground-ring telegraph with it -- otherwise a
	// stuck red decal is left on the floor.
	GetWorldTimerManager().ClearTimer(WindupTimerHandle);
	if (AActor* Telegraph = ActiveTelegraph.Get())
	{
		Telegraph->Destroy();
	}
	ActiveTelegraph = nullptr;

	// Take the possessing ACoopMonsterAIController with us -- otherwise every dead monster leaks a
	// pawn-less controller (monsters die often; the dev-mode dummies that share the DummyAIController
	// pattern don't, which is why that path never needed this).
	if (AController* MyController = GetController())
	{
		MyController->Destroy();
	}

	Super::Destroyed();
}

void ACoopMonsterCharacter::BindToTargetDownedDelegate()
{
	ACoopCharacter* TargetCharacter = Cast<ACoopCharacter>(TargetingComponent->GetCurrentTarget());
	if (!TargetCharacter)
	{
		return;
	}

	if (UCoopDownedComponent* Downed = TargetCharacter->GetDownedComponent())
	{
		Downed->OnDownedStateChanged.AddDynamic(this, &ACoopMonsterCharacter::HandleTargetDownedStateChanged);
		BoundTargetForDelegate = TargetCharacter;
	}
}

void ACoopMonsterCharacter::UnbindFromTargetDownedDelegate()
{
	if (ACoopCharacter* PreviouslyBound = BoundTargetForDelegate.Get())
	{
		if (UCoopDownedComponent* Downed = PreviouslyBound->GetDownedComponent())
		{
			Downed->OnDownedStateChanged.RemoveDynamic(this, &ACoopMonsterCharacter::HandleTargetDownedStateChanged);
		}
	}
	BoundTargetForDelegate = nullptr;
}

void ACoopMonsterCharacter::HandleTargetDownedStateChanged()
{
	// OnDownedStateChanged fires on both entering AND leaving Downed -- only entering Downed should
	// trigger a retarget; a revive completing is not this monster's concern.
	ACoopCharacter* CurrentTargetCharacter = BoundTargetForDelegate.Get();
	if (!CurrentTargetCharacter || !CurrentTargetCharacter->GetDownedComponent()->IsDowned())
	{
		return;
	}

	UnbindFromTargetDownedDelegate();

	// A brief pause before actually re-fixating (GameConstants->MonsterFixateSwitchDelaySeconds) --
	// a readable "it hesitates, then locks onto someone else" beat rather than an instant retarget.
	const float SwitchDelaySeconds = GameConstants ? GameConstants->MonsterFixateSwitchDelaySeconds : 0.5f;
	GetWorldTimerManager().SetTimer(RetargetDelayTimerHandle, this, &ACoopMonsterCharacter::PerformRetarget, SwitchDelaySeconds, false);
}

void ACoopMonsterCharacter::PerformRetarget()
{
	TargetingComponent->OnTargetDowned();
	BindToTargetDownedDelegate();
}

void ACoopMonsterCharacter::PerformAttackTick()
{
	if (!HasAuthority())
	{
		return;
	}

	// A windup is already telegraphing -- let WindupTimerHandle -> PerformStrike finish it before
	// starting another. (bWindingUp is cleared by PerformStrike, HandleHealthDepleted, Destroyed.)
	if (bWindingUp)
	{
		return;
	}

	ACoopCharacter* TargetCharacter = Cast<ACoopCharacter>(TargetingComponent->GetCurrentTarget());
	if (!TargetCharacter || !TargetCharacter->GetDownedComponent() || TargetCharacter->GetDownedComponent()->IsDowned())
	{
		// No valid target right now (e.g. everyone currently Downed) -- skip this tick, try again
		// next interval rather than forcing an immediate retarget search here.
		return;
	}

	// MONSTER_ENEMIES_PROGRESS.md Phase A: the attack is contact-range. Out of melee range ->
	// ACoopMonsterAIController is still walking the monster closer; skip this tick. (The Tank
	// body-blocking the monster with their capsule is what keeps this check failing -- the monster
	// physically can't get within GetMeleeRangeUnits() of its target while the Tank is in the lane.)
	const float RangeSq = FMath::Square(GetMeleeRangeUnits());
	if (FVector::DistSquared(GetActorLocation(), TargetCharacter->GetActorLocation()) > RangeSq)
	{
		return;
	}

	// MONSTER_ENEMIES_PROGRESS.md Phase B: a passing tick starts a telegraphed WINDUP, not an
	// instant hit. Spawn a flat red ground ring at the target's feet, then PerformStrike resolves
	// MonsterAttackWindupSeconds later (re-checking range). The window is what body-blocking and
	// Shield-shove exploit -- move the monster out of melee before the strike and it whiffs.
	bWindingUp = true;

	if (StrikeTelegraphClass && GetWorld())
	{
		// Drop the ring to roughly the target's feet so it reads as a ground decal -- same ~88-unit
		// (stock Mannequin capsule half-height) offset basis as ACoopTargetRing / the status badge.
		FVector TelegraphLocation = TargetCharacter->GetActorLocation();
		TelegraphLocation.Z -= 88.0f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AActor* Telegraph = GetWorld()->SpawnActor<AActor>(StrikeTelegraphClass, TelegraphLocation, FRotator::ZeroRotator, SpawnParams))
		{
			if (ACoopMonsterStrikeTelegraph* StrikeTelegraph = Cast<ACoopMonsterStrikeTelegraph>(Telegraph))
			{
				StrikeTelegraph->Initialize(GetMeleeRangeUnits());
			}
			ActiveTelegraph = Telegraph;
		}
	}

	const float WindupSeconds = GameConstants ? GameConstants->MonsterAttackWindupSeconds : 1.0f;
	GetWorldTimerManager().SetTimer(WindupTimerHandle, this, &ACoopMonsterCharacter::PerformStrike, WindupSeconds, false);
}

void ACoopMonsterCharacter::PerformStrike()
{
	// Windup resolved -- clear the flag and the telegraph regardless of whether the hit lands. Only
	// ever called via WindupTimerHandle, which is armed only inside the HasAuthority() branch of
	// PerformAttackTick, so this always runs server-side.
	bWindingUp = false;
	if (AActor* Telegraph = ActiveTelegraph.Get())
	{
		Telegraph->Destroy();
	}
	ActiveTelegraph = nullptr;

	if (!HasAuthority())
	{
		return;
	}

	ACoopCharacter* TargetCharacter = Cast<ACoopCharacter>(TargetingComponent->GetCurrentTarget());
	if (!TargetCharacter || !TargetCharacter->GetDownedComponent() || TargetCharacter->GetDownedComponent()->IsDowned())
	{
		return;
	}

	// Re-check melee range at strike time: if the target or the monster moved out during the windup
	// -- a body-block, a Shield-shove, the target just walking away -- the strike whiffs.
	const float RangeSq = FMath::Square(GetMeleeRangeUnits());
	const FVector MonsterLocation = GetActorLocation();
	const FVector TargetLocation = TargetCharacter->GetActorLocation();
	if (FVector::DistSquared(MonsterLocation, TargetLocation) > RangeSq)
	{
		return;
	}

	// Direct damage -- no projectile actor. Tank's Shield/Fortress negate this via HasStatusTag
	// checks inside UCoopHealthComponent::ApplyDamage.
	const float AttackDamage = GameConstants ? GameConstants->MonsterAttackDamage : 5.0f;
	if (UCoopHealthComponent* TargetHealth = TargetCharacter->GetHealthComponent())
	{
		TargetHealth->ApplyDamage(AttackDamage);
	}

	// MONSTER_ENEMIES_PROGRESS.md Phase B: knockback, directly away from the monster in the XY
	// plane. Tuned strong enough to shove a plate-holder off their plate -> ACoopPressurePlate's own
	// overlap logic fires -> the gate closes (no plate code changes). Status.Fortress resists a
	// fraction of it (FortressKnockbackResistPercent -- first consumer). Status.Shielded negated the
	// damage above but deliberately does NOT resist knockback, keeping a real Shield -> Fortress
	// distinction. bXYOverride = true replaces horizontal velocity; bZOverride = false leaves gravity.
	FVector KnockbackDir = (TargetLocation - MonsterLocation).GetSafeNormal2D();
	if (KnockbackDir.IsNearlyZero())
	{
		// Monster and target on the exact same XY -- shove along the monster's facing instead.
		KnockbackDir = GetActorForwardVector().GetSafeNormal2D();
	}

	float Impulse = GameConstants ? GameConstants->MonsterKnockbackImpulse : 650.0f;
	if (GameConstants && TargetCharacter->HasStatusTag(CoopGameplayTags::Status_Fortress))
	{
		Impulse *= (1.0f - GameConstants->FortressKnockbackResistPercent);
	}

	TargetCharacter->LaunchCharacter(KnockbackDir * Impulse, true, false);
}

void ACoopMonsterCharacter::ApplyStatusTag(FGameplayTag Tag, float DurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveStatusTags.AddTag(Tag);

	// Reapplying before expiry refreshes the timer rather than stacking a second one -- same
	// pattern as ACoopCharacter::ApplyStatusTag.
	FTimerHandle& Handle = StatusTagExpiryTimers.FindOrAdd(Tag);
	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &ACoopMonsterCharacter::RemoveStatusTag, Tag);
	GetWorldTimerManager().SetTimer(Handle, Delegate, DurationSeconds, false);
}

void ACoopMonsterCharacter::RemoveStatusTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveStatusTags.RemoveTag(Tag);
	StatusTagExpiryTimers.Remove(Tag);
}

void ACoopMonsterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopMonsterCharacter, ActiveStatusTags);
}

void ACoopMonsterCharacter::HandleHealthDepleted()
{
	// M11 scope: monster death just means it stops attacking and vanishes -- no scoring/loot/XP
	// (CLAUDE.md §8, out of scope). Unbind first so a dangling delegate can't fire into an
	// about-to-be-destroyed monster.
	UnbindFromTargetDownedDelegate();
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(RetargetDelayTimerHandle);
	GetWorldTimerManager().ClearTimer(WindupTimerHandle);
	Destroy();
}
