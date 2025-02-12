// Distributed under the MIT License (MIT) (see accompanying LICENSE file)
 
#pragma once
 
#include "Widgets/SCompoundWidget.h"
#include <imgui.h>
 
// forward declarations
class FImGuiContext;
class SImGuiOverlay;
class UTextureRenderTarget2D;
 
class SImGuiEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SImGuiEditorWidget) {}
	SLATE_END_ARGS()
 
	virtual ~SImGuiEditorWidget();
 
	void Construct(const FArguments& InArgs);

	// The main function to override to draw ImGui things in this Slate Widget.
	virtual void ImGuiUpdate();

protected:
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
 
private:
	ImVec2 CachedWindowPosition = ImVec2(0, 0);
	ImVec2 CachedWindowSize = ImVec2(0, 0);
};
