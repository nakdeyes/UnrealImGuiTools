// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiCDOExplorer.h"

#include <imgui.h>

#include "UObject/Class.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Utils/ImGuiUtils.h"
#include "Widgets/ClassSelectorWidget.h"

namespace ImGuiCDOExUtil
{
    enum EDrawClassType
    {
        Native,
        BP,
        NativeAndBP,
    };

    namespace EDrawColumnType
    {
        enum Type
        {
            NativeClass = 0,
            ActorTicks,
            ActorReplicates,
            ActorComponentsCount,
            SceneComponentsCount,
            PrimitiveComponentCount,
            GenerateOverlapsCompsCount,
            TickingCompsCount,
        
            COUNT
        };

		static bool DefaultVisibility[Type::COUNT] =
		{
			/*NativeClass */		        true,
            /*ActorTicks*/                  true,
            /*ActorReplicates*/             true,
            /*ActorComponentsCount*/        true,
            /*SceneComponentsCount*/        true,
            /*PrimitiveComponentCount*/     true,
            /*GenerateOverlapsCompsCount*/   true,
            /*TickingCompsCount*/           true,
		};
    };

    struct FColumnSettings
    {
        EDrawColumnType::Type SortColumnType = EDrawColumnType::SceneComponentsCount;
        ImGuiTools::Utils::FShowCols ShowCols = ImGuiTools::Utils::FShowCols(EDrawColumnType::COUNT, &EDrawColumnType::DefaultVisibility[0]);
    };

    static EDrawClassType sDrawClassType = EDrawClassType::BP;
    static FColumnSettings sColumnSettings;

    // Forward declaration
    static bool ShouldDrawCDO(ImGuiTools::FHierarchicalClassInfo& ClassInfo);
    static bool ShouldDrawAnyCDOChildren(ImGuiTools::FHierarchicalClassInfo& ClassInfo)
    {
        // Only returns true if one or more of our children should be drawn (ignoring if the provided class info should or not)
		for (ImGuiTools::FHierarchicalClassInfo& LoadedChildInfo : ClassInfo.mLoadedChildren)
		{
			if (ShouldDrawCDO(LoadedChildInfo))
			{
				return true;
			}
		}

        return false;
    }

    static bool ShouldDrawCDO(ImGuiTools::FHierarchicalClassInfo& ClassInfo)
    {
        // First, see if we are just drawing all
        if (ImGuiCDOExUtil::sDrawClassType == EDrawClassType::NativeAndBP)
        {
            return true;
        }
        
        // Next, check to see if this class matches the Native/BP type
		UClass* LoadedClass = ClassInfo.mClass.Get();
		const bool IsNative = LoadedClass && LoadedClass->IsNative();
        if ((IsNative && (ImGuiCDOExUtil::sDrawClassType == EDrawClassType::Native)) ||
            (!IsNative && (ImGuiCDOExUtil::sDrawClassType == EDrawClassType::BP)))
        {
            return true;
        }

        // Finally, we also need to draw ourselves if any of our children are of that type.
        return ShouldDrawAnyCDOChildren(ClassInfo);
    }

