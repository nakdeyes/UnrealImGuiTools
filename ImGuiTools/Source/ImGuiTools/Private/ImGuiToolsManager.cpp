// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsManager.h"

#include "ImGuiTools.h"
#include "ImGuiToolsDeveloperSettings.h"
#include "Tools/ImGuiActorComponentDebugger.h"
#include "Tools/ImGuiCDOExplorer.h"
#include "Tools/ImGuiFileLoadDebugger.h"
#include "Tools/ImGuiMemoryDebugger.h"
#include "Tools/ImGuiShaderCompilationInfo.h"

#include <Engine/Console.h>
#include <ImGuiModule.h>

#include "ImGuiToolsGameDebugger.h"

#if DRAW_IMGUI_TOOLS
#include "ImGuiTools.h"
#include "Utils/ImGuiUtils.h"
#include <imgui.h>
#endif	  // #if DRAW_IMGUI_TOOLS

DEFINE_LOG_CATEGORY_STATIC(LogImGuiToolsManager, Log, All);

// CVARs
TAutoConsoleVariable<bool> ImGuiDebugCVars::CVarImGuiToolsEnabled(TEXT("imgui.tools.enabled"), false, TEXT("If true, draw ImGui Debug tools."));
static FString ToggleToolVisCVARName = TEXT("imgui.tools.toggle_tool_vis");
FAutoConsoleCommand ToggleToolVis(*ToggleToolVisCVARName, TEXT("Toggle the visibility of a particular ImGui Debug Tool by providing it's string name as an argument"),
								  FConsoleCommandWithArgsDelegate::CreateStatic(&FImGuiToolsManager::ToggleToolVisCommand));
FAutoConsoleCommand ToggleToolsVis(TEXT("imgui.tools.toggle_enabled"), TEXT("Master toggle for ImGui Tools. Will also toggle on ImGui drawing and input, so this is great if ImGui Tools is your main interactions with ImGui."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FImGuiToolsManager::ToggleToolsVisCommand));

FImGuiToolsManager::FImGuiToolsManager()
	: DrawImGuiDemo(false)
	, ShowFPS(true)
{
	UConsole::RegisterConsoleAutoCompleteEntries.AddRaw(this, &FImGuiToolsManager::RegisterAutoCompleteEntries);
	
	OnEnabledCVarValueChanged.BindLambda([](IConsoleVariable* CVar) {
		if (!GetDefault<UImGuiToolsDeveloperSettings>()->SetImGuiInputOnToolsEnabled)
		{
			return;
		}

		if (FImGuiModule* ImGuiModule = FModuleManager::GetModulePtr<FImGuiModule>("ImGui"))
		{
			ImGuiModule->SetInputMode(CVar->GetBool());
		}
	});
	ImGuiDebugCVars::CVarImGuiToolsEnabled->SetOnChangedCallback(OnEnabledCVarValueChanged);
}

FImGuiToolsManager::~FImGuiToolsManager()
{
	UConsole::RegisterConsoleAutoCompleteEntries.RemoveAll(this);
}

TStatId FImGuiToolsManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FImGuiToolsManager, STATGROUP_Tickables);
}

ETickableTickType FImGuiToolsManager::GetTickableTickType() const
{
#if DRAW_IMGUI_TOOLS
	return ETickableTickType::Conditional;
#else
	return ETickableTickType::Never;
#endif	  // #if DRAW_IMGUI_TOOLS
}

bool FImGuiToolsManager::IsTickable() const
{
	return ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread();
}

void FImGuiToolsManager::Initialize()
{
#if DRAW_IMGUI_TOOLS
	static const FName PluginToolsNamespace = TEXT("Included Plugin Tools");
	RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiActorComponentDebugger()), PluginToolsNamespace);
    RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiCDOExplorer()), PluginToolsNamespace);
	RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiFileLoadDebugger()), PluginToolsNamespace);
	RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiMemoryDebugger()), PluginToolsNamespace);
	RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiShaderCompilationInfo()), PluginToolsNamespace);

	if (!InputProcessor)
	{
		InputProcessor = MakeShared<FImGuiToolsInputProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
#endif	  // #if DRAW_IMGUI_TOOLS
}

