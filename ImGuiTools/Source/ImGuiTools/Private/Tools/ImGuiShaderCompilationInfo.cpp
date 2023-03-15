// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiShaderCompilationInfo.h"

#include "Utils/ImGuiUtils.h"

#include <ShaderCompiler.h>

#include <imgui.h>

FImGuiShaderCompilationInfo::FImGuiShaderCompilationInfo()
{
	ToolName = TEXT("Shader Compilation Info");
    WindowFlags = ImGuiWindowFlags_MenuBar;
}

void FImGuiShaderCompilationInfo::ImGuiUpdate(float DeltaTime)
{
    ImGui::Text("Shader Compilation Info");

    if (ImGui::CollapsingHeader("Shader Comp Manager", ImGuiTreeNodeFlags_DefaultOpen))
	{
    	ImGui::Text("  * GetStaticAssetTypeName:			%s", Ansi(*GShaderCompilingManager->GetStaticAssetTypeName().ToString()));
    	ImGui::Text("  * IsCompiling:					%d", GShaderCompilingManager->IsCompiling());
    	ImGui::Text("  * HasShaderJobs:					%d", GShaderCompilingManager->HasShaderJobs());
    	ImGui::Text("  * IsShaderCompilationSkipped:		%d", GShaderCompilingManager->IsShaderCompilationSkipped());
    	ImGui::Text("  * GetNumLocalWorkers:				%d", GShaderCompilingManager->GetNumLocalWorkers());
    	ImGui::Text("  * GetNumOutstandingJobs:			%d", GShaderCompilingManager->GetNumOutstandingJobs());
    	ImGui::Text("  * GetNumPendingJobs:				%d", GShaderCompilingManager->GetNumPendingJobs());
    	ImGui::Text("  * GetNumRemainingJobs:			%d", GShaderCompilingManager->GetNumRemainingJobs());
    	ImGui::Text("  * ShouldDisplayCompilingNotification:		%d", GShaderCompilingManager->ShouldDisplayCompilingNotification());	
	}

	if (ImGui::CollapsingHeader("Shader Comp Stats", ImGuiTreeNodeFlags_DefaultOpen))
	{
		//ImGui::Text("  * GetTimeShaderCompilationWasActive:		% .02f", GShaderCompilerStats->GetTimeShaderCompilationWasActive());
		//ImGui::Text("  * GetTotalShadersCompiled:				%d", GShaderCompilerStats->GetTotalShadersCompiled());
		// ImGui::Text("  * GetDDCHits:					%d", GShaderCompilerStats->GetDDCHits());
		// ImGui::Text("  * GetDDCMisses:				%d", GShaderCompilerStats->GetDDCMisses());

		const TSparseArray<FShaderCompilerStats::ShaderCompilerStats>& ShaderCompStats = GShaderCompilerStats->GetShaderCompilerStats();
		const int NumShaderCompStats = ShaderCompStats.Num();
		int CurShaderCompStat = 0;
		for (const FShaderCompilerStats::ShaderCompilerStats& ShaderCompStat : ShaderCompStats)
		{
			const int NumShaderCompStatMap = ShaderCompStat.Num();
			ImGui::Text("  ShaderCompStat %02d / %02d - map size: %02d", ++CurShaderCompStat, NumShaderCompStats, NumShaderCompStatMap);
			for (const TPair<FString, FShaderCompilerStats::FShaderStats>& ShaderStatEntry : ShaderCompStat)
			{
				ImGui::Text("      key %s - compiled: %d (d: %d) - cooked: %d (d: %d) - compile time: % .04f - perms: %d", Ansi(*ShaderStatEntry.Key), ShaderStatEntry.Value.Compiled, ShaderStatEntry.Value.CompiledDouble, ShaderStatEntry.Value.Cooked, ShaderStatEntry.Value.CookedDouble, ShaderStatEntry.Value.CompileTime, ShaderStatEntry.Value.PermutationCompilations.Num());
				for (const FShaderCompilerStats::FShaderCompilerSinglePermutationStat& PermStat : ShaderStatEntry.Value.PermutationCompilations)
				{
					ImGui::Text("            perm: %s - compiled: %d (d: %d) - cooked: %d (d: %d)", Ansi(*PermStat.PermutationString), PermStat.Compiled, PermStat.CompiledDouble, PermStat.Cooked, PermStat.CookedDouble);
				}
			}
		}
	}
}
