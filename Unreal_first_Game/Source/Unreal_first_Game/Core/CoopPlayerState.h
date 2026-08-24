#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/CoopRoleTypes.h"
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

	// Build 1, M2: which of the 5 roles this player is playing this run. Defaults Unassigned until
	// the RoleSelect phase (M3) resolves -- see CoopRoleTypes.h and DECISIONS.md's "Role assignment
	// is player-chosen, not random" entry for why this is a claim, not a random shuffle.
	// Named PlayerRole, not Role -- AActor already declares a Role property (its replication
	// ENetRole), and UHT rejects a member name that shadows a base class member.
	UFUNCTION(BlueprintPure, Category = "Role")
	EPlayerRole GetRole() const { return PlayerRole; }

	// Server-only. Called by ACoopPlayerController::Server_ClaimRole's RPC handler (M3) once it has
	// confirmed the requested role is still Unassigned across every PlayerState.
	void SetRole(EPlayerRole NewRole);

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Dev Mode")
	bool bInvulnerable = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Role")
	EPlayerRole PlayerRole = EPlayerRole::Unassigned;
};
