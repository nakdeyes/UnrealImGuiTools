// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

class FImGuiToolsEditorElements;

class IMGUITOOLSEDITOR_API FImGuiToolsEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FImGuiToolsEditorElements> ToolsElements;
};
