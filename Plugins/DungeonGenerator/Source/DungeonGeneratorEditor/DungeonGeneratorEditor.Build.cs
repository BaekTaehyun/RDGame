using UnrealBuildTool;

public class DungeonGeneratorEditor : ModuleRules
{
	public DungeonGeneratorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"DungeonCore",
				"DungeonGenerator",
				"InputCore",
				"ToolMenus"
			}
			);
	}
}
