// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiToolsEditorStyle.h"

#include <CoreMinimal.h>
#include <Framework/Commands/Commands.h>

// forward declarations
class FImGuiToolWindow;

class FImGuiToolsEditorCommands : public TCommands<FImGuiToolsEditorCommands>
{
public:

	FImGuiToolsEditorCommands()
		: TCommands<FImGuiToolsEditorCommands>(TEXT("ImGuiTools"), NSLOCTEXT("Contexts", "ImGuiTools", "ImGuiTools Plugin"), NAME_None, FImGuiToolsEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> ImGuiToolEnabledCommand;
};

class FImGuiToolsEditorElements
{
public:
	// Should be called by the owner.. probably at module startup time
	void RegisterElements();

private:
	void OnModulesChanged(FName Name, EModuleChangeReason Reason);
	FDelegateHandle OnModulesChangedHandler;
	
	// Actually registers the elements, assuming the level editor module is present. 
	void RegisterElements_Internal();
	TSharedPtr<FUICommandList> CommandList;
};