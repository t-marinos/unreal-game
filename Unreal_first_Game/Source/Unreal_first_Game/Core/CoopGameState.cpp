#include "Core/CoopGameState.h"
#include "Core/GameConstants.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopDownedComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"

void ACoopGameState::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// CLAUDE.md §4.5: server time only, set once, never accumulated DeltaTime.
	MatchStartServerTime = GetServerWorldTimeSeconds();

	if (GameConstants)
	{
		// CLAUDE.md §4.4: an explicit, known, debuggable replication rate rather than the engine's
		// per-class default.
		SetNetUpdateFrequency(GameConstants->GameStateNetUpdateFrequency);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopGameState has no GameConstants assigned -- NetUpdateFrequency left at its engine default. Assign DA_GameConstants on BP_GameState."));
	}
}

float ACoopGameState::GetElapsedMatchTime() const
{
	if (MatchStartServerTime < 0.0f)
	{
		// Not set yet -- either the server hasn't run BeginPlay, or (on a client) the replicated
		// value hasn't arrived yet. Reporting 0 here (instead of the full world-time-since-start)
		// avoids a brief bogus spike to a large number before replication catches up.
		return 0.0f;
	}
	return FMath::Max(0.0f, GetServerWorldTimeSeconds() - MatchStartServerTime);
}

void ACoopGameState::ToggleButtonPressed()
{
	if (!HasAuthority())
	{
		return;
	}
	bButtonPressed = !bButtonPressed;
}

void ACoopGameState::StartRoleSelectPhase(float DurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentPhase = EMatchPhase::RoleSelect;
	RoleSelectEndServerTime = GetServerWorldTimeSeconds() + DurationSeconds;
}

void ACoopGameState::StartPrepPhase(float DurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentPhase = EMatchPhase::Prep;
	PrepPhaseEndServerTime = GetServerWorldTimeSeconds() + DurationSeconds;
}

void ACoopGameState::StartHoldTheGatePhase()
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentPhase = EMatchPhase::HoldTheGate;
}

void ACoopGameState::IncrementDownedCount()
{
	if (!HasAuthority())
	{
		return;
	}
	++DownedPlayerCount;

	if (IsPartyWiped())
	{
		RequestSceneReset();
	}
}

void ACoopGameState::CompleteMatch()
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentPhase = EMatchPhase::Complete;
}

void ACoopGameState::DecrementDownedCount()
{
	if (!HasAuthority())
	{
		return;
	}
	DownedPlayerCount = FMath::Max(0, DownedPlayerCount - 1);
}

void ACoopGameState::RequestSceneReset()
{
	if (!HasAuthority())
	{
		return;
	}

	for (TActorIterator<ACoopCharacter> It(GetWorld()); It; ++It)
	{
		if (ACoopCharacter* Character = *It)
		{
			if (UCoopDownedComponent* Downed = Character->GetDownedComponent())
			{
				Downed->ForceRevive();
			}
		}
	}

	OnSceneResetRequested.Broadcast();
}

void ACoopGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopGameState, MatchStartServerTime);
	DOREPLIFETIME(ACoopGameState, bButtonPressed);
	DOREPLIFETIME(ACoopGameState, CurrentPhase);
	DOREPLIFETIME(ACoopGameState, RoleSelectEndServerTime);
	DOREPLIFETIME(ACoopGameState, PrepPhaseEndServerTime);
	DOREPLIFETIME(ACoopGameState, DownedPlayerCount);
}
