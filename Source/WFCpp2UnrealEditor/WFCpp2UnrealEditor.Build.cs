﻿using UnrealBuildTool;

public class WFCpp2UnrealEditor : ModuleRules
{
    public WFCpp2UnrealEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "WFCpp2",
                "WFCpp2UnrealRuntime",
                "SlateCore", "Slate", "InputCore",
                "KismetWidgets", "ToolWidgets", "AdvancedWidgets",
                "EditorWidgets", "DesktopWidgets",
                "PropertyEditor",
                "SharedSettingsWidgets"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject", "Engine",
                "UnrealEd",
                "Json", "Slate", "SlateCore", "EditorStyle", "EditorWidgets",
                "Kismet", "KismetWidgets",
                "PropertyEditor", "WorkspaceMenuStructure", "ContentBrowser",
                "AdvancedPreviewScene",
                "RenderCore",
                "Projects", "AssetRegistry",
                
                "DataprepCore" //For some utility functions
            }
        );
        
        PrivateIncludePathModuleNames.AddRange(new [] {
            "Settings",
            "AssetTools", "LevelEditor"
        });
        DynamicallyLoadedModuleNames.AddRange(new[] {
            "AssetTools"
        });
    }
}