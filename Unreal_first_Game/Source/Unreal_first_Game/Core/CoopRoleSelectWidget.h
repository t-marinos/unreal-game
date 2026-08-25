#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Core/CoopRoleTypes.h"
#include "CoopRoleSelectWidget.generated.h"

// Build 1, M5. C++ base for WBP_RoleSelect. BlueprintPure query functions for the same reason as
// UCoopMatchTimerWidget (UMG's "Bind Function" list needs genuine pure UFUNCTIONs); ClaimRole is
// BlueprintCallable so a button's OnClicked event in the widget graph can call it directly.
//
// IsXTaken/IsXAvailable/ClaimX below are thin, no-argument wrappers around IsRoleTaken/ClaimRole
// for exactly one reason: UMG's Designer-side property binding (Text/Visibility/IsEnabled -- "Bind
// Function") and simple OnClicked event wiring both need a parameterless function to point at.
// IsRoleTaken(EPlayerRole)/ClaimRole(EPlayerRole) stay as the real, reusable implementations.
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

	// Bind the whole role-select panel's Visibility to this. Visible only during the RoleSelect
	// phase -- Collapsed the instant it resolves, so this screen doesn't linger over the prep
	// arena.
	UFUNCTION(BlueprintPure, Category = "Role Select")
	ESlateVisibility GetRoleSelectVisibility() const;

	// Intent only -- forwards to this player's own ACoopPlayerController::Server_ClaimRole, which
	// is the actual authoritative check (CLAUDE.md §4.1). A rejected claim (role already taken)
	// simply has no visible effect; IsRoleTaken() above is what the widget should poll to reflect
	// the outcome, not a return value from this call.
	UFUNCTION(BlueprintCallable, Category = "Role Select")
	void ClaimRole(EPlayerRole DesiredRole);

	// Bind each role's "Claim" button IsEnabled to the matching IsXAvailable() below, and each
	// button's OnClicked to the matching ClaimX() below -- both parameterless, per the class
	// comment above.
	UFUNCTION(BlueprintPure, Category = "Role Select")
	bool IsTankAvailable() const { return !IsRoleTaken(EPlayerRole::Tank); }

	UFUNCTION(BlueprintPure, Category = "Role Select")
	bool IsSupportAvailable() const { return !IsRoleTaken(EPlayerRole::Support); }

	UFUNCTION(BlueprintPure, Category = "Role Select")
	bool IsRunnerAvailable() const { return !IsRoleTaken(EPlayerRole::Runner); }

	UFUNCTION(BlueprintPure, Category = "Role Select")
	bool IsControlAvailable() const { return !IsRoleTaken(EPlayerRole::Control); }

	UFUNCTION(BlueprintPure, Category = "Role Select")
	bool IsDamageAvailable() const { return !IsRoleTaken(EPlayerRole::Damage); }

	UFUNCTION(BlueprintCallable, Category = "Role Select")
	void ClaimTank() { ClaimRole(EPlayerRole::Tank); }

	UFUNCTION(BlueprintCallable, Category = "Role Select")
	void ClaimSupport() { ClaimRole(EPlayerRole::Support); }

	UFUNCTION(BlueprintCallable, Category = "Role Select")
	void ClaimRunner() { ClaimRole(EPlayerRole::Runner); }

	UFUNCTION(BlueprintCallable, Category = "Role Select")
	void ClaimControl() { ClaimRole(EPlayerRole::Control); }

	UFUNCTION(BlueprintCallable, Category = "Role Select")
	void ClaimDamage() { ClaimRole(EPlayerRole::Damage); }
};
