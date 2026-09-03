#include "Core/CoopGameMode.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopGameState.h"
#include "Core/GameConstants.h"
#include "Dev/DummyAIController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"

ACoopGameMode::ACoopGameMode()
{
	// Wire the GameMode's own class defaults to the Coop skeletons now, ahead of any of
	// them having real logic, so the pattern is established early. DefaultPawnClass,
	// PlayerControllerClass, and GameStateClass are deliberately left untouched here --
	// all three need a Blueprint wrapper (BP_PlayerCharacter since M4, BP_PlayerController
	// since M5, BP_GameState since M6) so a UPROPERTY asset reference on them (mesh/
	// animation, GameConstants) can be set via content wiring instead of hardcoded in C++.
	// Until something points the project's GlobalDefaultGameMode at this class (M3), none
	// of this is live in PIE.
	PlayerStateClass = ACoopPlayerState::StaticClass();
}

void ACoopGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	int32 MaxPlayers = FallbackMaxPlayers;
	if (GameConstants)
	{
		MaxPlayers = GameConstants->MaxPlayers;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopGameMode has no GameConstants assigned -- falling back to a hardcoded MaxPlayers of %d. Assign DA_GameConstants on BP_GameMode."), FallbackMaxPlayers);
	}

	// Checked and incremented together against our own counter, not GameState->PlayerArray.Num()
	// (which only grows later, in PostLogin/InitNewPlayer) -- see the AcceptedPlayerCount comment
	// in the header for why that distinction matters. Don't overwrite an earlier rejection reason
	// if one is already set.
	if (ErrorMessage.IsEmpty() && AcceptedPlayerCount >= MaxPlayers)
	{
		ErrorMessage = FString::Printf(TEXT("Session is full (%d/%d players)."), AcceptedPlayerCount, MaxPlayers);
	}

	if (ErrorMessage.IsEmpty())
	{
		++AcceptedPlayerCount;
	}
}

void ACoopGameMode::Logout(AController* Exiting)
{
	if (Cast<APlayerController>(Exiting))
	{
		AcceptedPlayerCount = FMath::Max(0, AcceptedPlayerCount - 1);
	}
	Super::Logout(Exiting);
}

void ACoopGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 MaxPlayers = FallbackMaxPlayers;
	if (GameConstants)
	{
		MaxPlayers = GameConstants->MaxPlayers;
	}

	// Unlike PreLogin (which only reserves a connection slot), GameState->PlayerArray is
	// guaranteed populated by this point -- Super::PostLogin runs InitPlayerState internally
	// before this line executes.
	if (GameState && GameState->PlayerArray.Num() >= MaxPlayers)
	{
		OnRosterComplete();
	}
}

void ACoopGameMode::BeginPlay()
{
	Super::BeginPlay();

	// -devmode lets a packaged Development build (CLAUDE.md §3.1) opt in without an Editor Class
	// Defaults edit; bDevMode can also just be checked directly on BP_GameMode for PIE testing.
	if (FParse::Param(FCommandLine::Get(), TEXT("devmode")))
	{
		bDevMode = true;
	}

	if (bDevMode)
	{
		FillEmptySlotsWithDummies();
	}
}

