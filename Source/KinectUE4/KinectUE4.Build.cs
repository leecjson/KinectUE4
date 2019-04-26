// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class KinectUE4 : ModuleRules
{
	public KinectUE4(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicLibraryPaths.Add(@"C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\Lib\x64");
        //PublicLibraryPaths.Add(@"C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\Redist\VGB\x64");
        PublicAdditionalLibraries.Add("Kinect20.lib");
        PublicAdditionalLibraries.Add("Kinect20.VisualGestureBuilder.lib");

        //RuntimeDependencies.Add(new RuntimeDependency(@"C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\Redist\VGB\x64\Kinect20.VisualGestureBuilder.dll"));

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
                 @"C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\inc"
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
                //"$(KINECTSDK20_DIR)inc",
                @"C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\inc"
            }
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
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
	}
}
