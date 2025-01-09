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
		
		//Integrate WFC++.
		//  Headers:
		var wfcDir = DirectoryReference.Combine(
			DirectoryReference.FromString(PluginDirectory),
			"Core"
		);
		PublicIncludePaths.Add(wfcDir.FullName);
		//  Source:
		var wfcSourceDirs = new string[] { "Helpers", "Tiled3D" };
		foreach (string wfcSourceDir in wfcSourceDirs)
		{
			var directory = DirectoryReference.Combine(
				wfcDir, "WFC++", "src", wfcSourceDir
			);
			if (!ConditionalAddModuleDirectory(directory))
				throw new Exception("WFC++ directory not found! Expected at " + directory);
		}
		//  Preprocessor:
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