void ACoopGameMode::FillEmptySlotsWithDummies()
{
	int32 MaxPlayers = FallbackMaxPlayers;
	if (GameConstants)
	{
		MaxPlayers = GameConstants->MaxPlayers;
	}

	const int32 CurrentPlayers = GameState ? GameState->PlayerArray.Num() : 0;
	const int32 DummiesToSpawn = FMath::Max(0, MaxPlayers - CurrentPlayers);

	// Every dummy used to spawn at the exact same FindPlayerStart(nullptr) point and rely on
	// AdjustIfPossibleButAlwaysSpawn to nudge overlapping ones apart -- with several capsules
	// piling up at one spot, that nudge could leave one boxed in by the others with nowhere to
	// go. Spawning each dummy at its own PlayerStart (the level now has one per player slot,
	// laid out in a line) avoids the pileup entirely. Sorted by X so index order matches their
	// left-to-right placement, not spawn order.
	TArray<APlayerStart*> PlayerStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		PlayerStarts.Add(*It);
	}
	PlayerStarts.Sort([](const APlayerStart& A, const APlayerStart& B) { return A.GetActorLocation().X < B.GetActorLocation().X; });

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < DummiesToSpawn; ++Index)
	{
		const FTransform SpawnTransform = PlayerStarts.Num() > 0
			? PlayerStarts[Index % PlayerStarts.Num()]->GetActorTransform()
			: FTransform::Identity;
		APawn* DummyPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, SpawnTransform, SpawnParams);
		ADummyAIController* DummyController = GetWorld()->SpawnActor<ADummyAIController>();
		if (DummyPawn && DummyController)
		{
			DummyController->Possess(DummyPawn);
			DummyController->SetBehavior(EDummyBehavior::Idle);
			++SpawnedCount;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FillEmptySlotsWithDummies: failed to spawn dummy %d/%d."), Index + 1, DummiesToSpawn);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("FillEmptySlotsWithDummies: spawned %d/%d dummies (MaxPlayers=%d, CurrentPlayers=%d)."), SpawnedCount, DummiesToSpawn, MaxPlayers, CurrentPlayers);

	// M3: dev mode can reach a full roster in this one BeginPlay-time burst, with no PostLogin call
	// for the slots dummies just filled -- check here too so both paths converge on one fire-once
	// OnRosterComplete.
	if (GameState && GameState->PlayerArray.Num() >= MaxPlayers)
	{
		OnRosterComplete();
	}
}

void ACoopGameMode::OnRosterComplete()
{
	if (bRosterComplete)
	{
		return;
	}
	bRosterComplete = true;

	float RoleSelectDuration = FallbackRoleSelectDurationSeconds;
	if (GameConstants)
	{
		RoleSelectDuration = GameConstants->RoleSelectDurationSeconds;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopGameMode has no GameConstants assigned -- falling back to a hardcoded RoleSelectDurationSeconds of %.1f."), FallbackRoleSelectDurationSeconds);
	}

	GetWorldTimerManager().SetTimer(RoleSelectTimerHandle, this, &ACoopGameMode::ResolveRoleSelection, RoleSelectDuration, false);

	if (ACoopGameState* CoopGameState = Cast<ACoopGameState>(GameState))
	{
		CoopGameState->StartRoleSelectPhase(RoleSelectDuration);
	}

	UE_LOG(LogTemp, Log, TEXT("OnRosterComplete: roster is full -- RoleSelect phase started (%.1fs to choose, or resolves early once every real player has claimed)."), RoleSelectDuration);
}

void ACoopGameMode::TryClaimRole(ACoopPlayerState* RequestingPlayerState, EPlayerRole DesiredRole)
{
	if (!RequestingPlayerState || DesiredRole == EPlayerRole::Unassigned || !GameState || bRoleSelectionResolved)
	{
		return;
	}

	// Reject silently if anyone else already holds this role -- matches the "intent that might be
	// rejected, RPC caller gets no error, just no state change" shape PreLogin's cap check already
	// uses. First to claim a still-unclaimed role wins (DECISIONS.md's "Role assignment is
	// player-chosen, not random").
	for (const TObjectPtr<APlayerState>& PS : GameState->PlayerArray)
	{
		const ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(PS);
		if (CoopPS && CoopPS != RequestingPlayerState && CoopPS->GetRole() == DesiredRole)
		{
			return;
		}
	}

	RequestingPlayerState->SetRole(DesiredRole);
	UE_LOG(LogTemp, Log, TEXT("TryClaimRole: %s claimed role %d."), *RequestingPlayerState->GetPlayerName(), static_cast<int32>(DesiredRole));

	CheckAllRealPlayersClaimed();
}

