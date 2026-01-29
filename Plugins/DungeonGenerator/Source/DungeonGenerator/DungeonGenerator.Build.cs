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
        "GeometryFramework",       // Provides UDynamicMeshComponent and UDynamicMesh
        "PCG",                     // Added for PCG Graph references in Theme
        "Landscape"                // For ALandscape in SpawnedLandscape
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

        // Public include paths
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

        PublicDependencyModuleNames.AddRange(PublicDependencyModuleNamesArray);

        PrivateDependencyModuleNames.AddRange(PrivateDependencyModuleNamesArray);

        // Editor-only modules for GEditor, ActorFactory
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
    }
}
