#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "CoopUnitFrameWidget.generated.h"

class UBorder;
class UTextBlock;
class UProgressBar;
class UCoopHealthComponent;
struct FGameplayTag;

// Which unit a frame instance shows. Set per-instance in the WBP Designer, exactly like
// UCoopAbilitySlotWidget::SlotIndex.
UENUM(BlueprintType)
enum class EUnitFrameSource : uint8
{
	// The local player's current click-selected target
	// (ACoopPlayerController::GetCurrentTargetActor). This is the C++ default (enum value 0), so
	// WBP_TargetFrame's single instance needs no per-instance override.
	CurrentTarget,

	// GameState->PlayerArray[PartyMemberIndex]'s pawn -- one row of WBP_PartyFrame's 5-row stack.
	PartyMember
};

// Cursor-targeting feature (cursor_progress.md). ONE reusable frame that drives BOTH the top-left
// target frame (Source=CurrentTarget) and every row of the always-on party stack
// (Source=PartyMember, PartyMemberIndex 0-4).
//
// Purely local, cosmetic UI (CLAUDE.md §4.2): it only READS already-replicated state
// (UCoopHealthComponent's CurrentHealth/MaxHealth, ACoopPlayerState::GetRole,
// ACoopCharacter / ACoopMonsterCharacter::HasStatusTag) off whichever actor it resolves each tick.
// The target selection itself is never replicated (see ACoopPlayerController::GetCurrentTargetActor).
//
// All feedback is NativeTick against BindWidgetOptional pointers -- no Designer "Bind Function"
// bindings (can't be authored via unreal-mcp -- DECISIONS.md), same approach as
// UCoopRoleSelectWidget / UCoopActionBarWidget. Show/hide is a RenderOpacity toggle, never this
// widget's own Visibility: it is created (in ACoopPlayerController::BeginPlay) during the RoleSelect
// phase with nothing to show, and a widget that leaves the "visible" family in its own NativeTick
// freezes forever (the P9 action-bar gotcha, DECISIONS.md).
UCLASS()
class UNREAL_FIRST_GAME_API UCoopUnitFrameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Frame")
	EUnitFrameSource Source = EUnitFrameSource::CurrentTarget;

	// Only used when Source == PartyMember. Which entry of GameState->PlayerArray this row shows.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Frame")
	int32 PartyMemberIndex = 0;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Left-click a party-stack row (Source == PartyMember) to make that teammate the local player's
	// current target -- the top-left target frame and the ground ring then pick it up on their next
	// tick. Right-click and every other case return Unhandled so the right-click-drag orbit camera
	// still works with the mouse anywhere over the party frame. The target frame instance
	// (Source == CurrentTarget) stays HitTestInvisible, so this is never even called for it.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	// The actor this frame should display right now, or nullptr if there's nothing to show
	// (no target selected / party index past the current roster / pawn not spawned yet).
	AActor* ResolveSubjectActor() const;

	// Reads health / tags off EITHER an ACoopCharacter or an ACoopMonsterCharacter -- one explicit
	// Cast per class, no shared base (CLAUDE.md §4.6). Return nullptr / false for anything else.
	static UCoopHealthComponent* HealthOf(const AActor* Actor);
	static bool ActorHasTag(const AActor* Actor, const FGameplayTag& Tag);

	// True only when this frame is showing the local player's own character -- the party stack tints
	// that one row so a player can pick themselves out. Always false for enemy targets.
	bool IsLocalPlayerSubject(const AActor* SubjectActor) const;

	// --- WBP_UnitFrame children, matched by name ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TypeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
};
