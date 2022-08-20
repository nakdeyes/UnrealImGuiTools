// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <imgui.h>

// forward declarations
class UClass;

namespace ImGuiTools
{
    struct HierarchicalClassInfo
    {
        TWeakObjectPtr<UClass> mClass;
        TArray<HierarchicalClassInfo> mChildren;
    };

    class UClassSelector
    {
    public:
        UClassSelector(UClass* ParentClass, FString SearchDirectory = FString(TEXT("/Game/")), bool HierarchicalView = true);
        void Draw(const char* ID, ImVec2 Size);
        UClass* GetSelectedClass();

    private:
        void CacheUClasses();

    private:
        HierarchicalClassInfo mClassInfo;
        TWeakObjectPtr<UClass> mSelectedClass;
        TWeakObjectPtr<UClass> mParentClass;
        FString mSearchDirectory;
        bool mHierarchicalView;
    };
}
