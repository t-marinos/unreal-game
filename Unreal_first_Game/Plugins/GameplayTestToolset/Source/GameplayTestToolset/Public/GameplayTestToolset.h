#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "GameplayTestToolset.generated.h"

class APlayerController;
class UInputAction;

/**
 * Drives live gameplay state for testing by injecting input and console commands directly,
 * bypassing keyboard/window focus. Covers Enhanced Input action activation and Exec console
 * commands scoped to a specific player.
 */
UCLASS(BlueprintType, MinimalAPI)
class UGameplayTestToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:

	/** Injects a press-then-release of the given Enhanced Input action into the specified
	 * player's local input, exactly as if a real key or button press occurred. Fires whatever
	 * Blueprint/C++ triggers are bound to the action (e.g. "Triggered").
	 * @param Controller The target player's own client-local PlayerController -- must have a
	 * valid LocalPlayer. On a PIE server world, the replicated representation of a remote
	 * client has no LocalPlayer; pass that client's own world's PlayerController instead.
	 * @param Action The Enhanced Input action asset to trigger. */
	UFUNCTION(meta = (AICallable), Category = "GameplayTestToolset")
	static GAMEPLAYTESTTOOLSET_API void TriggerInputAction(APlayerController* Controller, UInputAction* Action);

	/** Executes a console command as the specified player, exactly as if typed into that
	 * player's in-game console.
	 * @param Controller The target player's own client-local PlayerController the command
	 * executes against. Same LocalPlayer requirement as TriggerInputAction.
	 * @param Command The full console command string, e.g. "ApplyTestDamage 30". */
	UFUNCTION(meta = (AICallable), Category = "GameplayTestToolset")
	static GAMEPLAYTESTTOOLSET_API void ExecCommand(APlayerController* Controller, const FString& Command);
};
