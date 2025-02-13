// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once
#include "CoreMinimal.h"

// types
typedef int ImGuiWindowFlags;

class IMGUITOOLS_API FImGuiToolWindow
{
public:
	FImGuiToolWindow() = default;
	virtual ~FImGuiToolWindow() = default;

	// Implement these in your tools
	virtual void ImGuiUpdate(float DeltaTime) = 0;

	// Called by ImGuiToolsManager
	void EnableTool(bool bNewEnabled);
	bool& GetEnabledRef();
	const FString& GetToolName() const;
	const ImGuiWindowFlags& GetFlags() const;
	virtual void UpdateTool(float DeltaTime);
	virtual bool IsEditorToolAllowed() { return false;};
	
protected:
	bool bEnabled = false;
	FString ToolName;
	ImGuiWindowFlags WindowFlags = 0;
};