// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsEditorCommands.h"

#include "ImGuiTools.h"
#include "ImGuiToolsDeveloperSettings.h"
#include "ImGuiToolsManager.h"
#include "ImGuiToolWindow.h"

#include <ImGuiModule.h>
#include <Internationalization/Text.h>
#include <LevelEditor.h>

#define ADD_PM_IMGUI_WIDGETS 1

#if ADD_PM_IMGUI_WIDGETS
#include "Runtime/Engine/Classes/GameFramework/PlayerController.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#endif

#define LOCTEXT_NAMESPACE "FImGuiToolsModule"

void FImGuiToolsEditorCommands::RegisterCommands()
{
	UI_COMMAND(ImGuiToolEnabledCommand, "ImGuiTools Enabled", "When enabled, the ImGui Tools menu bar will be drawn on screen. Can also be controlled via CVAR 'imgui.tools.enabled'.", EUserInterfaceActionType::ToggleButton, FInputGesture());
}

void FImGuiToolsEditorElements::RegisterElements()
{
	CommandList = MakeShareable(new FUICommandList);

	/*CommandList->MapAction(FImGuiToolsEditorCommands::Get().ImGuiToolEnabledCommand,
						   FExecuteAction::CreateRaw(this, &FImGuiToolsEditorElements::Callback_ImGuiToolEnabledCommand_Toggled), FCanExecuteAction(),
						   FIsActionChecked::CreateRaw(this, &FImGuiToolsEditorElements::Callback_ImGuiToolEnabledCommand_IsChecked));*/

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
#if ADD_PM_IMGUI_WIDGETS
                    //
                    // The goal of this change is to add custom PMImGui widgets here so that all ImGui functionality
                    // is located in one submenu.
                    // 
                    // Although Unreal provides an extension hook mechanism to add an entry to the submenu, this case is more complex since 
                    // here the extension needs to be added to the submenu of the custom toolbar button. 
                    // However, the default extension scenarios usually fall under one of the following:
                    //
                    // a) Use the level editor toolbar extender to add a new toolbar button
                    // b) Use the level editor menu extender to add a new menu entry
                    //
                    // Something that is needed is a mix of these two - add a new menu entry to an exiting toolbar button.
                    // There are instances of this, that incorporate the ToolMenus module.
                    // However, those require a slightly different implementation.
                    // 
                    // This way, to keep the plugin changes to a minimum, widgets are added here.
                    // There are only 2 changes to the Plugin code - this block and includes in this source file.
                    // They can be easily removed when needed.
                    //
                    // Custom widgets are added using the following mechanism:
                    // 
                    //  - There are no explicit calls to PMImGui API. PMImGuiWidgetHandler allows adding registered widgets using console commands,
                    //    so it is sufficient to run 'ConsoleCommand' on a player controller;
                    //  - PMImGui widgets are only valid in the PIE/Game context. This way, every FUIAction is implemented so that:
                    //      * When executed in the Editor context (no active PIE session), a corresponding ConsoleCommand is scheduled so that the widget 
                    //        is activated when the PIE session is initialized (first using the FEditorDelegates::PostPIEStarted to obtain an instance of PIEWorld,
                    //        then PIEWorld->OnWorldMatchStarting to obtain a valid player controller);
                    //      * When executed in the PIE context, a command is invoked immediately
                    //
                    MenuBuilder.BeginSection(FName(TEXT("ImGuiTools.PMImGuiWidgets")), FText::FromString(TEXT("PMImGuiWidgets")));
                    static bool sWidgetHandlerChecked = false;
                    static const TCHAR* kShowWidgetHandlerCommand = TEXT("ToggleWidgetHandler"); 
                    MenuBuilder.AddMenuEntry(
                        FText::FromString(FString("Widget Handler")), 
                        FText(), 
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]()
                            {
                                sWidgetHandlerChecked = !sWidgetHandlerChecked;
                                FWorldContext* WorldContext = IsValid(GEditor) ? GEditor->GetPIEWorldContext() : nullptr;
                                UWorld*        PIEWorld = nullptr;
                                if (WorldContext)
                                {
                                    PIEWorld = GEditor->GetPIEWorldContext()->World();
                                }
                                // During active PIE
                                if (IsValid(PIEWorld))
                                {
                                    PIEWorld->GetFirstPlayerController()->ConsoleCommand(kShowWidgetHandlerCommand, true);
                                }
                                // No active PIE
                                else
                                {
                                    FDelegateHandle PIEPostStartHandle = FEditorDelegates::PostPIEStarted.AddLambda([&PIEPostStartHandle](const bool)
                                    {
                                        FEditorDelegates::PostPIEStarted.Remove(PIEPostStartHandle);
                                        UWorld* PIEWorld = GEditor->GetPIEWorldContext()->World();
                                        if (IsValid(PIEWorld))
                                        {
                                            FDelegateHandle WorldMatchStartingHandle = PIEWorld->OnWorldMatchStarting
                                                .AddLambda([PIEWorld, &WorldMatchStartingHandle]()
                                                {
                                                    PIEWorld->OnWorldMatchStarting.Remove(WorldMatchStartingHandle);
                                                    if (sWidgetHandlerChecked)
                                                    {
                                                        PIEWorld->GetFirstPlayerController()->ConsoleCommand(kShowWidgetHandlerCommand, true);
                                                    }
                                                }
                                            );
                                        }
                                       
                                    });
                                    FDelegateHandle OnPreExitHandle = FCoreDelegates::OnPreExit.AddLambda([&PIEPostStartHandle, &OnPreExitHandle]()
                                    {
                                        FEditorDelegates::PostPIEStarted.Remove(PIEPostStartHandle);
                                        FCoreDelegates::OnPreExit.Remove(OnPreExitHandle);
                                    });
                                }  
                            }),
                        FCanExecuteAction(),
                        FIsActionChecked::CreateLambda([]()
                        {
                            return sWidgetHandlerChecked;
                        }
                        )),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    static bool sAnimationDebugChecked             = false;
                    static const TCHAR* kShowAnimationDebugCommand = TEXT("ShowImGuiDebugWidgetByDrawFunctionName AnimationDebug"); 
                    MenuBuilder.AddMenuEntry(
                        FText::FromString(FString("Animation Debug")), 
                        FText(), 
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]()
                            {
                                sAnimationDebugChecked = !sAnimationDebugChecked;
                                FWorldContext* WorldContext = IsValid(GEditor) ? GEditor->GetPIEWorldContext() : nullptr;
                                UWorld*        PIEWorld = nullptr;
                                if (WorldContext)
                                {
                                    PIEWorld = GEditor->GetPIEWorldContext()->World();
                                }
                      
                                // During active PIE
                                if (IsValid(PIEWorld))
                                {
                                    PIEWorld->GetFirstPlayerController()->ConsoleCommand(kShowAnimationDebugCommand, true);
                                }
                                // No active PIE
                                else
                                {
                                    FDelegateHandle PIEPostStartHandle = FEditorDelegates::PostPIEStarted.AddLambda([&PIEPostStartHandle](const bool)
                                    {
                                        FEditorDelegates::PostPIEStarted.Remove(PIEPostStartHandle);
                                        UWorld* PIEWorld = GEditor->GetPIEWorldContext()->World();
                                        if (IsValid(PIEWorld))
                                        {
                                            FDelegateHandle WorldMatchStartingHandle = PIEWorld->OnWorldMatchStarting
                                                .AddLambda([PIEWorld, &WorldMatchStartingHandle]()
                                                {
                                                    PIEWorld->OnWorldMatchStarting.Remove(WorldMatchStartingHandle);
                                                    if (sAnimationDebugChecked)
                                                    {
                                                        PIEWorld->GetFirstPlayerController()->ConsoleCommand(kShowAnimationDebugCommand, true);
                                                    }
                                                }
                                            );
                                        }
                                    });
                                    FDelegateHandle OnPreExitHandle = FCoreDelegates::OnPreExit.AddLambda([&PIEPostStartHandle, &OnPreExitHandle]()
                                    {
                                        FEditorDelegates::PostPIEStarted.Remove(PIEPostStartHandle);
                                        FCoreDelegates::OnPreExit.Remove(OnPreExitHandle);
                                    });
                                } 
                            }),
                        FCanExecuteAction(),
                        FIsActionChecked::CreateLambda([]()
                        {
                            return sAnimationDebugChecked;
                        }
                        )),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );
                    
                    static bool VODebugChecked = false;
                    MenuBuilder.AddMenuEntry(
                        FText::FromString(FString("VO Debug")), 
                        FText(), 
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]()
                            {
                                VODebugChecked= !VODebugChecked;
                                FWorldContext* WorldContext = IsValid(GEditor) ? GEditor->GetPIEWorldContext() : nullptr;
                                UWorld*        PIEWorld = nullptr;
                                if (WorldContext)
                                {
                                    PIEWorld = GEditor->GetPIEWorldContext()->World();
                                }
                      
                                // During active PIE
                                if (IsValid(PIEWorld))
                                {
                                    PIEWorld->GetFirstPlayerController()->ConsoleCommand(TEXT("ShowImGuiDebugWidgetByDrawFunctionName VoiceOverDispatchDebugImGUI"), true);
                                }
                                // No active PIE
                                else
                                {
                                    FDelegateHandle PIEPostStartHandle = FEditorDelegates::PostPIEStarted.AddLambda([&PIEPostStartHandle](const bool)
                                    {
                                        FEditorDelegates::PostPIEStarted.Remove(PIEPostStartHandle);
                                        UWorld* PIEWorld = GEditor->GetPIEWorldContext()->World();
                                        if (IsValid(PIEWorld))
                                        {
                                            FDelegateHandle WorldMatchStartingHandle = PIEWorld->OnWorldMatchStarting
                                                .AddLambda([PIEWorld, &WorldMatchStartingHandle]()
                                                {
                                                    PIEWorld->OnWorldMatchStarting.Remove(WorldMatchStartingHandle);
                                                    if (VODebugChecked)
                                                    {
                                                        PIEWorld->GetFirstPlayerController()->ConsoleCommand(TEXT("ShowImGuiDebugWidgetByDrawFunctionName VoiceOverDispatchDebugImGUI"), true);
                                                    }
                                                }
                                            );
                                        }
                                    });
                                    FDelegateHandle OnPreExitHandle = FCoreDelegates::OnPreExit.AddLambda([&PIEPostStartHandle, &OnPreExitHandle]()
                                    {
                                        FEditorDelegates::PostPIEStarted.Remove(PIEPostStartHandle);
                                        FCoreDelegates::OnPreExit.Remove(OnPreExitHandle);
                                    });
                                }    
                            }),
                        FCanExecuteAction(),
                        FIsActionChecked::CreateLambda([]()
                        {
                            return VODebugChecked;
                        }
                        )),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );
					MenuBuilder.EndSection();
 #endif
				}
			}
		}

		static void FillImGuiInputModuleSubmenu(FMenuBuilder& MenuBuilder, FImGuiModule* ImGuiModule)
		{
			MenuBuilder.AddMenuEntry(LOCTEXT("ImGui_InputEnabled", "Input Enabled"), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([ImGuiModule]() { ImGuiModule->GetProperties().ToggleInput(); }), FCanExecuteAction(),
				FIsActionChecked::CreateLambda([ImGuiModule]() -> bool { return ImGuiModule->GetProperties().IsInputEnabled(); })), NAME_None, EUserInterfaceActionType::ToggleButton);

			MenuBuilder.BeginSection(NAME_None, LOCTEXT("KeyboardMouse", "Keyboard & Mouse"));
			MenuBuilder.AddMenuEntry(LOCTEXT("ImGui_KeyboardNav", "Keyboard Navigation"), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([ImGuiModule]() { ImGuiModule->GetProperties().ToggleKeyboardNavigation(); }), FCanExecuteAction(),
					FIsActionChecked::CreateLambda([ImGuiModule]() -> bool { return ImGuiModule->GetProperties().IsKeyboardNavigationEnabled(); })), NAME_None, EUserInterfaceActionType::ToggleButton);

			MenuBuilder.AddMenuEntry(LOCTEXT("ImGui_KeyboardInputShared", "Keyboard Input Shared"), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([ImGuiModule]() { ImGuiModule->GetProperties().ToggleKeyboardInputSharing(); }), FCanExecuteAction(),
					FIsActionChecked::CreateLambda([ImGuiModule]() -> bool { return ImGuiModule->GetProperties().IsKeyboardInputShared(); })), NAME_None, EUserInterfaceActionType::ToggleButton);

			MenuBuilder.AddMenuEntry(LOCTEXT("ImGui_MouseInputShared", "Mouse Input Shared"), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([ImGuiModule]() { ImGuiModule->GetProperties().ToggleMouseInputSharing(); }), FCanExecuteAction(),
					FIsActionChecked::CreateLambda([ImGuiModule]() -> bool { return ImGuiModule->GetProperties().IsMouseInputShared(); })), NAME_None, EUserInterfaceActionType::ToggleButton);
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection(NAME_None, LOCTEXT("Gamepad", "Gamepad"));

			MenuBuilder.AddMenuEntry(LOCTEXT("ImGui_GamepadNav", "Gamepad Navigation"), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([ImGuiModule]() { ImGuiModule->GetProperties().ToggleGamepadNavigation(); }), FCanExecuteAction(),
					FIsActionChecked::CreateLambda([ImGuiModule]() -> bool { return ImGuiModule->GetProperties().IsGamepadNavigationEnabled(); })), NAME_None, EUserInterfaceActionType::ToggleButton);

			MenuBuilder.AddMenuEntry(LOCTEXT("ImGui_KeyboardInputShared", "Gamepad Input Shared"), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([ImGuiModule]() { ImGuiModule->GetProperties().ToggleGamepadInputSharing(); }), FCanExecuteAction(),
					FIsActionChecked::CreateLambda([ImGuiModule]() -> bool { return ImGuiModule->GetProperties().IsGamepadInputShared(); })), NAME_None, EUserInterfaceActionType::ToggleButton);

			MenuBuilder.EndSection();
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
				MenuBuilder.AddSubMenu(LOCTEXT("ImGui_Input", "Input"), FText(), FMenuExtensionDelegate::CreateStatic(&Local::FillImGuiInputModuleSubmenu, ImGuiModule));
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

        TSharedPtr <FExtender> NewToolbarExtender = MakeShareable(new FExtender);
        NewToolbarExtender->AddToolBarExtension("Play", EExtensionHook::After, CommandList, FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, CommandList));

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NewToolbarExtender);
	}
}

#undef LOCTEXT_NAMESPACE
