// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

using UnrealBuildTool;

public class ImGuiToolsEditor : ModuleRules
{
	public ImGuiToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
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
				"ImGuiTools",

				"CoreUObject",
				"Engine",
				"InputCore",
				"Projects",
				"RenderCore",
				"RHI",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				// ... add private dependencies that you statically link with here ...	
			}
			);

		// Additional Dependencies for non-Shipping builds.
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"ImGui",
			});
		}
	}
}
