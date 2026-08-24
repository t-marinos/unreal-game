using UnrealBuildTool;
using System.Collections.Generic;

// Editor target — this is what the Unreal Editor itself builds and loads.
public class Unreal_first_GameEditorTarget : TargetRules
{
	public Unreal_first_GameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("Unreal_first_Game");
	}
}
