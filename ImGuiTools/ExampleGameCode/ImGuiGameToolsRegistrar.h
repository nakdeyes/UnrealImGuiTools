// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <ImGuiToolWindow.h>

#if DRAW_IMGUI_TOOLS
// Example ImGui Tool Window
class GAME_API FExampleImGuiTool 
	: public FImGuiToolWindow
{
public:
	FExampleImGuiTool();
	virtual ~FExampleImGuiTool() = default;

	// FImGuiDebugToolWindow 
	virtual void ImGuiUpdate(float DeltaTime) override;
	// ~FImGuiDebugToolWindow 
};
#endif // #if DRAW_IMGUI_TOOLS

namespace GameImGuiToolsRegistrar
{
	// Function that registers module level tools. Can be called from ::StartupModule()
	void RegisterGameDebugTools();
};
