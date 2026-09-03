using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class GameplayTestToolset : ModuleRules
	{
		public GameplayTestToolset(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"ToolsetRegistry",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"EditorSubsystem",
					"EnhancedInput",
					"Engine",
					"UnrealEd",
				}
			);
		}
	}
}
