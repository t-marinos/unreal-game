#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// Build 1, M1: native FGameplayTag declarations. The canonical list of every tag name (what
// writes it, what reads it, why) lives in docs/abilities.md per CLAUDE.md §4.6 -- do not invent a
// new tag here without adding it there first. Only the tags an actual milestone needs exist below;
// docs/abilities.md documents several more (Status.SpeedBuff, Status.Broken, Status.Linked, etc.)
// that later builds will declare when their milestone lands, not preemptively here.
namespace CoopGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Shielded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Fortress);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Downed);

	// Build 1: Support Speed writes this, Runner Dash reads it (Thousand Dashes synergy).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_SpeedBuff);

	// Build 1: dev/test-granted only for now (ACoopPlayerController::ApplyTestVulnerable) --
	// docs/abilities.md's real writer is "The Heart"'s mechanic (Scene 5, Build 2), not built yet.
	// Damage Execution reads this on an ACoopMonsterCharacter target.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Vulnerable_Physical);

	// Ability kit expansion: Tank Armor Break writes this on a monster target. Control's Mind
	// Fracture (Build 2, needs The False King) is the eventual reader -- nothing reads it yet, so
	// for now it only shows on the target frame's status line (UCoopUnitFrameWidget) and expires.
	// Already in docs/abilities.md's glossary -- declared here, not invented.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Broken);

	// Ability kit expansion: Damage Overload reads this on a monster target (magic branch of
	// Execution/Overload). docs/abilities.md's real writer is "The Heart" (Scene 5, Build 2); until
	// then it is dev/test-granted via ACoopPlayerController::ApplyTestVulnerableMagic, same
	// precedent as Status.Vulnerable.Physical above. Already in docs/abilities.md's glossary.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Vulnerable_Magic);
}
