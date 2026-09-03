using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class GameplayTestToolsetTests : ModuleRules
	{
		public GameplayTestToolsetTests(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"CQTest",
				"Engine",
				"EnhancedInput",
				"GameplayTestToolset",
				"ToolsetRegistry",
				"UnrealEd",
			});
		}
	}
}
