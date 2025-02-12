// Distributed under the MIT License (MIT) (see accompanying LICENSE file)
 
#include "ImGuiEditorWidget.h"

#include <imgui_internal.h>
 
#include "ImGuiContext.h"
#include "ImGuiModule.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

SImGuiEditorWidget::~SImGuiEditorWidget()
{
}
 
void SImGuiEditorWidget::Construct(const FArguments& InArgs)
{
	SetCanTick(true); // Ensure the widget updates
	SetVisibility(EVisibility::HitTestInvisible); // Ensures the widget is interactive but doesn't block transparency
}

void SImGuiEditorWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	TSharedPtr<FImGuiContext> ImGuiContext = FImGuiModule::Get().FindOrCreateSessionContext(GPlayInEditorID);//, ParentWindow);
	
	if (ImGuiContext.IsValid())
	{
		ImGui::SetCurrentContext(*ImGuiContext);

		if (ImGui::GetCurrentContext()->WithinFrameScope)
		{
			const ImVec2 WindowPos = ImVec2(GetCachedGeometry().GetAbsolutePosition());
			const ImVec2 WindowSize = ImVec2(AllottedGeometry.GetAbsoluteSize());
			ImGui::GetCurrentContext()->CurrentViewport->Pos = WindowPos;
			ImGui::GetCurrentContext()->CurrentViewport->Size = WindowSize;
			
			ImGuiContext->BeginFrame();

			// Begin a full-screen ImGui window
			ImGui::SetNextWindowPos(WindowPos); // Top-left corner
			ImGui::SetNextWindowSize(WindowSize); // Full size

			// Define window flags to make it behave like a full-screen panel
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
										   ImGuiWindowFlags_NoResize |
										   ImGuiWindowFlags_NoMove;

			ImGui::Begin("ImGuiEditorWidgetRoot", nullptr, windowFlags);

			ImGuiUpdate();

			if ((CachedWindowPosition.x != WindowPos.x) || (CachedWindowPosition.y != WindowPos.y) || (CachedWindowSize.x != WindowSize.x) || (CachedWindowSize.y != WindowSize.y))
			{
				CachedWindowPosition = WindowPos;
				CachedWindowSize = WindowSize;
				
				FImGuiViewportData* ViewportData = FImGuiViewportData::GetOrCreate(ImGui::GetWindowViewport());
				if (ViewportData && ViewportData->Window.IsValid())
				{
					ViewportData->Window.Pin()->BringToFront(true);
				}
			}
			ImGui::End();

			ImGuiContext->EndFrame();
		}
	}	
}

void SImGuiEditorWidget::ImGuiUpdate()
{
	ImGui::Text("Subclass this widget and override \n SImGuiEditorWidget::ImGuiUpdate to draw ImGui on it!");
}