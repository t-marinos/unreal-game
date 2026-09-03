#include "Scenes/CoopMonsterSpawner.h"
#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopRoleTypes.h"
#include "Core/CoopGameState.h"
#include "Core/CoopMatchPhase.h"
#include "Core/GameConstants.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"
#include "EngineUtils.h"

ACoopMonsterSpawner::ACoopMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	// Purely a server-side spawn point -- clients never need to know about the spawner itself, only
	// the ACoopMonsterCharacter actors it spawns (which replicate on their own).
	bReplicates = false;
}

void ACoopMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(RosterCheckTimerHandle, this, &ACoopMonsterSpawner::CheckRosterAndStartSpawning, RosterCheckIntervalSeconds, true);
}

void ACoopMonsterSpawner::CheckRosterAndStartSpawning()
{
	const ACoopGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ACoopGameState>() : nullptr;
	if (!GameState)
	{
		return;
	}

	const EMatchPhase Phase = GameState->GetCurrentPhase();
	if (Phase == EMatchPhase::WaitingForRoster || Phase == EMatchPhase::RoleSelect)
	{
		// Roles aren't resolved yet -- keep polling rather than spawning into a candidate pool where
		// every PlayerState still reads Unassigned.
		return;
	}

	GetWorldTimerManager().ClearTimer(RosterCheckTimerHandle);
	SpawnerStartServerTime = GameState->GetServerWorldTimeSeconds();
	ScheduleNextSpawn();
}

TArray<AActor*> ACoopMonsterSpawner::GatherNonTankCandidates() const
{
	TArray<AActor*> Candidates;
	for (TActorIterator<ACoopCharacter> It(GetWorld()); It; ++It)
	{
		ACoopCharacter* Character = *It;
		const ACoopPlayerState* PlayerState = Character ? Character->GetPlayerState<ACoopPlayerState>() : nullptr;
		if (PlayerState && PlayerState->GetRole() != EPlayerRole::Tank)
		{
			Candidates.Add(Character);
		}
	}
	return Candidates;
}

float ACoopMonsterSpawner::ComputeCurrentSpawnInterval() const
{
	const float EarlySeconds = GameConstants ? GameConstants->MonsterSpawnIntervalEarlySeconds : 6.0f;
	const float LateSeconds = GameConstants ? GameConstants->MonsterSpawnIntervalLateSeconds : 2.5f;
	const float RampSeconds = GameConstants ? GameConstants->HoldTheGateSceneDurationSeconds : 90.0f;

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float ElapsedSeconds = GameState ? (GameState->GetServerWorldTimeSeconds() - SpawnerStartServerTime) : 0.0f;
	const float Alpha = FMath::Clamp(ElapsedSeconds / RampSeconds, 0.0f, 1.0f);

	return FMath::Lerp(EarlySeconds, LateSeconds, Alpha);
}

void ACoopMonsterSpawner::ResetSpawner()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	SpawnerStartServerTime = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
	ScheduleNextSpawn();
}

void ACoopMonsterSpawner::ScheduleNextSpawn()
{
	const float IntervalSeconds = ComputeCurrentSpawnInterval();
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ACoopMonsterSpawner::SpawnMonster, IntervalSeconds, false);
}

void ACoopMonsterSpawner::SpawnMonster()
{
	if (MonsterClass && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (ACoopMonsterCharacter* Monster = GetWorld()->SpawnActor<ACoopMonsterCharacter>(MonsterClass, GetActorLocation(), GetActorRotation(), SpawnParams))
		{
			Monster->InitializeMonster(GatherNonTankCandidates());
		}
	}

	// Re-schedule regardless of whether MonsterClass is set, so a content-wiring mistake (forgetting
	// to set MonsterClass on BP_MonsterSpawner) doesn't also silently stop the timer forever --
	// makes the bug visible (no monsters spawn) rather than compounding it (spawner goes fully dead).
	ScheduleNextSpawn();
}
