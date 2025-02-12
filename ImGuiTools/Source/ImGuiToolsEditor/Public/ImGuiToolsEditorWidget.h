// Distributed under the MIT License (MIT) (see accompanying LICENSE file)
 
#pragma once
 
#include "ImGuiEditorWidget.h"

#include "Templates/SharedPointer.h"

class FImGuiToolWindow;

class SImGuiToolsEditorWidget : public SImGuiEditorWidget
{
public:
	SLATE_BEGIN_ARGS(SImGuiToolsEditorWidget)
		: _ToolWindow(nullptr)
		{}
		SLATE_ARGUMENT(TSharedPtr<FImGuiToolWindow>, ToolWindow)
	SLATE_END_ARGS()
 
	virtual ~SImGuiToolsEditorWidget();
 
	void Construct(const FArguments& InArgs);

	// The main function to override to draw ImGui things in this Slate Widget.
	virtual void ImGuiUpdate() override;

protected:
	TSharedPtr<FImGuiToolWindow> ToolWindowPtr = nullptr;
};
