// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MultiplayerPlugin : ModuleRules
{
	public MultiplayerPlugin(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MultiplayerPlugin",
			"MultiplayerPlugin/Variant_Platforming",
			"MultiplayerPlugin/Variant_Platforming/Animation",
			"MultiplayerPlugin/Variant_Combat",
			"MultiplayerPlugin/Variant_Combat/AI",
			"MultiplayerPlugin/Variant_Combat/Animation",
			"MultiplayerPlugin/Variant_Combat/Gameplay",
			"MultiplayerPlugin/Variant_Combat/Interfaces",
			"MultiplayerPlugin/Variant_Combat/UI",
			"MultiplayerPlugin/Variant_SideScrolling",
			"MultiplayerPlugin/Variant_SideScrolling/AI",
			"MultiplayerPlugin/Variant_SideScrolling/Gameplay",
			"MultiplayerPlugin/Variant_SideScrolling/Interfaces",
			"MultiplayerPlugin/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
