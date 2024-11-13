// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsDeveloperSettings.h"

#include "ImGuiToolsManager.h"

UImGuiToolsDeveloperSettings::UImGuiToolsDeveloperSettings()
	: Super()
{
	
	static const KeyCodePair LeftCtrlKeyCode		= { 162, 0 };
	static const KeyCodePair BackslashKeyCode		= { 220, 92 };
	static const KeyCodePair RightBracketKeyCode	= { 221, 93 };

	if (ImGuiToggleInputKeys.IsEmpty())
	{
		// Add Default Toggle input keys if none are set.
		ImGuiToggleInputKeys.Add(FInputKeyManager::Get().GetKeyFromCodes(LeftCtrlKeyCode.Get<0>(), LeftCtrlKeyCode.Get<1>()));
		ImGuiToggleInputKeys.Add(FInputKeyManager::Get().GetKeyFromCodes(BackslashKeyCode.Get<0>(), BackslashKeyCode.Get<1>()));
	}

	if (ImGuiToggleVisibilityKeys.IsEmpty())
	{
		// Add Default Toggle visibility keys if none are set.
		ImGuiToggleVisibilityKeys.Add(FInputKeyManager::Get().GetKeyFromCodes(LeftCtrlKeyCode.Get<0>(), LeftCtrlKeyCode.Get<1>()));
		ImGuiToggleVisibilityKeys.Add(FInputKeyManager::Get().GetKeyFromCodes(RightBracketKeyCode.Get<0>(), RightBracketKeyCode.Get<1>()));
	}
}
