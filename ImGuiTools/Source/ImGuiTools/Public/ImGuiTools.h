// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// forward declarations
class FImGuiToolsManager;

class IMGUITOOLS_API FImGuiToolsModule : public IModuleInterface
{
public:

	// IModuleInterface
	virtual void StartupModule() override;
	// ~IModuleInterface
	
	TSharedPtr<FImGuiToolsManager> GetToolsManager();

private:
	TSharedPtr<FImGuiToolsManager> ToolsManager;
};
