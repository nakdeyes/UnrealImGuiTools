// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsManager.h"

#include "ImGuiToolsDeveloperSettings.h"

#if DRAW_IMGUI_TOOLS

#include <Engine/Console.h>

#include "ImGuiTools.h"
#include "ImGuiToolsGameDebugger.h"
#include "Tools/ImGuiActorComponentDebugger.h"
#include "Tools/ImGuiCDOExplorer.h"
#include "Tools/ImGuiFileLoadDebugger.h"
#include "Tools/ImGuiMemoryDebugger.h"
#include "Tools/ImGuiShaderCompilationInfo.h"
#include "Utils/ImGuiUtils.h"

// ImGui Plugin
#include "ImGuiContext.h"
#include "ImGuiModule.h"
#include <imgui.h>
#include <imgui_internal.h>

DEFINE_LOG_CATEGORY_STATIC(LogImGuiToolsManager, Log, All);

// CVARs
TAutoConsoleVariable<bool> ImGuiDebugCVars::CVarImGuiToolsEnabled(TEXT("imgui.tools.enabled"), false, TEXT("If true, draw ImGui Debug tools."));
static FString ToggleToolVisCVARName = TEXT("imgui.tools.toggle_tool_vis");
FAutoConsoleCommand ToggleToolVis(*ToggleToolVisCVARName, TEXT("Toggle the visibility of a particular ImGui Debug Tool by providing it's string name as an argument"),
								  FConsoleCommandWithArgsDelegate::CreateStatic(&FImGuiToolsManager::ToggleToolVisCommand));
FAutoConsoleCommand ToggleToolsVis(TEXT("imgui.tools.toggle_enabled"), TEXT("Master toggle for ImGui Tools. Will also toggle on ImGui drawing and input, so this is great if ImGui Tools is your main interactions with ImGui."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FImGuiToolsManager::ToggleToolsVisCommand));

#if WITH_EDITOR

static FString LaunchEditorToolCVARName = TEXT("imgui.tools.launch_editor_tool");
FAutoConsoleCommand LaunchEditorTool(*LaunchEditorToolCVARName, TEXT("Launch an editor tool panel by name for tools that support it."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FImGuiToolsManager::LaunchEditorTool));

#endif    // #if WITH_EDITOR

#endif	  // #if DRAW_IMGUI_TOOLS

FImGuiToolsManager::FImGuiToolsManager()
	: DrawImGuiDemo(false)
	, ShowFPS(true)
{
#if DRAW_IMGUI_TOOLS
	UConsole::RegisterConsoleAutoCompleteEntries.AddRaw(this, &FImGuiToolsManager::RegisterAutoCompleteEntries);
#endif	  // #if DRAW_IMGUI_TOOLS
}

FImGuiToolsManager::~FImGuiToolsManager()
{
#if DRAW_IMGUI_TOOLS
	UConsole::RegisterConsoleAutoCompleteEntries.RemoveAll(this);
#endif	  // #if DRAW_IMGUI_TOOLS
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

	if (!InputProcessor && FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FImGuiToolsInputProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
	
	OnWorldPostActorTickDelegateHandle = FWorldDelegates::OnWorldPostActorTick.AddLambda([&](UWorld* World, ELevelTick TickType, float DeltaSeconds)
	{
		OnWorldPostActorTick(World, TickType, DeltaSeconds);
	});
#endif	  // #if DRAW_IMGUI_TOOLS
}

void FImGuiToolsManager::Deinitialize()
{
#if DRAW_IMGUI_TOOLS
	if (OnWorldPostActorTickDelegateHandle.IsValid())
	{
		FWorldDelegates::OnWorldPostActorTick.Remove(OnWorldPostActorTickDelegateHandle);
		OnWorldPostActorTickDelegateHandle.Reset();
	}
	
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

void FImGuiToolsManager::OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
#if DRAW_IMGUI_TOOLS
	if (!ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread())
	{
		return;
	}
	
	if (!IsValid(World) || !World->IsGameWorld() || (World->GetNetMode() == NM_DedicatedServer))
	{
		return;
	}
	
	TSharedPtr<FImGuiContext> ImGuiContext = FImGuiModule::Get().FindOrCreateSessionContext();

	if (ImGuiContext.IsValid())
	{
		ImGui::SetCurrentContext(*ImGuiContext);

		if (ImGui::GetCurrentContext()->WithinFrameScope)
		{
			ImGuiContext->BeginFrame();
			
			// Draw Main Menu Bar
			if (ImGui::BeginMainMenuBar())
			{
				if (FImGuiModule* Module = FModuleManager::GetModulePtr<FImGuiModule>("ImGui"))
				{
					if (ImGui::BeginMenu("ImGui"))
					{
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
					const int FPS = static_cast<int>(1.0f / DeltaSeconds);
					const float Millis = DeltaSeconds * 1000.0f;
					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%03d FPS %.02f ms", FPS, Millis);
				}

				ImGui::SameLine(310.0f);
#if WITH_EDITOR
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "'Shift+F1' to toggle input in editor.");
#else
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "'~'/Console to toggle input.");
#endif
				
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
						ToolWindow->UpdateTool(DeltaSeconds);
					}
				}
			}
			
			ImGuiContext->EndFrame();
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

#if WITH_EDITOR
/*static*/ void FImGuiToolsManager::LaunchEditorTool(const TArray<FString>& Args)
{
#if DRAW_IMGUI_TOOLS
	FImGuiToolsModule& ImGuiToolsModule = FModuleManager::GetModuleChecked<FImGuiToolsModule>("ImGuiTools");
	if (!ImGuiToolsModule.GetToolsManager().IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.launch_editor_tool' was unable to find the ImGuiTools Module."));
		return;
	}

	if (Args.Num() < 1)
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.launch_editor_tool' must be provided a string parameter tool name to toggle visibility."));
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
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.launch_editor_tool' was unable to find a tool with name: '%s'"), *ToolWindowName);
		return;
	}

	if (!FoundTool->IsEditorToolAllowed())
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.launch_editor_tool' tool '%s' is not set to be an editor tool. To enabled, override ::IsEditorToolsAllowed()."), *ToolWindowName);
		return;	
	}

	const FString EditorToolName = FString::Printf(TEXT("%s ImGuiEditorWidget"), *FoundTool->GetToolName());
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(*EditorToolName));
#endif	  // #if DRAW_IMGUI_TOOLS
}
#endif    // #if WITH_EDITOR

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
			Entry.Color = FColor::Yellow;
			Commands.Add(Entry);

