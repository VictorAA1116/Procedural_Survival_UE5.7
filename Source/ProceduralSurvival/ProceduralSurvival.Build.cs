// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProceduralSurvival : ModuleRules
{
	public ProceduralSurvival(ReadOnlyTargetRules Target) : base(Target)
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
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
      "ProceduralMeshComponent"
    });

		PublicIncludePaths.AddRange(new string[] {
			"ProceduralSurvival",
			"ProceduralSurvival/Variant_Platforming",
			"ProceduralSurvival/Variant_Platforming/Animation",
			"ProceduralSurvival/Variant_Combat",
			"ProceduralSurvival/Variant_Combat/AI",
			"ProceduralSurvival/Variant_Combat/Animation",
			"ProceduralSurvival/Variant_Combat/Gameplay",
			"ProceduralSurvival/Variant_Combat/Interfaces",
			"ProceduralSurvival/Variant_Combat/UI",
			"ProceduralSurvival/Variant_SideScrolling",
			"ProceduralSurvival/Variant_SideScrolling/AI",
			"ProceduralSurvival/Variant_SideScrolling/Gameplay",
			"ProceduralSurvival/Variant_SideScrolling/Interfaces",
			"ProceduralSurvival/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
