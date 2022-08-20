// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiToolWindow.h"

class IMGUITOOLS_API FImGuiCDOExplorer : public FImGuiToolWindow
{
public:
	FImGuiCDOExplorer();
	virtual ~FImGuiCDOExplorer() = default;

	// FImGuiToolWindow Interface
	virtual void ImGuiUpdate(float DeltaTime) override;
	// FImGuiToolWindow Interface
};