#if WITH_EDITOR
			if (ToolWindow->IsEditorToolAllowed())
			{
				FAutoCompleteCommand EditorEntry;
				EditorEntry.Command = FString::Printf(TEXT("%s %s"), *LaunchEditorToolCVARName, *ToolName);
				EditorEntry.Desc = FString::Printf(TEXT("Launch editor panel tool for ImGui Tool '%s' in namespace '%s'"), *ToolName, *NamespaceName.ToString());
				EditorEntry.Color = FColor::Cyan;
				Commands.Add(EditorEntry);
			}
#endif
		}
	}
#endif // #if DRAW_IMGUI_TOOLS
}

bool FImGuiToolsInputProcessor::HandleKeyDownEvent( FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent )
{
#if DRAW_IMGUI_TOOLS
	KeyCodesDown.AddUnique({ InKeyEvent.GetKeyCode(), InKeyEvent.GetCharacter() });
	
	const bool ToggleVisBefore = ToggleVisDown;
	const bool ToggleInputBefore = ToggleInputDown;

	// Will update ToggleVisDown / ToggleInput Down with new state. 
	CheckForToggleShortcutState();

	if (!ToggleVisBefore && ToggleVisDown)
	{
		ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->Set(!ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->GetBool());
	}
#endif // #if DRAW_IMGUI_TOOLS
	
	return false;
}

bool FImGuiToolsInputProcessor::HandleKeyUpEvent( FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent )
{
#if DRAW_IMGUI_TOOLS
	KeyCodesDown.Remove({ InKeyEvent.GetKeyCode(), InKeyEvent.GetCharacter() });
	
	CheckForToggleShortcutState();
#endif // #if DRAW_IMGUI_TOOLS
	
	return false;
}

void FImGuiToolsInputProcessor::CheckForToggleShortcutState()
{
	// Toggle Input
	int32 InputKeysDown = 0;
	bool NewToggleInputDown = true;
	const TArray<FKey>& ToggleInputKeys = GetDefault<UImGuiToolsDeveloperSettings>()->ImGuiToggleInputKeys;
	for (const FKey& ToggleInputKey : ToggleInputKeys)
	{
		const bool KeyDown = KeyCodesDown.ContainsByPredicate([ToggleInputKey](const KeyCodePair& KeyCode)
		{
			const uint32* KeyCodePtr;
			const uint32* CharCodePtr;
			FInputKeyManager::Get().GetCodesFromKey(ToggleInputKey, KeyCodePtr, CharCodePtr);

			return (KeyCodePtr && (*KeyCodePtr == KeyCode.Get<0>())) || (CharCodePtr && (*CharCodePtr == KeyCode.Get<1>()));
		});

		if (KeyDown)
		{
			InputKeysDown++;
		}
		
		NewToggleInputDown &= KeyDown;
	}

	ToggleInputDown = NewToggleInputDown;

	// Toggle Vis
	bool NewToggleVisDown = true;
	const TArray<FKey>& ToggleVisKeys = GetDefault<UImGuiToolsDeveloperSettings>()->ImGuiToggleVisibilityKeys;
	for (const FKey& ToggleVisKey : ToggleVisKeys)
	{
		const bool KeyDown = KeyCodesDown.ContainsByPredicate([ToggleVisKey](const KeyCodePair& KeyCode)
		{
			const uint32* KeyCodePtr;
			const uint32* CharCodePtr;
			FInputKeyManager::Get().GetCodesFromKey(ToggleVisKey, KeyCodePtr, CharCodePtr);

			return (KeyCodePtr && (*KeyCodePtr == KeyCode.Get<0>())) || (CharCodePtr && (*CharCodePtr == KeyCode.Get<1>()));
		});
		
		NewToggleVisDown &= KeyDown;
	}

	ToggleVisDown = NewToggleVisDown;
}
