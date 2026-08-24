#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoopGameMode.generated.h"

class UGameConstants;

// Server-authoritative game mode for the prototype's 5-player coop sessions.
// GameModeBase only ever exists on the server (or the server side of a Listen Server),
// so every function here is implicitly authoritative per CLAUDE.md §4.1 -- there is no
// separate HasAuthority() check to make.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoopGameMode();

	// M9: lets one person iterate without four friends online (CLAUDE.md §7's dev mode). Toggle in
	// the Editor on BP_GameMode's Class Defaults, or via -devmode on the command line for a
	// packaged build (checked in BeginPlay below).
	UPROPERTY(EditAnywhere, Category = "Dev Mode")
	bool bDevMode = false;

protected:
	// Rejects a 6th connecting player once 5 are already present, per CLAUDE.md §4.7.
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void BeginPlay() override;

private:
	// M9: fills every empty player slot (up to MaxPlayers) with an ADummyAIController-possessed
	// ACoopCharacter, so a solo dev client can test a full 5-player session alone. Deliberately a
	// one-shot fill at session start, not a live slot-tracking system -- CLAUDE.md §4.8, this is as
	// much as "iterate alone" actually needs for Build 0.
	void FillEmptySlotsWithDummies();


	// Every tunable lives in DA_GameConstants per CLAUDE.md §10. Assigned as content wiring on
	// BP_GameMode (the Blueprint subclass that's the project's actual GlobalDefaultGameMode), not
	// hardcoded here. Left unset on the base C++ class on purpose. EditDefaultsOnly (no
	// BlueprintReadOnly -- UHT rejects that combination on a private member) is enough to expose
	// it on BP_GameMode's Class Defaults panel; nothing needs to read it from a Blueprint graph.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	// Only used if GameConstants is somehow unassigned, so a missing data asset fails loud (a log
	// warning) instead of silently rejecting every 6th player forever.
	static constexpr int32 FallbackMaxPlayers = 5;
};
