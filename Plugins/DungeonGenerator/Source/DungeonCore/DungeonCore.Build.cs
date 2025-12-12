using UnrealBuildTool;
using System.IO;

public class DungeonCore : ModuleRules
{
    private static readonly string[] PublicDependencyModuleNamesArray =
    {
        "Core",
        "CoreUObject",
        "Engine"
    };

    public DungeonCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Public 헤더가 외부 모듈에서 접근 가능하도록 설정
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

        PublicDependencyModuleNames.AddRange(PublicDependencyModuleNamesArray);
    }
}

