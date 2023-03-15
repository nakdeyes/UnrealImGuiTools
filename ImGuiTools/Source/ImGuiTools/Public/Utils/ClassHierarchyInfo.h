// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once
#include "UObject/SoftObjectPtr.h"

// forward declarations
class UClass;
class UObject;

namespace ImGuiTools
{
	void DrawClassHierarchy(UObject* Obj, UClass* TerminatingParentClass = UObject::StaticClass());

	struct FHierarchicalClassInfo
	{
		// This will only be set for loaded classes, check validity for load status.
		TWeakObjectPtr<UClass> mClass;

		TArray<FHierarchicalClassInfo> mLoadedChildren;

		struct UnloadedClassInfo
		{
			TSoftClassPtr<UObject>  SoftClassInfo;
			FString                 ClassName;
		};
		TArray<UnloadedClassInfo> mUnloadedChildren;

		// How many unloaded descendants of this class exist. Cached once after hierarchy is built.
		int mCachedAllUnloadedDecendants = 0;

		// Load any unloaded child classes, if LoadAllDescendants is true, all descendants even past direct children will be loaded
		void LoadChildren(bool LoadAllDescendants = true);
	};

	// A root class info that holds additional data for the whole tree like search directory, etc.
	struct FHierarchicalRootClassInfo
	{
		FString mSearchDirectory;
		FHierarchicalClassInfo mRootClassInfo;

		// Reset the view to this root class
		void ResetToRootClass(UClass* RootClass, FString SearchDirectory = FString(TEXT("/Game/")));
		// Reset to the currently selected root class
		void Reset();
	};
}