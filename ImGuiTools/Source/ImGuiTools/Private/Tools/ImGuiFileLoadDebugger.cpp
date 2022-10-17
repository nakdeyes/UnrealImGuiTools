// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiFileLoadDebugger.h"

#include <imgui.h>
#include <ImGuiTools.h>
#include <ImGuiToolsManager.h>
#include <Utils/ImGuiUtils.h>

// Log Category
DEFINE_LOG_CATEGORY_STATIC(LogImGuiDebugLoad, Warning, All);

// CVARs
FAutoConsoleCommand ToggleFileLoadRecordCMD(TEXT("imgui.tools.file_load.toggle_record"),
											TEXT("Toggle the recording state of the ImGui File Load Debugger"),
											FConsoleCommandWithArgsDelegate::CreateStatic(&FImGuiFileLoadDebugger::ToggleRecordCommand));

namespace ImGuiFileLoadDebuggerConsts
{
	static const FString ToolName = "LoadDebugger";
}	// namespace ImGuiFileLoadDebuggerConsts

namespace ImGuiFileLoadUtils
{
	void DrawLoadList(ImVec4 ColorVec, const TArray<FString>& LoadList)
	{
		for (int i = LoadList.Num() - 1; i >= 0; --i)
		{
			ImGui::TextColored(ColorVec, "%05d - %s", i + 1, Ansi(*LoadList[i]));

		}
	}
}	// namespace ImGuiFileLoadUtils

void FImGuiFileLoadDebugger::ToggleRecordCommand(const TArray<FString>& Args)
{
	FImGuiToolsModule& ImGuiToolsModule = FModuleManager::GetModuleChecked<FImGuiToolsModule>("ImGuiTools");
	if (!ImGuiToolsModule.GetToolsManager().IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.file_load.toggle_record' was unable to find the ImGuiTools Module."));
		return;
	}

	TSharedPtr<FImGuiFileLoadDebugger> FileLoadDebugger = StaticCastSharedPtr<FImGuiFileLoadDebugger>(ImGuiToolsModule.GetToolsManager()->GetToolWindow(ImGuiFileLoadDebuggerConsts::ToolName));
	if (!FileLoadDebugger.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("CVAR'imgui.tools.file_load.toggle_record' was unable to find a tool with name: '%s'"), *ImGuiFileLoadDebuggerConsts::ToolName);
		return;
	}

	FileLoadDebugger->ToggleRecord();
}

FImGuiFileLoadDebugger::FImGuiFileLoadDebugger()
{
	ToolName = ImGuiFileLoadDebuggerConsts::ToolName;
}

void FImGuiFileLoadDebugger::ImGuiUpdate(float DeltaTime)
{
	bool bRecordingLoadsPreCheckbox = bRecordingLoads;
	ImGui::Checkbox("Recording File Loads", &bRecordingLoadsPreCheckbox);
	if (bRecordingLoadsPreCheckbox != bRecordingLoads)
	{
		// Checkbox toggled
		ToggleRecord();
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear All"))
	{
		SyncLoadFiles.Empty();
		AsyncLoadFiles.Empty();
	}
	ImGui::Checkbox("Spew File Loads to Log", &bSpewToLog);

	if (ImGui::CollapsingHeader("Sync Loads", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("SyncLoads", ImVec2(0, 250.0f), true);
		ImGuiFileLoadUtils::DrawLoadList(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), SyncLoadFiles);
		ImGui::EndChild(); //	"SyncLoads"
	}

	if (ImGui::CollapsingHeader("Async Loads", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("AsyncLoads", ImVec2(0, 250.0f), true);
		ImGuiFileLoadUtils::DrawLoadList(ImVec4(0.0f, 0.4f, 1.0f, 1.0f), AsyncLoadFiles);
		ImGui::EndChild(); //	"AsyncLoads"
	}
}

void FImGuiFileLoadDebugger::ToggleRecord()
{
	bRecordingLoads = !bRecordingLoads;

	if (bRecordingLoads)
	{
		AsyncLoadDelegateHandle = FCoreDelegates::OnAsyncLoadPackage.AddRaw(this, &FImGuiFileLoadDebugger::OnAsyncLoadPackage);
		SyncLoadDelegateHandle = FCoreDelegates::OnSyncLoadPackage.AddRaw(this, &FImGuiFileLoadDebugger::OnSyncLoadPackage);
	}
	else
	{
		FCoreDelegates::OnAsyncLoadPackage.Remove(AsyncLoadDelegateHandle);
		AsyncLoadDelegateHandle.Reset();


		FCoreDelegates::OnSyncLoadPackage.Remove(SyncLoadDelegateHandle);
		SyncLoadDelegateHandle.Reset();
	}
	UE_LOG(LogImGuiDebugLoad, Warning, TEXT("FImGuiFileLoadDebugger::ToggleRecord. New Recording state: %s"), bRecordingLoads ? TEXT("enabled") : TEXT("disabled"));
}

void FImGuiFileLoadDebugger::OnAsyncLoadPackage(const FString& PackageName)
{
	// NOTE: sync loads will kick off an async request with this callback, then flush it within the callstack. So true Async loads are loads that
	//	come through this path, but didn't already come through via the SyncLoadPackage callback.
	if (!SyncLoadFiles.Contains(PackageName))
	{
		if (bSpewToLog)
		{
			UE_LOG(LogImGuiDebugLoad, Log, TEXT("FImGuiFileLoadDebugger::OnAsyncLoadPackage - Async Loading Package: '%s'"), *PackageName);
		}

		AsyncLoadFiles.Add(PackageName);
	}
}

void FImGuiFileLoadDebugger::OnSyncLoadPackage(const FString& PackageName)
{
	if (bSpewToLog)
	{
		UE_LOG(LogImGuiDebugLoad, Error, TEXT("FImGuiFileLoadDebugger::OnSyncLoadPackage - Sync Loading Package: '%s'"), *PackageName);
	}

	SyncLoadFiles.Add(PackageName);
}
