// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <ConsoleSettings.h>
#include <Framework/Application/IInputProcessor.h>
#include <HAL/IConsoleManager.h>
#include <Tickable.h>

// forward declarations
class FImGuiToolWindow;

// types
typedef TTuple<uint32, uint32> KeyCodePair;
typedef TMap<FName, TArray<TSharedPtr<FImGuiToolWindow>>> ToolNamespaceMap;

// CVARs
namespace ImGuiDebugCVars
{
	extern IMGUITOOLS_API TAutoConsoleVariable<bool> CVarImGuiToolsEnabled;
} // namespace ImGuiDebugCVars

class FImGuiToolsInputProcessor : public IInputProcessor
{
public:
	//~ IInputProcess overrides
	bool HandleKeyDownEvent( FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent ) override;
	bool HandleKeyUpEvent( FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent ) override;
	void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {};

private:
	TArray<KeyCodePair> KeyCodesDown;

	void CheckForToggleShortcutState();
	bool ToggleInputDown = false;
	bool ToggleVisDown = false;
};

class IMGUITOOLS_API FImGuiToolsManager
{
public:
	FImGuiToolsManager();
	virtual ~FImGuiToolsManager();

	// Used to init/register any tools that internal to this plugin
	void Initialize();
	void Deinitialize();

	// Used to register tools externally
	void RegisterToolWindow(TSharedPtr<FImGuiToolWindow> ToolWindow, FName ToolNamespace = NAME_None);

	ToolNamespaceMap& GetToolsWindows();
	TSharedPtr<FImGuiToolWindow> GetToolWindow(const FString& ToolWindowName, FName ToolNamespace = NAME_None);

	static void ToggleToolVisCommand(const TArray<FString>& Args);
    static void ToggleToolsVisCommand(const TArray<FString>& Args);

	// Delegate callbacks
	void OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);

private:
	ToolNamespaceMap ToolWindows;
	bool DrawImGuiDemo;
	bool ShowFPS;

	void RegisterAutoCompleteEntries(TArray<FAutoCompleteCommand>& Commands) const;

	FConsoleVariableDelegate OnEnabledCVarValueChanged;

	TSharedPtr<FImGuiToolsInputProcessor> InputProcessor = nullptr;

	FDelegateHandle OnWorldPostActorTickDelegateHandle;
};