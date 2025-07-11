// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

using UnrealBuildTool;

public class ImGuiTools : ModuleRules
{
	public ImGuiTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags"
				// ... add other public dependencies that you statically link with here ...
			}
		);
		
		// Add StructUtils only for UE 5.4 and earlier
		if (Target.Version.MajorVersion < 5 || (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion <= 4))
		{
			PublicDependencyModuleNames.Add("StructUtils");
		}
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"InputCore",
				"Projects",
				"RenderCore",
				"RHI",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
		);

		// Additional Dependencies for non-Shipping, non-dedicated server builds.
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "ImGui",
            });
            
            // DRAW_IMGUI_TOOLS - is meant to be a flag that can be disabled to run the module in a minimal / mostly disabled state.
            //	It is recommended to simply not load the module for whatever builds you do not wish to use ImGuiTools with, but this
            //	flag can be disabled for corner cases where you want a mostly no-op module.
            PublicDefinitions.Add("DRAW_IMGUI_TOOLS=1");
        }
		else 
		{
			PublicDefinitions.Add("DRAW_IMGUI_TOOLS=0");
		}
	}
}
