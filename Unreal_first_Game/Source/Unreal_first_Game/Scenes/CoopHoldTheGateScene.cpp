#include "Scenes/CoopHoldTheGateScene.h"
#include "Scenes/CoopPressurePlate.h"
#include "Scenes/CoopMonsterSpawner.h"
#include "Core/CoopGameState.h"
#include "Core/CoopMatchPhase.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopMonsterCharacter.h"
#include "Core/GameConstants.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "EngineUtils.h"

ACoopHoldTheGateScene::ACoopHoldTheGateScene()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ACoopHoldTheGateScene::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	for (TActorIterator<ACoopPressurePlate> It(GetWorld()); It; ++It)
	{
		if (ACoopPressurePlate* Plate = *It)
		{
			Plates.Add(Plate);
			Plate->OnOccupancyChanged.AddDynamic(this, &ACoopHoldTheGateScene::HandlePlateOccupancyChanged);
		}
	}

	const int32 ExpectedPlateCount = GameConstants ? GameConstants->PlateCount : 4;
	if (Plates.Num() != ExpectedPlateCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopHoldTheGateScene::BeginPlay: found %d ACoopPressurePlate actor(s) in the level, expected %d (DA_GameConstants.PlateCount)."), Plates.Num(), ExpectedPlateCount);
	}

	for (TActorIterator<ACoopMonsterSpawner> It(GetWorld()); It; ++It)
	{
		if (ACoopMonsterSpawner* Spawner = *It)
		{
			Spawners.Add(Spawner);
		}
	}

	if (ACoopGameState* CoopGameState = GetWorld()->GetGameState<ACoopGameState>())
	{
		CoopGameState->OnSceneResetRequested.AddDynamic(this, &ACoopHoldTheGateScene::HandleSceneResetRequested);
	}

	GetWorldTimerManager().SetTimer(ScenePhaseCheckTimerHandle, this, &ACoopHoldTheGateScene::CheckPhaseAndStartSceneTimer, ScenePhaseCheckIntervalSeconds, true);
}

void ACoopHoldTheGateScene::CheckPhaseAndStartSceneTimer()
{
	const ACoopGameState* CoopGameState = GetWorld() ? GetWorld()->GetGameState<ACoopGameState>() : nullptr;
	if (!CoopGameState || CoopGameState->GetCurrentPhase() != EMatchPhase::HoldTheGate)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ScenePhaseCheckTimerHandle);

	const float DurationSeconds = GameConstants ? GameConstants->HoldTheGateSceneDurationSeconds : 90.0f;
	GetWorldTimerManager().SetTimer(SceneDurationTimerHandle, this, &ACoopHoldTheGateScene::OnSceneDurationExpired, DurationSeconds, false);
}

bool ACoopHoldTheGateScene::AreAllPlatesOccupied() const
{
	if (Plates.Num() == 0)
	{
		return false;
	}

	for (const ACoopPressurePlate* Plate : Plates)
	{
		if (!Plate || !Plate->IsOccupied())
		{
			return false;
		}
	}
	return true;
}

void ACoopHoldTheGateScene::HandlePlateOccupancyChanged()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AreAllPlatesOccupied())
	{
		// Full occupancy restored (or newly achieved) -- clear any pending fail timer and open the
		// gate if this is the first time all four have been held simultaneously.
		GetWorldTimerManager().ClearTimer(RestoreWindowTimerHandle);
		if (!bGateOpen)
		{
			bGateOpen = true;
		}
		return;
	}

	// Occupancy broke. Only meaningful once the gate has actually been open before -- if it never
	// achieved full occupancy in the first place, there's nothing to "restore" and no timer needed.
	if (bGateOpen && !GetWorldTimerManager().IsTimerActive(RestoreWindowTimerHandle))
	{
		const float RestoreWindowSeconds = GameConstants ? GameConstants->PlateRestoreWindowSeconds : 5.0f;
		GetWorldTimerManager().SetTimer(RestoreWindowTimerHandle, this, &ACoopHoldTheGateScene::OnRestoreWindowExpired, RestoreWindowSeconds, false);
	}
}

void ACoopHoldTheGateScene::OnRestoreWindowExpired()
{
	// Re-validate rather than trust the state from PlateRestoreWindowSeconds ago -- same
	// re-check-at-completion reasoning as UCoopDownedComponent::CompleteRevive().
	if (AreAllPlatesOccupied())
	{
		return;
	}

	bGateOpen = false;

	if (ACoopGameState* CoopGameState = GetWorld()->GetGameState<ACoopGameState>())
	{
		CoopGameState->RequestSceneReset();
	}
}

