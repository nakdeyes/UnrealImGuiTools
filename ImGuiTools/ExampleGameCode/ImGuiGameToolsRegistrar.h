// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

/**
*	Example ImGui Game Tools Registrar. An example of how to register your game level ImGui Tools w/ the ImGuiTools plugin!
*
*	Steps to follow to put in your game.
*		1. Copy this ImGuiGameToolsRegistrar .h / .cpp to your game module somewhere you want the tools to exist.
*		2. Change "GAME_API" below to your module dll export api macro for your module.
*		3. If you haven't already add "ImGuiTools" to your included plugins in your .uproject file.
*		4. If you haven't already, add "ImGui" and "ImGuiTools" to your module's *.build.cs dependencies.
*		5. While you are there in your *.build.cs add this line when the ImGuiTools Plugin is included:
*			PublicDefinitions.Add("DRAW_IMGUI_TOOLS=1");
*		6. Somewhere in your game module ( I prefer FDefaultGameModuleImpl::StartupModule() ), you should call
*		   GameImGuiToolsRegistrar::RegisterGameDebugTools(); to register your tools.
*
*   That's it! One last tip: I recommend pulling your tools out of the registrar and into their own .h / .cpp files. Good luck!
*/

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
