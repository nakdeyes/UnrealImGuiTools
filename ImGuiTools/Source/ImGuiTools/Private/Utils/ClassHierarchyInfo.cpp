// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "Utils/ClassHierarchyInfo.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Utils/ImGuiUtils.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Blueprint.h"

namespace ClassHierarchyUtil
{
	void CreateAndSortLoadedHierarchy(ImGuiTools::FHierarchicalClassInfo& ClassInfo, TArray<UClass*>& LoadedClasses)
	{
		for (UClass* PotentialChildClass : LoadedClasses)
		{
			if (PotentialChildClass && (PotentialChildClass->GetSuperClass() == ClassInfo.mClass.Get()))
			{
				ImGuiTools::FHierarchicalClassInfo& ChildClassInfo = ClassInfo.mLoadedChildren.AddDefaulted_GetRef();
				ChildClassInfo.mClass = PotentialChildClass;

				CreateAndSortLoadedHierarchy(ChildClassInfo, LoadedClasses);
			}
		}

		// Sort child classes alphabetically
		ClassInfo.mLoadedChildren.Sort([](const ImGuiTools::FHierarchicalClassInfo& A, const ImGuiTools::FHierarchicalClassInfo& B) {
			return A.mClass->GetFName().Compare(B.mClass->GetFName()) < 0;
		});
	}

	bool InsertIntoChildOrSelf(ImGuiTools::FHierarchicalClassInfo& ClassInfo, ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo& UnloadedClass, IAssetRegistry& AssetRegistry)
	{
		// for this unloaded class, see if class info class is an ancestor, if so, see if any children are as well first to claim ownership.
		TArray<FName> AncestorNames;
		AssetRegistry.GetAncestorClassNames(FName(UnloadedClass.ClassName), AncestorNames);

		if (AncestorNames.Contains(ClassInfo.mClass->GetFName()))
		{
			// check for child inheritance first
			bool ChildInherited = false;
			for (ImGuiTools::FHierarchicalClassInfo& ChildClassInfo : ClassInfo.mLoadedChildren)
			{
				ChildInherited |= InsertIntoChildOrSelf(ChildClassInfo, UnloadedClass, AssetRegistry);
			}

			if (!ChildInherited)
			{
				// No children claimed, so claim it for this class.
				ClassInfo.mUnloadedChildren.Add(UnloadedClass);
				return true;
			}
			return ChildInherited;
		}

		return false;
	}

	void SortUnloadedHierarchy(ImGuiTools::FHierarchicalClassInfo& ClassInfo, TArray<ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo>& UnloadedClasses, IAssetRegistry& AssetRegistry)
	{
		for (ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo& UnloadedClass : UnloadedClasses)
		{
			InsertIntoChildOrSelf(ClassInfo, UnloadedClass, AssetRegistry);
		}
	}

	// Cache ClassInfo Descendant Count of self and all descendants, returning the count for the current classinfo
	int CacheUnloadedDescendantCounts(ImGuiTools::FHierarchicalClassInfo& ClassInfo)
	{
		int DescendantCount = 0;
		for (ImGuiTools::FHierarchicalClassInfo& LoadedChild : ClassInfo.mLoadedChildren)
		{
			DescendantCount += CacheUnloadedDescendantCounts(LoadedChild);
		}
		DescendantCount += ClassInfo.mUnloadedChildren.Num();
		ClassInfo.mCachedAllUnloadedDecendants = DescendantCount;
		return DescendantCount;
	}
}

void ImGuiTools::FHierarchicalClassInfo::LoadChildren(bool LoadAllDescendants /*= true*/)
{
	if (LoadAllDescendants)
	{
		for (ImGuiTools::FHierarchicalClassInfo& LoadedChildInfo : mLoadedChildren)
		{
			LoadedChildInfo.LoadChildren(LoadAllDescendants);
		}
	}

	for (ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo& UnloadedChildInfo : mUnloadedChildren)
	{
		UnloadedChildInfo.SoftClassInfo.LoadSynchronous();
	}
}

