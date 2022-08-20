// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "Widgets/ClassSelectorWidget.h"

#include "ImGuiUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"

#pragma optimize ("", off)

namespace ClassSelectorUtil
{
    static bool LoadClasses = false;

    void SortCachedClassInfo(ImGuiTools::HierarchicalClassInfo& ClassInfo, UClass* Class, TArray<UClass*>& ChildClasses)
    {
        ClassInfo.mClass = Class;

        // Iterate backward through potential child classes, finding direct descendants
        for (int i = ChildClasses.Num() - 1; i >= 0; --i)
        {
            if (UClass* PotentialChildClass = ChildClasses[i])
            {
                if (PotentialChildClass->GetSuperClass() == Class)
                {
                    ImGuiTools::HierarchicalClassInfo& ChildClassInfo = ClassInfo.mChildren.AddDefaulted_GetRef();
                    SortCachedClassInfo(ChildClassInfo, PotentialChildClass, ChildClasses);
                }
            }
        }

        // Sort child classes alphabetically
        ClassInfo.mChildren.Sort([](const ImGuiTools::HierarchicalClassInfo& A, const ImGuiTools::HierarchicalClassInfo& B) {
            return A.mClass->GetFName().Compare(B.mClass->GetFName()) < 0;
        });
    }

    void DrawClass(ImGuiTools::HierarchicalClassInfo& ClassInfo, TWeakObjectPtr<UClass>& OUT_SelectedUClass)
    {
        if (ImGui::TreeNode(Ansi(*ClassInfo.mClass->GetFName().ToString())))
        {
            ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
            if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Select##%s"), *ClassInfo.mClass->GetFName().ToString()))))
            {
                OUT_SelectedUClass = ClassInfo.mClass;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Load %d BP Children##%s"), *ClassInfo.mClass->GetFName().ToString()))))
            {
                OUT_SelectedUClass = ClassInfo.mClass;
            }

            for (ImGuiTools::HierarchicalClassInfo& ChildInfo : ClassInfo.mChildren)
            {
                DrawClass(ChildInfo, OUT_SelectedUClass);
            }

            ImGui::TreePop();
        }
    }
}

ImGuiTools::UClassSelector::UClassSelector(UClass* ParentClass, FString SearchDirectory /*= FString(TEXT("/Game/"))*/, bool HierarchicalView /*= true*/)
{
    mParentClass = ParentClass;
    mSearchDirectory = SearchDirectory;
    mHierarchicalView = HierarchicalView;
    CacheUClasses();
}

void ImGuiTools::UClassSelector::Draw(const char* ID, ImVec2 Size)
{
    ImGui::BeginChild(ID, Size, true);

    ImGui::BeginChild(Ansi(*FString::Printf(TEXT("%s_Options"), ID)), ImVec2(0.0f, 36.0f));
    ImGui::Checkbox("Load BP Classes", &ClassSelectorUtil::LoadClasses);
    ImGui::EndChild();  // Options

    ImGui::BeginChild(Ansi(*FString::Printf(TEXT("%s_Body"), ID)), ImVec2(0.0f, 0.0f));
    ClassSelectorUtil::DrawClass(mClassInfo, mSelectedClass);
    ImGui::EndChild();  // Body

    ImGui::EndChild();  // ID
}

UClass* ImGuiTools::UClassSelector::GetSelectedClass()
{
    return mSelectedClass.Get();
}

void ImGuiTools::UClassSelector::CacheUClasses()
{
    // Load the asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    // The asset registry is populated asynchronously at startup, so there's no guarantee it has finished.
    // This simple approach just runs a synchronous scan on the entire content directory.
    TArray<FString> PathsToScan;
    PathsToScan.Add(mSearchDirectory);
    AssetRegistry.ScanPathsSynchronous(PathsToScan);

    TArray<UClass*> ChildClasses;

    // Get all native classes
    for (TObjectIterator< UClass > ClassIt; ClassIt; ++ClassIt)
    {
        UClass* Class = *ClassIt;

        // Only interested in native C++ classes
        if (!Class->IsNative())
        {
            continue;
        }

        // Ignore deprecated
        if (Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }

        // Check this class is a subclass of Base
        if (!Class->IsChildOf(mParentClass.Get()))
        {
            continue;
        }

        // Add this class
        ChildClasses.Add(Class);
    }

    // Get all BP Classes
    TSet<FName> DerivedNames;
    {
        TArray<FName> BaseNames;
        BaseNames.Add(mParentClass->GetFName());

        TSet<FName> Excluded;
        AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
    }

    // Get all assets in the path, does not load them
    TArray<FAssetData> ScriptAssetList;
    AssetRegistry.GetAssetsByPath(*mSearchDirectory, ScriptAssetList, /*bRecursive=*/true);

    // Iterate through all assets and find the appropriate ones.
    for (const FAssetData& Asset : ScriptAssetList)
    {
        if (UClass* Class = Asset.GetClass())
        {
            if (Class == UBlueprint::StaticClass())
            {
                FString GeneratedClassPath;
                if (Asset.GetTagValue(TEXT("GeneratedClass"), GeneratedClassPath))
                {
                    // Get class name and object path.
                    const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPath);
                    const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

                    if (DerivedNames.Contains(*ClassName))
                    {
                        // BP Class matches class name. Get the CDO of the generated BP class if possible.
                        TSoftClassPtr<UObject> AssetSubClass = TSoftClassPtr<UObject>(FStringAssetReference(ClassObjectPath));
                        
                        if (ClassSelectorUtil::LoadClasses)
                        {
                            AssetSubClass.LoadSynchronous();
                        }

                        if (UClass* SubClassUClass = Cast<UClass>(AssetSubClass.TryGetDefaultObject()))
                        {
                            // Store using the path to the generated class
                            ChildClasses.Add(AssetSubClass.Get());
                        }
                    }
                }
            }
        }
    }

    ClassSelectorUtil::SortCachedClassInfo(mClassInfo, mParentClass.Get(), ChildClasses);
}