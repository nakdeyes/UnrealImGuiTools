// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolWindow.h"
#include "Utils/ImGuiUtils.h"

#include <imgui.h>

void FImGuiToolWindow::EnableTool(bool bNewEnabled)
{
	bEnabled = bNewEnabled;
}

bool& FImGuiToolWindow::GetEnabledRef()
{
	return bEnabled;
}

const FString& FImGuiToolWindow::GetToolName()
{
	return ToolName;
}

void FImGuiToolWindow::UpdateTool(float DeltaTime)
{
	if (bEnabled)
	{
		if (ImGui::Begin(Ansi(*GetToolName()), &bEnabled, WindowFlags))
		{
			ImGuiUpdate(DeltaTime);
		}

		ImGui::End();
	}
}