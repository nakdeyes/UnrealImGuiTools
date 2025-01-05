// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsEditorCommands.h"

#include "ImGuiTools.h"
#include "ImGuiToolsDeveloperSettings.h"
#include "ImGuiToolsManager.h"
#include "ImGuiToolWindow.h"

#include <ImGuiModule.h>
#include <Internationalization/Text.h>
#include <LevelEditor.h>

#define LOCTEXT_NAMESPACE "FImGuiToolsModule"

namespace ImGuiEditorUtils
{
	bool IsModuleLoaded(FName ModuleName)
	{
		FModuleStatus ModuleStatus;
		FModuleManager::Get().QueryModule(ModuleName, ModuleStatus);
        return ModuleStatus.bIsLoaded;
	}

	bool AreRequiredModulesLoaded()
	{
        // Check if modules are loaded..
        const bool RequiredModulesLoaded = (IsModuleLoaded(TEXT("LevelEditor")) && IsModuleLoaded(TEXT("ImGui")));

        // If all required modules are loaded, ensure the level editor extensibility manager is also available. So dumb that this is required, but stems from nuances of
        //  the module manager code, where modules can be not "ready" (and thus extensibilitymanager not init'ed), but still report as loaded. Checking that state is private
        //  to the module manager, so lets just manually check if the level editor modules' extensibility manager is init'ed yet as that is what we need it for.
        bool LevelEditorModuleReady = false;
        if (RequiredModulesLoaded)
        {
            FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
            LevelEditorModuleReady = LevelEditorModule.GetToolBarExtensibilityManager().IsValid();
        }

        return RequiredModulesLoaded && LevelEditorModuleReady;
	}
}

FImGuiToolsEditorCommands::FImGuiToolsEditorCommands()
    : TCommands<FImGuiToolsEditorCommands>(TEXT("ImGuiTools"), NSLOCTEXT("Contexts", "ImGuiTools", "ImGuiTools Plugin"), NAME_None, FImGuiToolsEditorStyle::GetStyleSetName())
{
}

void FImGuiToolsEditorCommands::RegisterCommands()
{
	UI_COMMAND(ImGuiToolEnabledCommand, "ImGuiTools Enabled", "When enabled, the ImGui Tools menu bar will be drawn on screen. Can also be controlled via CVAR 'imgui.tools.enabled'.", EUserInterfaceActionType::ToggleButton, FInputGesture());
}

// Entry point to register elements. Do it now if able, else wait until able then do it.
void FImGuiToolsEditorElements::RegisterElements()
{
	if (ImGuiEditorUtils::AreRequiredModulesLoaded())
	{
		// Ready to register the elements
		RegisterElements_Internal();
	}
	else
	{
		// Module not yet loaded, subscribe and wait for the module to be loaded 
		OnModulesChangedHandler = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FImGuiToolsEditorElements::OnModulesChanged);
	}
}

void FImGuiToolsEditorElements::OnModulesChanged(FName Name, EModuleChangeReason Reason)
{
	if (ImGuiEditorUtils::AreRequiredModulesLoaded())
	{
		// Modules loaded. Unsub from OnModulesChanged and register the elements.
		FModuleManager::Get().OnModulesChanged().Remove(OnModulesChangedHandler);
		OnModulesChangedHandler.Reset();
		RegisterElements_Internal();
	}
}

// Actually registers the elements, assuming the level editor module is present. 
void FImGuiToolsEditorElements::RegisterElements_Internal()
{
	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(FImGuiToolsEditorCommands::Get().ImGuiToolEnabledCommand,
		FExecuteAction::CreateLambda([]() { ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->Set(!ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread()); }), FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]() -> bool { return ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread(); }));

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
					MenuBuilder.BeginSection(FName(FString::Printf(TEXT("ImGuiTools.%s"), *NamespaceToolWindows.Key.ToString())), FText::FromName(NamespaceToolWindows.Key));
					const TArray<TSharedPtr<FImGuiToolWindow>>& NamespaceTools = NamespaceToolWindows.Value;
					for (const TSharedPtr<FImGuiToolWindow>& ToolWindow : NamespaceTools)
					{
						MenuBuilder.AddMenuEntry(FText::FromString(FString(ToolWindow->GetToolName())), FText(), FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([ToolWindow]() { ToolWindow->EnableTool(!ToolWindow->GetEnabledRef()); }), FCanExecuteAction(),
								FIsActionChecked::CreateLambda([ToolWindow]() -> bool { return ToolWindow->GetEnabledRef(); })), NAME_None, EUserInterfaceActionType::ToggleButton);
					}
					MenuBuilder.EndSection();
				}
			}
		}

		static TSharedRef<SWidget> FillImGuiToolsMenu(TSharedPtr<FUICommandList> CommandList) 
		{
			FMenuBuilder MenuBuilder(true, CommandList);

			MenuBuilder.BeginSection(TEXT("ImGuiTools.Main"), LOCTEXT("ImGuiTools", "ImGui Tools"));
			MenuBuilder.AddMenuEntry(FImGuiToolsEditorCommands::Get().ImGuiToolEnabledCommand);
			MenuBuilder.AddSubMenu(LOCTEXT("ImGuiTools_ToolWindows", "Tool Windows"), FText(), FMenuExtensionDelegate::CreateStatic(&Local::FillToolWindowsSubmenu));
			MenuBuilder.EndSection();

			if (FImGuiModule* ImGuiModule = FModuleManager::GetModulePtr<FImGuiModule>("ImGui"))
			{
				MenuBuilder.BeginSection(TEXT("ImGuiTools.ImGuiOptions"), LOCTEXT("ImGuiOptions", "ImGui Options"));
				MenuBuilder.EndSection();
			}

			return MenuBuilder.MakeWidget();
		}

		static void FillToolbar(FToolBarBuilder& Builder, TSharedPtr<FUICommandList> CommandList)
		{
			Builder.AddComboButton(FUIAction(), FOnGetContent::CreateStatic(Local::FillImGuiToolsMenu, CommandList), LOCTEXT("ImGuiTools", "ImGuiTools"),
					LOCTEXT("ImGuiTools_Tooltip", "Tools for use with the ImGui plugin."), FSlateIcon(FImGuiToolsEditorStyle::Get().GetStyleSetName(), "ImGuiTools.EditorIcon"));
		}
	};

	if (GetDefault<UImGuiToolsDeveloperSettings>()->DisplayEditorButton)
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

		TSharedPtr<FExtender> NewToolbarExtender = MakeShareable(new FExtender);
#if ENGINE_MAJOR_VERSION == 4
		NewToolbarExtender->AddToolBarExtension("Build", EExtensionHook::Before, CommandList, FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, CommandList));
#elif ENGINE_MAJOR_VERSION == 5
		NewToolbarExtender->AddToolBarExtension("Play", EExtensionHook::After, CommandList, FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, CommandList));
#endif

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NewToolbarExtender);
	}
}

#undef LOCTEXT_NAMESPACE
