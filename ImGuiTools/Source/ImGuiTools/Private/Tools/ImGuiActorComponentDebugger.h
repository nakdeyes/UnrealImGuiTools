// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiToolWindow.h"

namespace ImGuiActorCompUtils
{
	// Cached info for a given UClass such as object instances and child class indicies.
	struct FCachedClassInfo
	{
		// A class and array of objects of that class.
		TWeakObjectPtr<UClass>          Class;
		TArray<TWeakObjectPtr<UObject>> Objects;

		// Array of indices in parent container array that point at direct child classes
		TArray<int> ChildClassIndicies;
		// A cached count of actors in this class and child classes down the hierarchy. Can be invalidated by setting to -1
		int CachedActorHierarchyCount = -1;

		// Will cache actor hierarchy count on this class cache ( and any children as a by product ) and return the result.
		int CacheActorHierarchyCount(TArray<FCachedClassInfo>& ParentContainer);
	};
}   // namespace ImGuiActorCompUtils

class IMGUITOOLS_API FImGuiActorComponentDebugger : public FImGuiToolWindow
{
public:
	FImGuiActorComponentDebugger();
	virtual ~FImGuiActorComponentDebugger() = default;

	// FImGuiToolWindow Interface
	virtual void ImGuiUpdate(float DeltaTime) override;
	virtual void UpdateTool(float DeltaTime) override;
	// FImGuiToolWindow Interface
};