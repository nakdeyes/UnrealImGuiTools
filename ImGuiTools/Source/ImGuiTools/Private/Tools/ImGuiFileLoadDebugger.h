// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiToolWindow.h"

#include "Misc/CoreDelegates.h"
#include "Runtime/Launch/Resources/Version.h"

class IMGUITOOLS_API FImGuiFileLoadDebugger : public FImGuiToolWindow
{
public:
	FImGuiFileLoadDebugger();
	virtual ~FImGuiFileLoadDebugger() = default;

	// FImGuiToolWindow Interface
	virtual void ImGuiUpdate(float DeltaTime) override;
	// FImGuiToolWindow Interface

	static void ToggleRecordCommand(const TArray<FString>& Args);
	void ToggleRecord();

	// Core Delegate callbacks
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	void OnAsyncLoadPackage(const FStringView PackageNameStrView);
#else   // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    void OnAsyncLoadPackage(const FString& PackageName);
#endif  // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	void OnSyncLoadPackage(const FString& PackageName);

private:
	// Are loads currently being recorded
	bool bRecordingLoads = false;
	bool bSpewToLog = true;

	TArray<FString> AsyncLoadFiles;
	TArray<FString> SyncLoadFiles;

	FDelegateHandle AsyncLoadDelegateHandle;
	FDelegateHandle SyncLoadDelegateHandle;
};