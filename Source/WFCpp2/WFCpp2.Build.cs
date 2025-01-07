// Copyright Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using EpicGames.Core;
using UnrealBuildTool;

public class WFCpp2 : ModuleRules
{
	public WFCpp2(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		//Integrate WFC++.
		//TODO: Bring WFC++ in as a git submodule somehow.
		PublicIncludePaths.AddRange(new string[] {
			"E:/Git Repos/wfcpp/WFC++"
		} );
		var wfcDir = DirectoryReference.Combine(
			DirectoryReference.FromString(ModuleDirectory).ParentDirectory.ParentDirectory.ParentDirectory.ParentDirectory.ParentDirectory,
			"wfcpp", "WFC++"
		);
		foreach (string subFolder in new[] { "Tiled3D", "HelperSrc" })
		{
			var nestedWfcDir = DirectoryReference.Combine(wfcDir, subFolder);
			if (!ConditionalAddModuleDirectory(nestedWfcDir))
				throw new Exception("WFC++ directory not found! Expected at " + nestedWfcDir);
		}
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core"
		} );
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore"
		} );
		
		//Enable debugging:
		if (new HashSet<UnrealTargetConfiguration>() {
			    UnrealTargetConfiguration.Debug,
			    UnrealTargetConfiguration.DebugGame
		    }.Contains(target.Configuration))
		{
			PublicDefinitions.Add("WFCPP_CHECK_MEMORY=1");
			PublicDefinitions.Add("WFCPP_DEBUG=1");
		}
		PublicDefinitions.Add("WFC_API=WFCPP2_API");
	}
}
