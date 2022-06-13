// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsEditor.h"

#include "ImGuiToolsEditorStyle.h"
#include "ImGuiToolsEditorCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FImGuiToolsModule"

void FImGuiToolsEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FImGuiToolsEditorStyle::Initialize();
	FImGuiToolsEditorStyle::ReloadTextures();
	
	FImGuiToolsEditorCommands::Register();

	ToolsElements = MakeShareable(new FImGuiToolsEditorElements());
	ToolsElements->RegisterElements();
}

void FImGuiToolsEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FImGuiToolsEditorStyle::Shutdown();

	FImGuiToolsEditorCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FImGuiToolsEditorModule, ImGuiToolsEditor)