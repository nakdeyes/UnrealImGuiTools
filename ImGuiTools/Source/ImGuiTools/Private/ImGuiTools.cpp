// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiTools.h"
#include "ImGuiToolsManager.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "FImGuiToolsModule"

void FImGuiToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	ToolsManager = MakeShareable(new FImGuiToolsManager());
	ToolsManager->InitPluginTools();
}

TSharedPtr<FImGuiToolsManager> FImGuiToolsModule::GetToolsManager()
{
	return ToolsManager;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FImGuiToolsModule, ImGuiTools)