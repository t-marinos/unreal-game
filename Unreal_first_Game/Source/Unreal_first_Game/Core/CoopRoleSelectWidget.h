#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/CoopRoleTypes.h"
#include "CoopRoleSelectWidget.generated.h"

// Build 1, M5. C++ base for WBP_RoleSelect. BlueprintPure query functions for the same reason as
// UCoopMatchTimerWidget (UMG's "Bind Function" list needs genuine pure UFUNCTIONs); ClaimRole is
// BlueprintCallable so a button's OnClicked event in the widget graph can call it directly.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopRoleSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// True if any PlayerState (real or dev-mode dummy) currently holds this role. Reads
	// GameState->PlayerArray directly -- purely a display query, resolves nothing itself.
	UFUNCTION(BlueprintPure, Category = "Role Select")
	bool IsRoleTaken(EPlayerRole Role) const;

	// Seconds remaining in the RoleSelect phase, derived from
	// ACoopGameState::GetRoleSelectEndServerTime() minus server world time every call -- never a
	// locally ticked countdown (CLAUDE.md §4.5). Returns "0" before the phase has started.
	UFUNCTION(BlueprintPure, Category = "Role Select")
	FText GetRoleSelectRemainingSecondsText() const;

	// Intent only -- forwards to this player's own ACoopPlayerController::Server_ClaimRole, which
	// is the actual authoritative check (CLAUDE.md §4.1). A rejected claim (role already taken)
	// simply has no visible effect; IsRoleTaken() above is what the widget should poll to reflect
	// the outcome, not a return value from this call.
	UFUNCTION(BlueprintCallable, Category = "Role Select")
	void ClaimRole(EPlayerRole DesiredRole);
};