    static void DrawCDO(ImGuiTools::FHierarchicalClassInfo& ClassInfo)
    {
        // See if we should draw ourselves first
        if (!ShouldDrawCDO(ClassInfo))
        {
            return;
        }

		bool TreeOpen = false;
        FString ClassLabel = ClassInfo.mClass->GetFName().ToString();
        //const bool DrawAsTree = (ClassInfo.mLoadedChildren.Num() == 0) && (ClassInfo.mUnloadedChildren.Num() == 0);
        const bool DrawAsTree = ShouldDrawAnyCDOChildren(ClassInfo);
		if (DrawAsTree)
		{
			TreeOpen = ImGui::TreeNode(Ansi(*ClassLabel)); ImGui::NextColumn();
		}
		else
		{
			ImGui::Text("   %s", Ansi(*ClassLabel)); ImGui::NextColumn();
		}
        if (ClassInfo.mCachedAllUnloadedDecendants > 0)
        {
            ImGui::TextColored(ImGuiTools::Colors::Red, "!! %d Unloaded descendants found when they really shouldn't be", ClassInfo.mCachedAllUnloadedDecendants);
        }

        if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::NativeClass)) { ImGui::Text(ClassInfo.mClass->IsNative() ? "Native" : "BP"); ImGui::NextColumn(); }

        if (AActor* CDOAct = Cast<AActor>(ClassInfo.mClass->GetDefaultObject()))
        {
            TArray<UActorComponent*> ActComps; CDOAct->GetComponents(UActorComponent::StaticClass(), ActComps);
            TArray<UActorComponent*> SceneComps; CDOAct->GetComponents(USceneComponent::StaticClass(), SceneComps);
            TArray<UActorComponent*> PrimitiveComps; CDOAct->GetComponents(UPrimitiveComponent::StaticClass(), PrimitiveComps);
		    if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::ActorTicks)) { ImGui::Text(CDOAct->IsActorTickEnabled() ? "ticking" : "not ticking"); ImGui::NextColumn(); }
		    if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::ActorReplicates)) { ImGui::Text(CDOAct->GetIsReplicated() ? "replicated" : "not replicated"); ImGui::NextColumn(); }
		    if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::ActorComponentsCount)) { ImGui::Text("%02d", ActComps.Num()); ImGui::NextColumn(); }
		    if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::SceneComponentsCount)) { ImGui::Text("%02d", SceneComps.Num()); ImGui::NextColumn(); }
            if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::PrimitiveComponentCount)) { ImGui::Text("%02d", PrimitiveComps.Num()); ImGui::NextColumn(); }
		    if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::GenerateOverlapsCompsCount)) { 
                int PrimsGeneratingOverlaps = 0;
                for (UActorComponent* ActComp : PrimitiveComps)
                {
                    UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActComp);
					if (PrimComp && PrimComp->GetGenerateOverlapEvents())
						++PrimsGeneratingOverlaps;
                }
                ImGui::Text("%02d", PrimsGeneratingOverlaps); ImGui::NextColumn(); 
            }
		    if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::TickingCompsCount)) { 
                int TickingComps = 0;
                for (UActorComponent* ActComp : ActComps)
                {
                    if (ActComp->IsComponentTickEnabled())
                        ++TickingComps;
                }
                ImGui::Text("%02d", TickingComps); ImGui::NextColumn(); 
            }
        }
        else
        {
			if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::ActorTicks)) { ImGui::Text("*not_actor*"); ImGui::NextColumn(); }
			if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::ActorReplicates)) { ImGui::Text("*not_actor*"); ImGui::NextColumn(); }
			if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::ActorComponentsCount)) { ImGui::Text("*not_actor*"); ImGui::NextColumn(); }
			if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::SceneComponentsCount)) { ImGui::Text("*not_actor*"); ImGui::NextColumn(); }
            if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::PrimitiveComponentCount)) { ImGui::Text("*not_actor*"); ImGui::NextColumn(); }
			if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::GenerateOverlapsCompsCount)) { ImGui::Text("*not_actor*"); ImGui::NextColumn(); }
			if (sColumnSettings.ShowCols.GetShowCol(EDrawColumnType::TickingCompsCount)) { ImGui::Text("*not_actor*"); ImGui::NextColumn(); }
        }

        if (TreeOpen)
        {
			for (ImGuiTools::FHierarchicalClassInfo& LoadedChildInfo : ClassInfo.mLoadedChildren)
			{
                DrawCDO(LoadedChildInfo);
			}
            ImGui::TreePop();
        }
    }
}   //  namespace FImGuiCDOExUtil

FImGuiCDOExplorer::FImGuiCDOExplorer()
{
	ToolName = "CDO Explorer";
}

