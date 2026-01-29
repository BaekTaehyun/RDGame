using UnrealBuildTool;

public class UIHelper : ModuleRules
{
    public UIHelper(ReadOnlyTargetRules Target) : base(Target)
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
                "UMG", // Restored to unconditional dependencies
                "Json",
                "JsonUtilities"
            }
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Blutility",
                    "UMGEditor",
                    "UnrealEd",
                    "EditorScriptingUtilities",
                    "AssetTools"
                }
            );
        }
    }
}
