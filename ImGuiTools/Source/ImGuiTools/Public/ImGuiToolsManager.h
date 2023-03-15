// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <HAL/IConsoleManager.h>
#include <ConsoleSettings.h>
#include <Tickable.h>

// forward declarations
class FImGuiToolWindow;

// types
typedef TMap<FName, TArray<TSharedPtr<FImGuiToolWindow>>> ToolNamespaceMap;

// CVARs
namespace ImGuiDebugCVars
{
	extern IMGUITOOLS_API TAutoConsoleVariable<bool> CVarImGuiToolsEnabled;
} // namespace ImGuiDebugCVars

class IMGUITOOLS_API FImGuiToolsManager : public FTickableGameObject
{
public:
	FImGuiToolsManager();
	virtual ~FImGuiToolsManager();

	// Used to init/register any tools that internal to this plugin
	void InitPluginTools();
	void RegisterToolWindow(TSharedPtr<FImGuiToolWindow> ToolWindow, FName ToolNamespace = NAME_None);

	// FTickableGameObject
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	// ~FTickableGameObject

	ToolNamespaceMap& GetToolsWindows();
	TSharedPtr<FImGuiToolWindow> GetToolWindow(const FString& ToolWindowName, FName ToolNamespace = NAME_None);

	static void ToggleToolVisCommand(const TArray<FString>& Args);
    static void ToggleToolsVisCommand(const TArray<FString>& Args);

private:
	ToolNamespaceMap ToolWindows;
	bool DrawImGuiDemo;
	bool ShowFPS;

	void RegisterAutoCompleteEntries(TArray<FAutoCompleteCommand>& Commands) const;

	FConsoleVariableDelegate OnEnabledCVarValueChanged;
};