// Reset the view to this root class
void ImGuiTools::FHierarchicalRootClassInfo::ResetToRootClass(UClass* RootClass, FString SearchDirectory /*= FString(TEXT("/Game/"))*/)
{
	// Set desired parameters then reset()
	mSearchDirectory = SearchDirectory;
	mRootClassInfo.mClass = RootClass;

	Reset();
}
// Reset to the currently selected root class
void ImGuiTools::FHierarchicalRootClassInfo::Reset()
{
	mRootClassInfo.mLoadedChildren.Empty();
	mRootClassInfo.mUnloadedChildren.Empty();
	mRootClassInfo.mCachedAllUnloadedDecendants = 0;

	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	// The asset registry is populated asynchronously at startup, so there's no guarantee it has finished.
	// This simple approach just runs a synchronous scan on the entire content directory.
	TArray<FString> PathsToScan;
	PathsToScan.Add(mSearchDirectory);
	AssetRegistry.ScanPathsSynchronous(PathsToScan);

	TArray<UClass*>                                                 LoadedClasses;
	TArray<ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo>   UnloadedClasses;

	// Gather Native classes, they are loaded already
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
		if (!Class->IsChildOf(mRootClassInfo.mClass.Get()))
		{
			continue;
		}

		LoadedClasses.Add(Class);
	}

	// Get all BP Classes
	TSet<FName> DerivedNames;
	{
		TArray<FName> BaseNames;
		BaseNames.Add(mRootClassInfo.mClass->GetFName());

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
						TSoftClassPtr<UObject> AssetSubClass = TSoftClassPtr<UObject>(FSoftObjectPath(ClassObjectPath));
						if (UClass* SubClassUClass = Cast<UClass>(AssetSubClass.Get()))
						{
							// Loaded classes have UClass available already.
							LoadedClasses.Add(SubClassUClass);
						}
						else
						{
							// Unloaded classes, we store a copy to the soft class ptr for loading later
							ImGuiTools::FHierarchicalClassInfo::UnloadedClassInfo& UnloadedClassInfo = UnloadedClasses.AddDefaulted_GetRef();
							UnloadedClassInfo.ClassName = ClassName;
							UnloadedClassInfo.SoftClassInfo = AssetSubClass;
						}
					}
				}
			}
		}
	}

	// We should have all loaded and unloaded subclasses. Build hierarchy with loaded classes first, then find unloaded parents.
	ClassHierarchyUtil::CreateAndSortLoadedHierarchy(mRootClassInfo, LoadedClasses);
	ClassHierarchyUtil::SortUnloadedHierarchy(mRootClassInfo, UnloadedClasses, AssetRegistry);

	// Cache descendant count
	ClassHierarchyUtil::CacheUnloadedDescendantCounts(mRootClassInfo);
}

////// Drawing helper functions
void ImGuiTools::DrawClassHierarchy(UObject* Obj, UClass* TerminatingParentClass /*= UObject::StaticClass()*/)
{
	UClass* CurClass = Obj->GetClass();
	TArray<UClass*> ClassHierarchy;
	ClassHierarchy.Add(CurClass);
	while (CurClass && CurClass != TerminatingParentClass)
	{
		CurClass = CurClass->GetSuperClass();
		ClassHierarchy.Add(CurClass);
	}

	int j = 0;
	for (int i = ClassHierarchy.Num() - 1; i >= 0; --i)
	{
		if (UClass* Class = ClassHierarchy[i])
		{
			const bool FirstEntry = (j++ == 0);
			if (FirstEntry)
			{
				ImGui::Text("%s", Ansi(*Class->GetName()));
			}
			else
			{
				ImGui::NewLine();
				ImGui::SameLine(j * 8);
				ImGui::Text("|-> %s", Ansi(*Class->GetName()));
			}
		}
	}
}
