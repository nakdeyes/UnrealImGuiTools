// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// forward declarations
class FImGuiToolsGameDebugger;
class FImGuiToolsManager;

class IMGUITOOLS_API FImGuiToolsModule : public IModuleInterface
{
public:

	// IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// ~IModuleInterface
	
	TSharedPtr<FImGuiToolsManager> GetToolsManager();
	TSharedPtr<FImGuiToolsGameDebugger> GetGameDebugger();

private:
	TSharedPtr<FImGuiToolsManager> ToolsManager;
	TSharedPtr<FImGuiToolsGameDebugger> GameDebugger;
};
