// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiToolWindow.h"

class IMGUITOOLS_API FImGuiMemoryDebugger : public FImGuiToolWindow
{
public:
	FImGuiMemoryDebugger();
	virtual ~FImGuiMemoryDebugger() = default;

	// FImGuiToolWindow Interface
	virtual void ImGuiUpdate(float DeltaTime) override;
	virtual void UpdateTool(float DeltaTime) override;
	// FImGuiToolWindow Interface
};