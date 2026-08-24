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
}
