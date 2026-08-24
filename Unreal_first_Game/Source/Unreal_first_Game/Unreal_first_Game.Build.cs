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
		// EnhancedInput/AIModule/GameplayTags to this list only when a later milestone (dev-mode
		// dummy AI, status tags) actually needs them — CLAUDE.md §1: don't build ahead of what's
		// needed yet. "UMG" added in M6 for the shared match-timer widget (CoopPlayerController's
		// CreateWidget<UUserWidget>/AddToViewport call).
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
