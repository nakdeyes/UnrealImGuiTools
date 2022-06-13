// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <ImGuiToolWindow.h>

#if !UE_BUILD_SHIPPING
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
#endif // #if !UE_BUILD_SHIPPING

namespace GameImGuiToolsRegistrar
{
	// Function that registers module level tools. Can be called from ::StartupModule()
	void RegisterGameDebugTools();
};