void ACoopHoldTheGateScene::OnSceneDurationExpired()
{
	if (bGateOpen)
	{
		CompleteScene();
		return;
	}

	// Ran out the clock without the gate held open -- not one of CLAUDE.md §6.6's two named wipe
	// conditions, but not a win either. See this function's header comment for why a reset is the
	// documented fallback here.
	if (ACoopGameState* CoopGameState = GetWorld()->GetGameState<ACoopGameState>())
	{
		CoopGameState->RequestSceneReset();
	}
}

void ACoopHoldTheGateScene::CompleteScene()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SceneDurationTimerHandle);

	UE_LOG(LogTemp, Log, TEXT("ACoopHoldTheGateScene::CompleteScene: Hold the Gate complete -- the party held the gate for the full scene duration."));

	if (ACoopGameState* CoopGameState = GetWorld()->GetGameState<ACoopGameState>())
	{
		CoopGameState->CompleteMatch();
	}

	// Deliberately not stopping the spawners/despawning live monsters on a win -- Build 1 has no
	// scene beyond this one for the party to "proceed through the gate" into, and no Complete-phase
	// UI yet either, so there's nothing downstream that a lingering monster or two would break.
	// Worth revisiting once a real "you won" state exists to transition into.
}

void ACoopHoldTheGateScene::HandleSceneResetRequested()
{
	if (!HasAuthority())
	{
		return;
	}

	ResetScene();
}

void ACoopHoldTheGateScene::ResetScene()
{
	GetWorldTimerManager().ClearTimer(RestoreWindowTimerHandle);
	bGateOpen = false;

	ACoopGameState* CoopGameState = GetWorld()->GetGameState<ACoopGameState>();

	GetWorldTimerManager().ClearTimer(SceneDurationTimerHandle);
	const float DurationSeconds = GameConstants ? GameConstants->HoldTheGateSceneDurationSeconds : 90.0f;
	if (CoopGameState)
	{
		GetWorldTimerManager().SetTimer(SceneDurationTimerHandle, this, &ACoopHoldTheGateScene::OnSceneDurationExpired, DurationSeconds, false);
	}

	// Respawn: teleport every real ACoopCharacter back to one of the level's PlayerStarts (laid
	// out in a line, one per player slot -- same set ACoopGameMode::FillEmptySlotsWithDummies
	// distributes dummies across). This used to teleport everyone to the exact same single
	// PlayerStart and rely on capsule interpenetration resolving itself, but that could leave a
	// character boxed in by the others with no room to separate, unable to move -- distributing
	// across distinct points avoids the pileup instead of hoping physics untangles it. Also moves
	// everyone off the plates, so ACoopPressurePlate's own overlap-driven occupancy naturally
	// clears without this scene needing to poke plate state directly.
	TArray<APlayerStart*> PlayerStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		PlayerStarts.Add(*It);
	}
	PlayerStarts.Sort([](const APlayerStart& A, const APlayerStart& B) { return A.GetActorLocation().X < B.GetActorLocation().X; });

	if (PlayerStarts.Num() > 0)
	{
		int32 CharacterIndex = 0;
		for (TActorIterator<ACoopCharacter> It(GetWorld()); It; ++It)
		{
			if (ACoopCharacter* Character = *It)
			{
				const APlayerStart* SpawnPoint = PlayerStarts[CharacterIndex % PlayerStarts.Num()];
				Character->TeleportTo(SpawnPoint->GetActorLocation(), SpawnPoint->GetActorRotation());
				++CharacterIndex;
			}
		}
	}

	// Clear the room: destroy every live monster rather than leaving a stale wave from before the
	// reset, then restart each spawner's own escalation ramp from "early" again.
	for (TActorIterator<ACoopMonsterCharacter> It(GetWorld()); It; ++It)
	{
		if (ACoopMonsterCharacter* Monster = *It)
		{
			Monster->Destroy();
		}
	}

	for (ACoopMonsterSpawner* Spawner : Spawners)
	{
		if (Spawner)
		{
			Spawner->ResetSpawner();
		}
	}
}

void ACoopHoldTheGateScene::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopHoldTheGateScene, bGateOpen);
}
