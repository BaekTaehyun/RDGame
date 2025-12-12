using UnrealBuildTool;
using System.IO;

public class GsNetworking : ModuleRules
{
	public GsNetworking(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Sockets",
				"Networking"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        // Libsodium Configuration
        PublicIncludePaths.AddRange(
            new string[] {
                Path.GetFullPath(Path.Combine(ModulePath, "libsodium/include/"))
            }
            );

        // Windows Platform Sockets Private Header
        if ((Target.Platform == UnrealTargetPlatform.Win64))
        {
            string enginePath = Path.GetFullPath(Target.RelativeEnginePath);
            string runtimePath = enginePath + "Source/Runtime/";

            PublicIncludePaths.Add(runtimePath + "Sockets/Private");
        }
        
        LoadSodium();
	}

    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public bool LoadSodium()
    {
        if ((Target.Platform == UnrealTargetPlatform.Win64))
        {
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libsodium/lib/windows-x64/", "libsodium.lib"));
        }

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicSystemLibraryPaths.Add(Path.Combine(ModuleDirectory, "libsodium/lib/android-armv7-a/"));
            PublicSystemLibraryPaths.Add(Path.Combine(ModuleDirectory, "libsodium/lib/android-armv8-a/"));
            PublicAdditionalLibraries.Add("sodium");
        }

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libsodium/lib/ios/", "libsodium.a"));
        }

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libsodium/lib/osx/", "libsodium.a"));
        }

        PublicDefinitions.Add(string.Format("WITH_SODIUM_BINDING={0}", 1));
        PublicDefinitions.Add("SODIUM_STATIC");
        PublicDefinitions.Add("_LIB");

        return true;
    }
}
