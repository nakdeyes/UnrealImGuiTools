// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiGameToolsRegistrar.h"

#if DRAW_IMGUI_TOOLS
#include <imgui.h>
#include <ImGuiTools.h>
#include <ImGuiToolsManager.h>
#endif // #if DRAW_IMGUI_TOOLS

// Function that registers module level tools. Can be called from ::StartupModule()
void GameImGuiToolsRegistrar::RegisterGameDebugTools()
{
#if DRAW_IMGUI_TOOLS
	FImGuiToolsModule& ImGuiToolsModule = FModuleManager::GetModuleChecked<FImGuiToolsModule>("ImGuiTools");
	if (ImGuiToolsModule.GetToolsManager().IsValid())
	{
		static const FName GameToolsNamespace = TEXT("Game Tools");
		ImGuiToolsModule.GetToolsManager()->RegisterToolWindow(TSharedPtr<FExampleImGuiTool>(new FExampleImGuiTool()), GameToolsNamespace);
	}
#endif // #if DRAW_IMGUI_TOOLS
}

#if DRAW_IMGUI_TOOLS
// Example ImGui Tool Window
FExampleImGuiTool::FExampleImGuiTool()
{
	ToolName = TEXT("Example ImGui Tool");
}

void FExampleImGuiTool::ImGuiUpdate(float DeltaTime)
{
	ImGui::Text("Name");
}
#endif // #if DRAW_IMGUI_TOOLS