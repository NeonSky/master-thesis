using UnrealBuildTool;

public class ShaderModels : ModuleRules {
	public ShaderModels(ReadOnlyTargetRules Target) : base(Target) {

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
      "Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"RHI",
			"RenderCore",
    });

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Projects",
    });

	}
}
