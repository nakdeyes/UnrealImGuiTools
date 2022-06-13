// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsEditorCommands.h"

#include "ImGuiTools.h"
#include "ImGuiToolsManager.h"
#include "ImGuiToolWindow.h"

#include <LevelEditor.h>
#include <Internationalization/Text.h>

#define LOCTEXT_NAMESPACE "FImGuiToolsModule"

void FImGuiToolsEditorCommands::RegisterCommands()
{
	UI_COMMAND(ImGuiToolEnabledCommand, "ImGuiTools Enabled", "When enabled, the ImGui Tools menu bar will be drawn on screen. Can also be controlled via CVAR 'imgui.tools.enabled'.", EUserInterfaceActionType::ToggleButton, FInputGesture());
}

void FImGuiToolsEditorElements::RegisterElements()
{
	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(FImGuiToolsEditorCommands::Get().ImGuiToolEnabledCommand,
						   FExecuteAction::CreateRaw(this, &FImGuiToolsEditorElements::Callback_ImGuiToolEnabledCommand_Toggled), FCanExecuteAction(),
						   FIsActionChecked::CreateRaw(this, &FImGuiToolsEditorElements::Callback_ImGuiToolEnabledCommand_IsChecked));

	struct Local
	{
		static void FillToolWindowsSubmenu(FMenuBuilder& MenuBuilder)
		{
			FImGuiToolsModule& ImGuiToolsModule = FModuleManager::GetModuleChecked<FImGuiToolsModule>("ImGuiTools");
			if (ImGuiToolsModule.GetToolsManager().IsValid())
			{
				ToolNamespaceMap& ToolsWindows = ImGuiToolsModule.GetToolsManager()->GetToolsWindows();
				for (const TPair<FName, TArray<TSharedPtr<FImGuiToolWindow>>>& NamespaceToolWindows : ToolsWindows)
				{
					MenuBuilder.BeginSection("ImGuiTools", FText::FromName(NamespaceToolWindows.Key));
					const TArray<TSharedPtr<FImGuiToolWindow>>& NamespaceTools = NamespaceToolWindows.Value;
					for (const TSharedPtr<FImGuiToolWindow>& ToolWindow : NamespaceTools)
					{
						MenuBuilder.AddMenuEntry(FText::FromString(FString(ToolWindow->GetToolName())), FText(), FSlateIcon(), 
							FUIAction(FExecuteAction::CreateStatic(&FImGuiToolsEditorElements::Callback_ImguiToolWindow_Toggle, ToolWindow), FCanExecuteAction(),
									  FIsActionChecked::CreateStatic(&FImGuiToolsEditorElements::Callback_ImguiToolWindow_IsChecked, ToolWindow)), NAME_None, EUserInterfaceActionType::ToggleButton);
					}
					MenuBuilder.EndSection();
				}
			}
		}

		static TSharedRef<SWidget> FillImGuiToolsMenu(TSharedPtr<FUICommandList> CommandList) 
		{
			FMenuBuilder MenuBuilder(true, CommandList);

			MenuBuilder.AddMenuEntry(FImGuiToolsEditorCommands::Get().ImGuiToolEnabledCommand);

			MenuBuilder.AddSubMenu(LOCTEXT("ImGuiTools_ToolWindows", "Tool Windows"), FText(), FMenuExtensionDelegate::CreateStatic(&Local::FillToolWindowsSubmenu));

			return MenuBuilder.MakeWidget();
		}

		static void FillToolbar(FToolBarBuilder& Builder, TSharedPtr<FUICommandList> CommandList)
		{
			Builder.AddComboButton(FUIAction(), FOnGetContent::CreateStatic(Local::FillImGuiToolsMenu, CommandList), LOCTEXT("ImGuiTools", "ImGuiTools"),
					LOCTEXT("ImGuiTools_Tooltip", "Tools for use with the ImGui plugin."), FSlateIcon(FImGuiToolsEditorStyle::Get().GetStyleSetName(), "ImGuiTools.EditorIcon"));
		}
	};

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedPtr<FExtender> NewToolbarExtender = MakeShareable(new FExtender);
	NewToolbarExtender->AddToolBarExtension("Play", EExtensionHook::After, CommandList, FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, CommandList));

	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NewToolbarExtender);
}

bool FImGuiToolsEditorElements::Callback_ImGuiToolEnabledCommand_IsChecked()
{
	return ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread();
}

void FImGuiToolsEditorElements::Callback_ImGuiToolEnabledCommand_Toggled()
{
	ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->Set(!ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread());
}

bool FImGuiToolsEditorElements::Callback_ImguiToolWindow_IsChecked(TSharedPtr<FImGuiToolWindow> ToolWindow)
{
	return ToolWindow->GetEnabledRef();
}

void FImGuiToolsEditorElements::Callback_ImguiToolWindow_Toggle(TSharedPtr<FImGuiToolWindow> ToolWindow)
{
	ToolWindow->EnableTool(!ToolWindow->GetEnabledRef());
}

#undef LOCTEXT_NAMESPACE