void FImGuiToolsManager::Deinitialize()
{
#if DRAW_IMGUI_TOOLS
	if (InputProcessor)
	{
		InputProcessor = nullptr;
	}
#endif	  // #if DRAW_IMGUI_TOOLS
}

void FImGuiToolsManager::RegisterToolWindow(TSharedPtr<FImGuiToolWindow> ToolWindow, FName ToolNamespace /*= NAME_None*/)
{
#if DRAW_IMGUI_TOOLS
	TArray<TSharedPtr<FImGuiToolWindow>>& NamespaceTools = ToolWindows.FindOrAdd(ToolNamespace);
	NamespaceTools.Add(ToolWindow);
#endif	  // #if DRAW_IMGUI_TOOLS
}

void FImGuiToolsManager::Tick(float DeltaTime)
{
#if DRAW_IMGUI_TOOLS
	// Draw Main Menu Bar
	if (ImGui::BeginMainMenuBar())
	{
		if (FImGuiModule* Module = FModuleManager::GetModulePtr<FImGuiModule>("ImGui"))
		{
			if (ImGui::BeginMenu("ImGui"))
			{
				bool InputCheckboxVal = Module->GetProperties().IsInputEnabled();
				ImGui::Checkbox("Input Enabled", &InputCheckboxVal);
				if (InputCheckboxVal != Module->GetProperties().IsInputEnabled())
				{
					Module->GetProperties().ToggleInput();
				}

				if (ImGui::BeginMenu("Input Options"))
				{
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Keyboard");
				
					InputCheckboxVal = Module->GetProperties().IsKeyboardNavigationEnabled();
					ImGui::Checkbox("Keyboard Navigation", &InputCheckboxVal);
					if (InputCheckboxVal != Module->GetProperties().IsKeyboardNavigationEnabled())
					{
						Module->GetProperties().ToggleKeyboardNavigation();
					}
					InputCheckboxVal = Module->GetProperties().IsKeyboardInputShared();
					ImGui::Checkbox("Keyboard Input Shared", &InputCheckboxVal);
					if (InputCheckboxVal != Module->GetProperties().IsKeyboardInputShared())
					{
						Module->GetProperties().ToggleKeyboardInputSharing();
					}
					InputCheckboxVal = Module->GetProperties().IsMouseInputShared();
					ImGui::Checkbox("Mouse Input Shared", &InputCheckboxVal);
					if (InputCheckboxVal != Module->GetProperties().IsMouseInputShared())
					{
						Module->GetProperties().ToggleMouseInputSharing();
					}

					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Gamepad");

					InputCheckboxVal = Module->GetProperties().IsGamepadNavigationEnabled();
					ImGui::Checkbox("Gameplay Navigation", &InputCheckboxVal);
					if (InputCheckboxVal != Module->GetProperties().IsGamepadNavigationEnabled())
					{
						Module->GetProperties().ToggleGamepadNavigation();
					}
					InputCheckboxVal = Module->GetProperties().IsGamepadInputShared();
					ImGui::Checkbox("Gameplay Input Shared", &InputCheckboxVal);
					if (InputCheckboxVal != Module->GetProperties().IsGamepadInputShared())
					{
						Module->GetProperties().ToggleGamepadInputSharing();
					}

					ImGui::EndMenu();
				}

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Misc");
				ImGui::Checkbox("Draw ImGui demo", &DrawImGuiDemo);
				ImGui::EndMenu();
			}

		}

		if (ImGui::BeginMenu("UETools"))
		{
			if (ImGui::BeginMenu("Options"))
			{
				ImGui::Checkbox("Show FPS", &ShowFPS);
				ImGui::EndMenu();
			}

			for (TPair<FName, TArray<TSharedPtr<FImGuiToolWindow>>>& NamespaceToolWindows : ToolWindows)
			{
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", Ansi(*NamespaceToolWindows.Key.ToString()));
				TArray<TSharedPtr<FImGuiToolWindow>>& NamespaceTools = NamespaceToolWindows.Value;
				for (TSharedPtr<FImGuiToolWindow>& ToolWindow : NamespaceTools)
				{
					const FString ToolName = ToolWindow->GetToolName();
					ImGui::Checkbox(Ansi(*ToolName), &ToolWindow->GetEnabledRef());
				}
				ImGui::Separator();
			}
			ImGui::EndMenu();
		}

		// Draw Game Debugger menu
		FImGuiToolsGameDebugger::DrawMainImGuiMenu();
		
		if (ShowFPS)
		{
			ImGui::SameLine(180.0f);
			const int FPS = static_cast<int>(1.0f / DeltaTime);
			const float Millis = DeltaTime * 1000.0f;
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%03d FPS %.02f ms", FPS, Millis);
		}

		ImGui::SameLine(310.0f);
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "'imgui.toggleinput' to toggle input, or set HotKey in ImGui Plugin prefs.");

		ImGui::EndMainMenuBar();
	}

	if (DrawImGuiDemo)
	{
		// Draw imgui demo window if requested
		ImGui::ShowDemoWindow(&DrawImGuiDemo);
	}

	// Update any active tools:
	for (TPair<FName, TArray<TSharedPtr<FImGuiToolWindow>>>& NamespaceToolWindows : ToolWindows)
	{
		for (TSharedPtr<FImGuiToolWindow>& ToolWindow : NamespaceToolWindows.Value)
		{
			if (ToolWindow.IsValid())
			{
				ToolWindow->UpdateTool(DeltaTime);
			}
		}
	}
