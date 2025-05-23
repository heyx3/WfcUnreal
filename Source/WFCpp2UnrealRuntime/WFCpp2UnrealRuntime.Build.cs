﻿using UnrealBuildTool;

public class WFCpp2UnrealRuntime : ModuleRules
{
    public WFCpp2UnrealRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "WFCpp2"
            }
        );
        if (Target.bCompileAgainstEditor)
            PrivateDependencyModuleNames.AddRange(new[] {
                "AssetTools"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );
    }
}