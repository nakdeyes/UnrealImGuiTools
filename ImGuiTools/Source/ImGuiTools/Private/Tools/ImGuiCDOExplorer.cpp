// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiCDOExplorer.h"

#include <imgui.h>

#include "ImGuiUtils.h"
#include "Widgets/ClassSelectorWidget.h"

FImGuiCDOExplorer::FImGuiCDOExplorer()
{
	ToolName = "CDO Explorer";
}

void FImGuiCDOExplorer::ImGuiUpdate(float DeltaTime)
{
    ImGui::BeginChild("CDOExplorerSettings", ImVec2(0, 200), true);

    static ImGuiTools::UClassSelector CDOExplorerClassSelector(APawn::StaticClass());
    CDOExplorerClassSelector.Draw("CDOExplorerClassSel", ImVec2(0, 180.0f));
    
    UClass* SelectedClass = CDOExplorerClassSelector.GetSelectedClass();
    ImGui::Text("Selected Class: %s", SelectedClass ? Ansi(*SelectedClass->GetName()) : "*none*");
    if (SelectedClass && ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Load and View X BP CDOs from class: %s"), *SelectedClass->GetName()))))
    {

    }

    ImGui::EndChild();  // "CDOExplorerSettings"
}
