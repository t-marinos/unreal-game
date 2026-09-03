using UnrealBuildTool;

public class Unreal_first_Game : ModuleRules
{
	public Unreal_first_Game(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// This module has no Public/Private split (flat subfolders per CLAUDE.md's repo layout:
		// Core/, Tags/, Abilities/, Scenes/, Dev/, Camera/) -- without a "Public" folder, UBT does
		// not automatically add the module root as an include search path, so folder-prefixed
		// includes like "Core/CoopGameMode.h" fail to resolve from outside that same folder. Adding
		// the module root explicitly makes that standard Unreal include style
		// (folder-prefix/File.h, same convention as "GameFramework/Actor.h") work from anywhere in
		// this module.
		PublicIncludePaths.Add(ModuleDirectory);

		// Kept minimal on purpose (Build 0, M1) — only what's actually needed compiles in. Add
		// EnhancedInput to this list only when a later milestone actually needs it in C++ —
		// CLAUDE.md §1: don't build ahead of what's needed yet. "UMG" added in M6 for the shared
		// match-timer widget (CreateWidget<UUserWidget>/AddToViewport). "AIModule" added in M9 for
		// ADummyAIController (AAIController). "GameplayTags" added in Build 1 M1 for the native
		// FGameplayTag status tags (Tags/CoopGameplayTags.h) — CLAUDE.md §4.6. "SlateCore" added in
		// Build 1 for UCoopStatusBarWidget::GetStatusColor()'s FSlateColor return type — UMG depends
		// on SlateCore itself but only privately, so it doesn't transitively link FSlateColor's
		// reflection data for downstream modules; this is the first place in this project's C++ that
		// references a SlateCore type directly (FSlateVisibility/etc. used elsewhere all live in
		// UMG's own Components/SlateWrapperTypes.h, not SlateCore).
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"AIModule",
			"GameplayTags",
			"SlateCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
