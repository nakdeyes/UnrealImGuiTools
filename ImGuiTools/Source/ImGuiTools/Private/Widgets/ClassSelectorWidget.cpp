// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "Widgets/ClassSelectorWidget.h"

#include "Runtime/Launch/Resources/Version.h"

#include "Utils/ImGuiUtils.h"

namespace ClassSelectorUtil
{
    static bool ClassPassesNameFilter(ImGuiTools::FHierarchicalClassInfo& ClassInfo, ImGuiTextFilter& ClassNameFilter)
    {
        // We pass if our class name passes filter...
        if (ClassNameFilter.PassFilter(Ansi(*ClassInfo.mClass->GetFName().ToString())))
        {
            return true;
        }
        
        // .. or if any unloaded classes match filter
        for (ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo& UnloadedClassInfo : ClassInfo.mUnloadedChildren)
        {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
            if (ClassNameFilter.PassFilter(Ansi(*UnloadedClassInfo.ClassAssetPath.ToString())))
#else   // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
            if (ClassNameFilter.PassFilter(Ansi(*UnloadedClassInfo.ClassName)))
#endif  // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
            {
                return true;
            }
        }

        // .. or finally, any loaded children and their potential children pass filter
        for (ImGuiTools::FHierarchicalClassInfo& LoadedClassInfo : ClassInfo.mLoadedChildren)
        {
            if (ClassPassesNameFilter(LoadedClassInfo, ClassNameFilter))
            {
                return true;
            }
        }

        return false;
    }

    static void DrawClass(ImGuiTools::FHierarchicalClassInfo& ClassInfo, TWeakObjectPtr<UClass>& OUT_SelectedUClass, ImGuiTools::FClassSelector& ClassSelector)
    {
        
        ImGuiTextFilter& ClassTextFilter = ClassSelector.GetClassNameFilter();
        const bool InFilter = ClassPassesNameFilter(ClassInfo, ClassTextFilter);
        if (!InFilter)
        {
            return;
        }

        bool TreeOpen = false;
        if ((ClassInfo.mLoadedChildren.Num() == 0) && (ClassInfo.mUnloadedChildren.Num() == 0))
        {
            ImGui::Text("   %s", Ansi(*ClassInfo.mClass->GetFName().ToString()));
        }
        else
        {
            ImGuiTreeNodeFlags NodeFlags = ClassTextFilter.IsActive() ? ImGuiTreeNodeFlags_DefaultOpen : 0;
            TreeOpen = ImGui::TreeNodeEx(Ansi(*ClassInfo.mClass->GetFName().ToString()), NodeFlags);
        }
        if (ClassInfo.mCachedAllUnloadedDecendants > 0)
        {
            ImGui::SameLine(ImGui::GetWindowWidth() - 180.0f);
            if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Load %d BPs##%s"), ClassInfo.mCachedAllUnloadedDecendants, *ClassInfo.mClass->GetFName().ToString()))))
            {
                ClassInfo.LoadChildren();
                ClassSelector.QueueReset();
            }
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
        if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Select##%s"), *ClassInfo.mClass->GetFName().ToString()))))
        {
            OUT_SelectedUClass = ClassInfo.mClass;
        }

        if (TreeOpen)
        {
            for (ImGuiTools::FHierarchicalClassInfo& LoadedChildInfo : ClassInfo.mLoadedChildren)
            {
                DrawClass(LoadedChildInfo, OUT_SelectedUClass, ClassSelector);
            }

            for (ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo& UnloadedChildInfo : ClassInfo.mUnloadedChildren)
            {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
                if (ClassSelector.GetClassNameFilter().PassFilter(Ansi(*UnloadedChildInfo.ClassAssetPath.ToString())))
                {
                    ImGui::TextColored(ImGuiTools::Colors::Purple, " Unloaded BP: %s (inf: %s)", Ansi(*UnloadedChildInfo.ClassAssetPath.ToString()), Ansi(*UnloadedChildInfo.SoftClassInfo.ToString()));
					ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);
					if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Load##%s"), *UnloadedChildInfo.ClassAssetPath.ToString()))))
					{
                        UnloadedChildInfo.SoftClassInfo.LoadSynchronous();
                        ClassSelector.QueueReset();
					}
                }
#else   // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
                if (ClassSelector.GetClassNameFilter().PassFilter(Ansi(*UnloadedChildInfo.ClassName)))
                {
                    ImGui::TextColored(ImGuiTools::Colors::Purple, " Unloaded BP: %s (inf: %s)", Ansi(*UnloadedChildInfo.ClassName), Ansi(*UnloadedChildInfo.SoftClassInfo.ToString()));
                    ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);
                    if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Load##%s"), *UnloadedChildInfo.ClassName))))
                    {
                        UnloadedChildInfo.SoftClassInfo.LoadSynchronous();
                        ClassSelector.QueueReset();
                    }
                }
#endif  // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
            }

            ImGui::TreePop();
        }
    }
}

ImGuiTools::FClassSelector::FClassSelector(UClass* RootClass, FString SearchDirectory /*= FString(TEXT("/Game/"))*/, bool HierarchicalView /*= true*/)
{
    mHierarchicalView = HierarchicalView;

    mRootClassInfo.ResetToRootClass(RootClass, SearchDirectory);
}

void ImGuiTools::FClassSelector::QueueReset()
{
    mResetQueued = true;
}

void ImGuiTools::FClassSelector::Draw(const char* ID, ImVec2 Size)
{
    // Execute any queued resets from last frame
    if (mResetQueued)
    {
        mResetQueued = false;
        mRootClassInfo.Reset();
    }

    ImGui::BeginChild(ID, Size);

    ImGui::BeginChild(Ansi(*FString::Printf(TEXT("%s_Options"), ID)), ImVec2(0.0f, 36.0f), true);
    ImGui::SameLine();
    if (ImGui::SmallButton("Refresh Hierarchy"))
    {
        mRootClassInfo.Reset();
    }
    ImGui::SameLine();
    ClassNameFilter.Draw();
    ImGui::EndChild();  // Options

    ImGui::BeginChild(Ansi(*FString::Printf(TEXT("%s_Body"), ID)), ImVec2(0.0f, 0.0f), true);
    ClassSelectorUtil::DrawClass(mRootClassInfo.mRootClassInfo, mSelectedClass, *this);
    ImGui::EndChild();  // Body

    ImGui::EndChild();  // ID
}

UClass* ImGuiTools::FClassSelector::GetSelectedClass()
{
    return mSelectedClass.Get();
}

ImGuiTextFilter& ImGuiTools::FClassSelector::GetClassNameFilter()
{
    return ClassNameFilter;
}
