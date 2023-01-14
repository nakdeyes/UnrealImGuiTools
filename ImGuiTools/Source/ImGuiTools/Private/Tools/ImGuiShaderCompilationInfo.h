// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiToolWindow.h"

class IMGUITOOLS_API FImGuiShaderCompilationInfo : public FImGuiToolWindow
{
public:
	FImGuiShaderCompilationInfo();
	virtual ~FImGuiShaderCompilationInfo() = default;

	// FImGuiToolWindow Interface
	virtual void ImGuiUpdate(float DeltaTime) override;
	// FImGuiToolWindow Interface
};