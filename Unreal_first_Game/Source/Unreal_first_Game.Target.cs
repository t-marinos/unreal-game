using UnrealBuildTool;
using System.Collections.Generic;

// Game (runtime, no Editor) target. Built for the packaged Development build in later builds
// (§3.1) — the Editor target below is what you actually use day to day.
public class Unreal_first_GameTarget : TargetRules
{
	public Unreal_first_GameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("Unreal_first_Game");
	}
}
