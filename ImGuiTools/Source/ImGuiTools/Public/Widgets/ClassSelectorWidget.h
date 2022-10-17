// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <imgui.h>

#include "Utils/ClassHierarchyInfo.h"

// forward declarations
class UClass;

namespace ImGuiTools
{
    struct FClassSelector
    {
        FClassSelector(UClass* RootClass, FString SearchDirectory = FString(TEXT("/Game/")), bool HierarchicalView = true);
        void Draw(const char* ID, ImVec2 Size);
        UClass* GetSelectedClass();
        ImGuiTextFilter& GetClassNameFilter();

        // Queue a reset for the next draw
        void QueueReset();

    private:
        FHierarchicalRootClassInfo mRootClassInfo;
        ImGuiTextFilter ClassNameFilter;
        TWeakObjectPtr<UClass> mSelectedClass;
        bool mHierarchicalView = true;
        bool mResetQueued = false;
    };
}
