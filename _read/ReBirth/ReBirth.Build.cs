// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ReBirth : ModuleRules
{
	public ReBirth(ReadOnlyTargetRules Target) : base(Target)
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
			// Lyra-style Hero Component dependencies
			"GameplayTags",
			"GameplayAbilities",
			"ModularGameplay"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ReBirth",
			"ReBirth/Input",
			"ReBirth/Character",
			"ReBirth/Variant_Platforming",
			"ReBirth/Variant_Platforming/Animation",
			"ReBirth/Variant_Combat",
			"ReBirth/Variant_Combat/AI",
			"ReBirth/Variant_Combat/Animation",
			"ReBirth/Variant_Combat/Gameplay",
			"ReBirth/Variant_Combat/Interfaces",
			"ReBirth/Variant_Combat/UI",
			"ReBirth/Variant_SideScrolling",
			"ReBirth/Variant_SideScrolling/AI",
			"ReBirth/Variant_SideScrolling/Gameplay",
			"ReBirth/Variant_SideScrolling/Interfaces",
			"ReBirth/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
