// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RdGame : ModuleRules
{
    public RdGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate",
            "SlateCore",
            "GsMessaging",
            "GsNetworking",
            "GameplayTags",
            "ModularGameplay",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            "RdGame",
            "RdGame/Variant_Platforming",
            "RdGame/Variant_Platforming/Animation",
            "RdGame/Variant_Combat",
            "RdGame/Variant_Combat/AI",
            "RdGame/Variant_Combat/Animation",
            "RdGame/Variant_Combat/Gameplay",
            "RdGame/Variant_Combat/Interfaces",
            "RdGame/Variant_Combat/UI",
            "RdGame/Variant_SideScrolling",
            "RdGame/Variant_SideScrolling/AI",
            "RdGame/Variant_SideScrolling/Gameplay",
            "RdGame/Variant_SideScrolling/Interfaces",
            "RdGame/Variant_SideScrolling/UI",
            "RdGame/Network",
            "RdGame/UI"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
