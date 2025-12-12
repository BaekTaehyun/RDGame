using UnrealBuildTool;
using System.IO;

public class DungeonGenerator : ModuleRules
{
    private static readonly string[] PublicDependencyModuleNamesArray =
    {
        "Core",
        "DungeonCore",
        "ProceduralMeshComponent",
        "NavigationSystem",
        "GeometryScriptingCore",  // Phase 3: Mesh Merging
        "GeometryFramework"       // Provides UDynamicMeshComponent and UDynamicMesh
    };
    private static readonly string[] PrivateDependencyModuleNamesArray =
    {
        "CoreUObject",
        "Engine",
        "Slate",
        "SlateCore",
    };

    public DungeonGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Public 폴더를 include 경로에 추가
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

        PublicDependencyModuleNames.AddRange(PublicDependencyModuleNamesArray);

        PrivateDependencyModuleNames.AddRange(PrivateDependencyModuleNamesArray);
    }
}