#endif	  // #if DRAW_IMGUI_TOOLS
}

ToolNamespaceMap& FImGuiToolsManager::GetToolsWindows()
{
	return ToolWindows;
}

TSharedPtr<FImGuiToolWindow> FImGuiToolsManager::GetToolWindow(const FString& ToolWindowName, FName ToolNamespace /*= NAME_None*/)
{
	const bool UseNamespace = (ToolNamespace != NAME_None);
	for (const TPair<FName, TArray<TSharedPtr<FImGuiToolWindow>>>& NamespaceToolWindows : ToolWindows)
	{
		if (!UseNamespace || (ToolNamespace == NamespaceToolWindows.Key))
		{
			const TArray<TSharedPtr<FImGuiToolWindow>>& NamespaceTools = NamespaceToolWindows.Value;
			for (const TSharedPtr<FImGuiToolWindow>& ToolWindow : NamespaceTools)
			{
				if (!ToolWindow.IsValid())
				{
					continue;
				}

				FString ToolWindowStringName(ToolWindow->GetToolName());
				if (ToolWindow->GetToolName() == ToolWindowName)
				{
					return ToolWindow;
				}
			}
		}
	}

	return nullptr;
}

void FImGuiToolsManager::ToggleToolVisCommand(const TArray<FString>& Args)
{
#if DRAW_IMGUI_TOOLS
	FImGuiToolsModule& ImGuiToolsModule = FModuleManager::GetModuleChecked<FImGuiToolsModule>("ImGuiTools");
	if (!ImGuiToolsModule.GetToolsManager().IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.toggle_tool_vis' was unable to find the ImGuiTools Module."));
		return;
	}

	if (Args.Num() < 1)
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.toggle_tool_vis' must be provided a string parameter tool name to toggle visibility."));
		return;
	}

	FString ToolWindowName(TEXT(""));
	for (int i = 0; i < Args.Num(); ++i)
	{
		ToolWindowName += Args[i];
		const bool IsLast = (i == (Args.Num() - 1));
		if (!IsLast)
		{
			ToolWindowName += TEXT(" ");
		}
	}

	TSharedPtr<FImGuiToolWindow> FoundTool = ImGuiToolsModule.GetToolsManager()->GetToolWindow(ToolWindowName);
	if (!FoundTool.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.toggle_tool_vis' was unable to find a tool with name: '%s'"), *ToolWindowName);
		return;
	}

    const bool NewToolEnabled = !FoundTool->GetEnabledRef();
    FoundTool->EnableTool(NewToolEnabled);

    if (NewToolEnabled)
    {
        // New tool enabled, flick on ImGui Tools enabled if not already set. 
		ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->Set(true);
    }
    else
    {
        // Tool now disabled.. check to see if all are closed and if so - close ImGuiTools
        bool AnyToolEnabled = false;
        ToolNamespaceMap& AllToolWindows = ImGuiToolsModule.GetToolsManager()->GetToolsWindows();
        for (const TPair<FName, TArray<TSharedPtr<FImGuiToolWindow>>>& NamespaceTools : AllToolWindows)
        {
            for (const TSharedPtr<FImGuiToolWindow>& ToolSharedPtr : NamespaceTools.Value)
            {
                if (ToolSharedPtr.IsValid())
                {
                    AnyToolEnabled |= ToolSharedPtr->GetEnabledRef();
                }
            }
        }

        if (!AnyToolEnabled)
        {
            ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->Set(false);
        }
    }
