// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealMasterThesis : ModuleRules {
	public UnrealMasterThesis(ReadOnlyTargetRules Target) : base(Target) {
		
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ProceduralMeshComponent",
			"RHI",
			"RenderCore",
			"Json",
			"SlateCore",
			"Slate",
			"ShaderModels"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

	}
}
