// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Engine/DeveloperSettings.h>

#include "ImGuiToolsDeveloperSettings.generated.h"

UCLASS(config = ImGui, meta = (DisplayName = "ImGui Tools Settings"))
class IMGUITOOLS_API UImGuiToolsDeveloperSettings : public UDeveloperSettings
{
public:
	GENERATED_BODY()

	UImGuiToolsDeveloperSettings();

	// Display the ImGui Tools Editor Button. Requires Editor restart to take effect.
	UPROPERTY(config, EditAnywhere)
	bool DisplayEditorButton = true;

	// If true, when the ImGui Tools are enabled, ImGui Input is enabled, when they are disabled, the input is disabled. If the tools aren't enabled/disabled, the input is not toggled.
	UPROPERTY(config, EditAnywhere)
	bool SetImGuiInputOnToolsEnabled = true;

	// Array of keys for a key chord, defining a key short cut to toggle ImGui Input.
	UPROPERTY(config, EditAnywhere)
	TArray<FKey> ImGuiToggleInputKeys;

	// Array of keys for a key chord, defining a key short cut to toggle ImGui Visibility.
	UPROPERTY(config, EditAnywhere)
	TArray<FKey> ImGuiToggleVisibilityKeys;
};