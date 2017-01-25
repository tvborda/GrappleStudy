// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GrappleStudy : ModuleRules
{
	public GrappleStudy(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
        PrivateDependencyModuleNames.AddRange(new string[] { "CableComponent" });
        PrivateIncludePathModuleNames.AddRange(new string[] { "CableComponent" });
    }
}