#endif	  // #if DRAW_IMGUI_TOOLS
}

void FImGuiToolsManager::ToggleToolsVisCommand(const TArray<FString>& Args)
{
#if DRAW_IMGUI_TOOLS
    ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->Set(!ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread());
#endif	  // #if DRAW_IMGUI_TOOLS
}

void FImGuiToolsManager::RegisterAutoCompleteEntries(TArray<FAutoCompleteCommand>& Commands) const
{
#if DRAW_IMGUI_TOOLS
	for (const TPair<FName, TArray<TSharedPtr<FImGuiToolWindow>>>& NamespaceToolWindows : ToolWindows)
	{
		const FName& NamespaceName = NamespaceToolWindows.Key;
		for (const TSharedPtr<FImGuiToolWindow>& ToolWindow : NamespaceToolWindows.Value)
		{
			FAutoCompleteCommand Entry;
			const FString ToolName = ToolWindow->GetToolName();
			Entry.Command = FString::Printf(TEXT("%s %s"), *ToggleToolVisCVARName, *ToolName);
			Entry.Desc = FString::Printf(TEXT("Toggle visibility for ImGui Tool '%s' in namespace '%s'"), *ToolName, *NamespaceName.ToString());
			Commands.Add(Entry);
		}
	}
#endif // #if DRAW_IMGUI_TOOLS
}

bool FImGuiToolsInputProcessor::HandleKeyDownEvent( FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent )
{
#if DRAW_IMGUI_TOOLS
	if (!ToggleInputDown)
	{
		if (GetDefault<UImGuiToolsDeveloperSettings>()->ImGuiToggleInputKeys.ContainsByPredicate([InKeyEvent](const FKey& Key) { return (InKeyEvent.GetKey().GetFName() == Key.GetFName()); }))
		{
			KeyCodesDown.AddUnique(InKeyEvent.GetKeyCode());
			if (KeyCodesDown.Num() == GetDefault<UImGuiToolsDeveloperSettings>()->ImGuiToggleInputKeys.Num())
			{
				ToggleInputDown = true;

				if (FImGuiModule* Module = FModuleManager::GetModulePtr<FImGuiModule>("ImGui"))
				{
					UE_LOG(LogImGuiToolsManager, Log, TEXT("ImGuiTools - Toggling Input"));
					Module->GetProperties().ToggleInput();
				}
			}
		}
	}
#endif // #if DRAW_IMGUI_TOOLS
	
	return false;
}

bool FImGuiToolsInputProcessor::HandleKeyUpEvent( FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent )
{
#if DRAW_IMGUI_TOOLS
	if (GetDefault<UImGuiToolsDeveloperSettings>()->ImGuiToggleInputKeys.ContainsByPredicate([InKeyEvent](const FKey& Key) { return (InKeyEvent.GetKey().GetFName() == Key.GetFName()); }))
	{
		KeyCodesDown.Remove(InKeyEvent.GetKeyCode());
		ToggleInputDown = false;
	}
#endif // #if DRAW_IMGUI_TOOLS
	
	return false;
}

void FImGuiToolsInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
}