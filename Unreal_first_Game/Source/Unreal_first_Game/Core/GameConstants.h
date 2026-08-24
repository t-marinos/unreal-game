#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameConstants.generated.h"

// Every tunable number for Build 0 lives here (CLAUDE.md §10) -- editable in the Unreal Editor
// or via unreal-mcp's data asset tools, without touching code. The actual instance is
// Content/Data/DA_GameConstants, referenced by BP_GameMode. Grows as later milestones add more
// tunables (movement speeds, cooldowns, arena dimensions, ...); nothing here is load-bearing
// until something references an instance of it.
UCLASS(BlueprintType)
class UNREAL_FIRST_GAME_API UGameConstants : public UDataAsset
{
	GENERATED_BODY()

public:
	// M2/M3: session player cap, checked in ACoopGameMode::PreLogin (CLAUDE.md §4.7).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 5;

	// M6: replication tick rate for CoopGameState, set explicitly per CLAUDE.md §4.4 (20-30Hz
	// range) rather than left at the engine's per-class default, so it's a known, debuggable
	// number instead of an accident.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Networking")
	float GameStateNetUpdateFrequency = 30.0f;

	// M6: how often the shared elapsed-time UMG widget refreshes its displayed text. This is a
	// local display throttle, not a replication rate -- MatchStartServerTime itself replicates
	// once (set a single time in BeginPlay) and every client derives elapsed time locally from it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	float MatchTimerDisplayUpdateIntervalSeconds = 0.1f;
};
