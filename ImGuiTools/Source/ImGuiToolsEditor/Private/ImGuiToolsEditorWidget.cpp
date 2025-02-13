// Distributed under the MIT License (MIT) (see accompanying LICENSE file)
 
#include "ImGuiToolsEditorWidget.h"

#include "ImGuiToolWindow.h"

SImGuiToolsEditorWidget::~SImGuiToolsEditorWidget()
{
}
 
void SImGuiToolsEditorWidget::Construct(const FArguments& InArgs)
{
	ToolWindowPtr = InArgs._ToolWindow;
	if (ToolWindowPtr.IsValid())
	{
		WindowFlags = WindowFlags | ToolWindowPtr->GetFlags();
	}
}

void SImGuiToolsEditorWidget::ImGuiUpdate()
{
	if (ToolWindowPtr.IsValid())
	{
		ToolWindowPtr->ImGuiUpdate(0.001f);
	}
}
