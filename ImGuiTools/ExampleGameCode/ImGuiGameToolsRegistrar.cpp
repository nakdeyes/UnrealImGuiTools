// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsRegistrar.h"

#if !UE_BUILD_SHIPPING
#include <imgui.h>
#include <ImGuiDebugTools.h>
#include <ImGuiDebugToolsManager.h>
#endif // #if !UE_BUILD_SHIPPING

// Function that registers module level tools. Can be called from ::StartupModule()
void GameImGuiToolsRegistrar::RegisterGameDebugTools()
{
#if !UE_BUILD_SHIPPING
	FImGuiToolsModule& ImGuiToolsModule = FModuleManager::GetModuleChecked<FImGuiToolsModule>("ImGuiTools");
	if (ImGuiToolsModule.GetToolsManager().IsValid())
	{
		static const FName GameToolsNamespace = TEXT("Game Tools");
		ImGuiToolsModule.GetToolsManager()->RegisterToolWindow(TSharedPtr<FExampleImGuiTool>(new FExampleImGuiTool()), GameToolsNamespace);
	}
#endif // #if !UE_BUILD_SHIPPING
}

#if !UE_BUILD_SHIPPING
// Example ImGui Tool Window
FExampleImGuiTool::FExampleImGuiTool()
{
	ToolName = TEXT("Example ImGui Tool");
}

void FExampleImGuiTool::ImGuiUpdate(float DeltaTime)
{
	ImGui::Text("Name");
}
#endif // #if !UE_BUILD_SHIPPING