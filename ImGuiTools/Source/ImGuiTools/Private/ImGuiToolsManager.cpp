// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsManager.h"

#include "Tools/ImGuiActorComponentDebugger.h"
#include "Tools/ImGuiFileLoadDebugger.h"
#include "Tools/ImGuiMemoryDebugger.h"

#include <Engine/Console.h>

#if !UE_BUILD_SHIPPING
#include "ImGuiTools.h"
#include "ImGuiUtils.h"
#include <imgui.h>
#endif	  // #if !UE_BUILD_SHIPPING

// CVARs
TAutoConsoleVariable<bool> ImGuiDebugCVars::CVarImGuiToolsEnabled(TEXT("imgui.tools.enabled"), false, TEXT("If true, draw ImGui Debug tools."));
static FString ToggleToolVisCVARName = TEXT("imgui.tools.toggle_tool_vis");
FAutoConsoleCommand ToggleToolVis(*ToggleToolVisCVARName, TEXT("Toggle the visibility of a particular ImGui Debug Tool by providing it's string name as an argument"),
								  FConsoleCommandWithArgsDelegate::CreateStatic(&FImGuiToolsManager::ToggleToolVisCommand));

FImGuiToolsManager::FImGuiToolsManager()
	: DrawImGuiDemo(false)
{
	UConsole::RegisterConsoleAutoCompleteEntries.AddRaw(this, &FImGuiToolsManager::RegisterAutoCompleteEntries);
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
#if !UE_BUILD_SHIPPING
	return ETickableTickType::Conditional;
#else
	return ETickableTickType::Never;
#endif	  // #if !UE_BUILD_SHIPPING
}

bool FImGuiToolsManager::IsTickable() const
{
	return ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnGameThread();
}

void FImGuiToolsManager::InitPluginTools()
{
	static const FName PluginToolsNamespace = TEXT("Included Plugin Tools");
	RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiActorComponentDebugger()), PluginToolsNamespace);
	RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiFileLoadDebugger()), PluginToolsNamespace);
	RegisterToolWindow(TSharedPtr<FImGuiToolWindow>(new FImGuiMemoryDebugger()), PluginToolsNamespace);
}

void FImGuiToolsManager::RegisterToolWindow(TSharedPtr<FImGuiToolWindow> ToolWindow, FName ToolNamespace /*= NAME_None*/)
{
	TArray<TSharedPtr<FImGuiToolWindow>>& NamespaceTools = ToolWindows.FindOrAdd(ToolNamespace);
	NamespaceTools.Add(ToolWindow);
}

void FImGuiToolsManager::Tick(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
	// Draw Main Menu Bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("UETools"))
		{
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
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Misc");
			ImGui::Checkbox("Draw ImGui demo", &DrawImGuiDemo);
			ImGui::EndMenu();
		}
		const float Width = ImGui::GetWindowWidth();
		ImGui::SameLine(500.0f);
		ImGui::Text("'imgui.toggleinput' to toggle input, or set HotKey in ImGui Plugin prefs.");
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
#endif	  // #if !UE_BUILD_SHIPPING
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
#if !UE_BUILD_SHIPPING
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

	const bool AllToolsHidden = !ImGuiDebugCVars::CVarImGuiToolsEnabled.GetValueOnAnyThread();
	if (AllToolsHidden)
	{
		// Use case is: I just want to see my tool, or I just want to hide 1 tool. So always set the Tools to Enabled if they are globally disabled...
		ImGuiDebugCVars::CVarImGuiToolsEnabled.AsVariable()->Set(true);
		FoundTool->EnableTool(true);
	}
	else
	{
		// .. else just toggle tool vis.
		FoundTool->EnableTool(!FoundTool->GetEnabledRef());
	}
#endif	  // #if !UE_BUILD_SHIPPING
}

void FImGuiToolsManager::RegisterAutoCompleteEntries(TArray<FAutoCompleteCommand>& Commands) const
{
#if !UE_BUILD_SHIPPING
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
#endif // #if !UE_BUILD_SHIPPING
}