void ACoopGameMode::CheckAllRealPlayersClaimed()
{
	if (bRoleSelectionResolved || !GetWorld())
	{
		return;
	}

	int32 RealPlayerCount = 0;
	int32 RealPlayerClaimedCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const ACoopPlayerState* CoopPS = PC ? PC->GetPlayerState<ACoopPlayerState>() : nullptr;
		if (!CoopPS)
		{
			continue;
		}

		++RealPlayerCount;
		if (CoopPS->GetRole() != EPlayerRole::Unassigned)
		{
			++RealPlayerClaimedCount;
		}
	}

	// Dev-mode dummies deliberately excluded (the player controller iterator only ever yields real
	// APlayerControllers) -- ResolveRoleSelection() is what gives dummies a role, not this check.
	if (RealPlayerCount > 0 && RealPlayerClaimedCount == RealPlayerCount)
	{
		ResolveRoleSelection();
	}
}

void ACoopGameMode::ResolveRoleSelection()
{
	if (bRoleSelectionResolved || !GameState)
	{
		return;
	}
	bRoleSelectionResolved = true;
	GetWorldTimerManager().ClearTimer(RoleSelectTimerHandle);

	static const TArray<EPlayerRole> AllRoles = {
		EPlayerRole::Tank, EPlayerRole::Support, EPlayerRole::Runner, EPlayerRole::Control, EPlayerRole::Damage
	};

	TArray<EPlayerRole> RemainingRoles = AllRoles;
	TArray<ACoopPlayerState*> UnassignedPlayers;

	for (const TObjectPtr<APlayerState>& PS : GameState->PlayerArray)
	{
		ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(PS);
		if (!CoopPS)
		{
			continue;
		}

		if (CoopPS->GetRole() == EPlayerRole::Unassigned)
		{
			UnassignedPlayers.Add(CoopPS);
		}
		else
		{
			RemainingRoles.Remove(CoopPS->GetRole());
		}
	}

	// Random fallback assignment (DECISIONS.md's "Role assignment is player-chosen, not random"):
	// covers an AFK real player who never claimed in time, and every dev-mode dummy, which never
	// claims at all.
	for (ACoopPlayerState* CoopPS : UnassignedPlayers)
	{
		if (RemainingRoles.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolveRoleSelection: ran out of roles to auto-assign -- more Unassigned PlayerStates than remaining roles, should never happen at MaxPlayers=5 with 5 roles."));
			break;
		}
		const int32 RandomIndex = FMath::RandRange(0, RemainingRoles.Num() - 1);
		CoopPS->SetRole(RemainingRoles[RandomIndex]);
		RemainingRoles.RemoveAt(RandomIndex);
	}

	UE_LOG(LogTemp, Log, TEXT("ResolveRoleSelection: role selection resolved (%d player(s) auto-assigned)."), UnassignedPlayers.Num());

	float PrepDuration = FallbackPrepArenaDurationSeconds;
	if (GameConstants)
	{
		PrepDuration = GameConstants->PrepArenaDurationSeconds;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopGameMode has no GameConstants assigned -- falling back to a hardcoded PrepArenaDurationSeconds of %.1f."), FallbackPrepArenaDurationSeconds);
	}

	GetWorldTimerManager().SetTimer(PrepPhaseTimerHandle, this, &ACoopGameMode::OnPrepPhaseExpired, PrepDuration, false);

	if (ACoopGameState* CoopGameState = Cast<ACoopGameState>(GameState))
	{
		CoopGameState->StartPrepPhase(PrepDuration);
	}

	UE_LOG(LogTemp, Log, TEXT("ResolveRoleSelection: Prep phase started (%.1fs)."), PrepDuration);
}

void ACoopGameMode::OnPrepPhaseExpired()
{
	if (ACoopGameState* CoopGameState = Cast<ACoopGameState>(GameState))
	{
		CoopGameState->StartHoldTheGatePhase();
	}

	// M10 gives this phase real scene setup (plates, gate, spawner). Only the phase value itself
	// exists this early, per CLAUDE.md §1/§4.8 -- don't build ahead of what a milestone needs.
	UE_LOG(LogTemp, Log, TEXT("OnPrepPhaseExpired: Prep phase ended -- HoldTheGate phase started (scene setup lands in M10)."));
}
