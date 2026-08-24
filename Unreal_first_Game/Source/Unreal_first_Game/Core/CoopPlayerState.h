#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CoopPlayerState.generated.h"

// Establishes the pattern early -- Build 1 adds each player's assigned role and gameplay tags
// here, replicated per CLAUDE.md §4.3 so state stays inspectable.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// M9: dev-mode god mode/invuln toggle (CLAUDE.md §7). Stub -- real and replicated, but nothing
	// checks it yet since Build 0 has no damage system. Wire the gating check when Scene 2's damage
	// exists in Build 1.
	UFUNCTION(BlueprintPure, Category = "Dev Mode")
	bool IsInvulnerable() const { return bInvulnerable; }

	// Server-only. Called by ACoopPlayerController::Server_ToggleGodMode's RPC handler.
	void SetInvulnerable(bool bNewInvulnerable);

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Dev Mode")
	bool bInvulnerable = false;
};
