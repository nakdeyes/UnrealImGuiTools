// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Engine/DeveloperSettings.h>

#include "ImGuiToolsDeveloperSettings.generated.h"

UCLASS(config = ImGui, meta = (DisplayName = "ImGui Tools Settings"))
class IMGUITOOLS_API UImGuiToolsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Developer Settings

	// Display the ImGui Tools Editor Button. Requires Editor restart to take effect.
	UPROPERTY(config, EditAnywhere)
	bool DisplayEditorButton = true;

	// If true, when the ImGui Tools are enabled, ImGui Input is enabled, when they are disabled, the input is disabled. If the tools aren't enabled/disabled, the input is not toggled.
	UPROPERTY(config, EditAnywhere)
	bool SetImGuiInputOnToolsEnabled = true;
};