void FImGuiCDOExplorer::ImGuiUpdate(float DeltaTime)
{
    static ImGuiTools::FClassSelector CDOExplorerClassSelector(UObject::StaticClass());
    if (ImGui::CollapsingHeader("Parent Class to Explore", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const float ClassExplorerHeight = FMath::Max(250.0f, ImGui::GetWindowHeight() * 0.5f);
        ImGui::BeginChild("CDOExplorerClassSelection", ImVec2(0, ClassExplorerHeight), true);
        CDOExplorerClassSelector.Draw("CDOExplorerClassSel", ImVec2(0, ClassExplorerHeight - 48.0f));
    
        UClass* SelectedClass = CDOExplorerClassSelector.GetSelectedClass();
        if (SelectedClass)
        {
            if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Load and View X BP CDOs from class: %s"), *SelectedClass->GetName()))))
            {
                mCDOExplorerClasses.ResetToRootClass(SelectedClass);
                mCDOExplorerClasses.mRootClassInfo.LoadChildren();
            }
        }
        else
        {
            ImGui::TextColored(ImGuiTools::Colors::Orange, "Select a Class type above to explore.");
        }
        ImGui::EndChild();  // "CDOExplorerClassSelection"
    }

    if (ImGui::CollapsingHeader("CDO Explorer Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static float TempHeight = 124.0f;
        ImGui::BeginChild("CDOExplorerSettings", ImVec2(0, TempHeight), true);
        ImGui::Columns(2);

        if (ImGui::RadioButton("##Native", ImGuiCDOExUtil::sDrawClassType == ImGuiCDOExUtil::EDrawClassType::Native)) { ImGuiCDOExUtil::sDrawClassType = ImGuiCDOExUtil::EDrawClassType::Native; } ImGui::SameLine();
        ImGui::Text("Native"); ImGui::SameLine();
		if (ImGui::RadioButton("##BP", ImGuiCDOExUtil::sDrawClassType == ImGuiCDOExUtil::EDrawClassType::BP)) { ImGuiCDOExUtil::sDrawClassType = ImGuiCDOExUtil::EDrawClassType::BP; } ImGui::SameLine();
		ImGui::Text("BP"); ImGui::SameLine();
		if (ImGui::RadioButton("##Native/BP", ImGuiCDOExUtil::sDrawClassType == ImGuiCDOExUtil::EDrawClassType::NativeAndBP)) { ImGuiCDOExUtil::sDrawClassType = ImGuiCDOExUtil::EDrawClassType::NativeAndBP; } ImGui::SameLine();
		ImGui::Text("Native/BP");

        ImGui::NextColumn();

        static const float RadWidth = 54.0f;
        ImGui::Text("Include/Sort:");
		ImGui::BeginChild("IncSortCDOExp", ImVec2(390.0f, 0.0f));
		ImGui::Columns(2);
		ImGui::Checkbox("##NativeCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::NativeClass)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::NativeClass))
		{ if (ImGui::RadioButton("##NativeRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::NativeClass)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::NativeClass; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Native"); ImGui::NextColumn();

		ImGui::Checkbox("##ActorTicksCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorTicks)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorTicks))
		{ if (ImGui::RadioButton("##ActorTicksRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::ActorTicks)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::ActorTicks; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Actor Ticks"); ImGui::NextColumn();

		ImGui::Checkbox("##ActorReplicatesCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorReplicates)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorReplicates))
		{ if (ImGui::RadioButton("##ActorReplicatesRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::ActorReplicates)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::ActorReplicates; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Replicates"); ImGui::NextColumn();

		ImGui::Checkbox("##ActorCompsCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorComponentsCount)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorComponentsCount))
		{ if (ImGui::RadioButton("##ActorCompsCRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::ActorComponentsCount)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::ActorReplicates; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Actor Comps"); ImGui::NextColumn();

		ImGui::Checkbox("##SceneCompsCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::SceneComponentsCount)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::SceneComponentsCount))
		{ if (ImGui::RadioButton("##SceneCompsRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::SceneComponentsCount)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::ActorReplicates; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Scene Comps"); ImGui::NextColumn();

		ImGui::Checkbox("##PrimCompsCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::PrimitiveComponentCount)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::PrimitiveComponentCount))
		{ if (ImGui::RadioButton("##PrimCompsRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::PrimitiveComponentCount)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::ActorReplicates; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Primitive Comps"); ImGui::NextColumn();

		ImGui::Checkbox("##OverlapCompsCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::GenerateOverlapsCompsCount)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::GenerateOverlapsCompsCount))
		{ if (ImGui::RadioButton("##OverlapCompsRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::GenerateOverlapsCompsCount)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::ActorReplicates; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Overlap Comps"); ImGui::NextColumn();

		ImGui::Checkbox("##TickingCompsCh", &ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::TickingCompsCount)); ImGui::SameLine();
		if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::TickingCompsCount))
		{ if (ImGui::RadioButton("##TickingCompsRB", ImGuiCDOExUtil::sColumnSettings.SortColumnType == ImGuiCDOExUtil::EDrawColumnType::TickingCompsCount)) { ImGuiCDOExUtil::sColumnSettings.SortColumnType = ImGuiCDOExUtil::EDrawColumnType::ActorReplicates; } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Ticking Comps"); ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::EndChild();  // "IncSortCDOExp"
        ImGui::Columns(1);
        ImGui::EndChild();  // "CDOExplorerSettings"
    }
	
    const int VisibleColCount = ImGuiCDOExUtil::sColumnSettings.ShowCols.CacheShowColCount() + 1;

    ImGui::BeginChild("CDOExplorerHeader", ImVec2(0, 36), true);
    ImGui::Columns(VisibleColCount + 1);
	ImGui::Text("Class"); ImGui::NextColumn();
	if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::NativeClass)) {                ImGui::Text("Native Class"); ImGui::NextColumn(); }
    if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorTicks)) {                 ImGui::Text("Ticks"); ImGui::NextColumn(); }
    if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorReplicates)) {            ImGui::Text("Replicates"); ImGui::NextColumn(); }
    if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::ActorComponentsCount)) {       ImGui::Text("Actor Comps"); ImGui::NextColumn(); }
    if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::SceneComponentsCount)) {       ImGui::Text("Scene Comps"); ImGui::NextColumn(); }
    if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::PrimitiveComponentCount)) { ImGui::Text("Primitive Comps"); ImGui::NextColumn(); }
    if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::GenerateOverlapsCompsCount)) {  ImGui::Text("Generate Overlaps"); ImGui::NextColumn(); }
    if (ImGuiCDOExUtil::sColumnSettings.ShowCols.GetShowCol(ImGuiCDOExUtil::EDrawColumnType::TickingCompsCount)) {          ImGui::Text("Ticking Comps"); ImGui::NextColumn(); }

    ImGui::Columns(1);
    ImGui::EndChild();  // "CDOExplorerHeader"
    
    ImGui::BeginChild("CDOExplorerBody", ImVec2(0, 0), true);


    if (mCDOExplorerClasses.mRootClassInfo.mClass.IsValid())
    {
    	ImGui::Columns(VisibleColCount, "CDODataCols");
        
        ImGuiCDOExUtil::DrawCDO(mCDOExplorerClasses.mRootClassInfo);
        
        ImGui::Columns(1);
    }
    else
    {
        ImGui::TextColored(ImGuiTools::Colors::Orange, "Select a parent class above to view CDOs");
    }

    ImGui::EndChild();  // "CDOExplorerBody"
}