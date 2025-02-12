// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiToolWindow.h"
#include "Utils/ClassHierarchyInfo.h"

class IMGUITOOLS_API FImGuiCDOExplorer : public FImGuiToolWindow
{
public:
	FImGuiCDOExplorer();
	virtual ~FImGuiCDOExplorer() = default;

	// FImGuiToolWindow Interface
	virtual void ImGuiUpdate(float DeltaTime) override;
	virtual bool IsEditorToolAllowed() override { return true;}
	// FImGuiToolWindow Interface

	void SelectClassLoadSubclasses(UClass* ParentClass, bool LoadAll);

private:
	ImGuiTools::FHierarchicalRootClassInfo mCDOExplorerClasses;
};