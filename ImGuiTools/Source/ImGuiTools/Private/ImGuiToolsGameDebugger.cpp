// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiToolsGameDebugger.h"

#include <imgui.h>
#include "Utils/ImGuiUtils.h"
#include <GameFramework/Actor.h>
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/PlayerController.h"


namespace GameDebuggerUtil
{
	void DrawActorMenu(const char* ActorDesc, AActor* Actor)
	{
		const bool ActorValid = IsValid(Actor);
		const FString ActorMenuStr = FString::Printf(TEXT("%s: %s"), ActorDesc, ActorValid ? *Actor->GetName() : TEXT("*none*"));

		if (!ActorValid)
		{
			ImGui::Text("%s", Ansi(*ActorMenuStr));
			return;
		}
		
		if (ImGui::BeginMenu(Ansi(*ActorMenuStr)))
		{
			
			ImGui::EndMenu();
		}
		
	}
} // namespace GameDebuggerUtil

void FImGuiToolsGameDebugger::DrawMainImGuiMenu()
{
#if 0 // This is super WIP and disabled at the moment.
	
	if (ImGui::BeginMenu("Game"))
	{
		// What types of worlds to display: 
		static const auto WorldShouldDisplay = [](UWorld* World) 
		{
			return (IsValid(World) && !World->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && World->bIsWorldInitialized 
				&& (World->WorldType != EWorldType::Editor) && (World->WorldType != EWorldType::EditorPreview));
		};

		APlayerController* LocalPC = nullptr;
		for (TObjectIterator<UWorld> It; It && (LocalPC == nullptr); ++It)
		{
			UWorld* World = *It;
			if (!WorldShouldDisplay(World))
			{
				continue;
			}

			LocalPC = World->GetFirstPlayerController();
		}

		if (IsValid(LocalPC))
		{
			
			ImGui::Text("World w/ PC: %s - %s", Ansi(*LocalPC->GetWorld()->GetName()), Ansi(*LocalPC->GetName()));
			if (ImGui::BeginMenu(Ansi(*FString::Printf(TEXT("%s##LocalPCMenu"), *LocalPC->GetName()))))
			{
				APawn* CurPawn = LocalPC->GetPawn();
				ImGui::Text("Current Pawn: %s", CurPawn ? Ansi(*CurPawn->GetName()) : "*none*");
					
				ImGui::EndMenu();
			}
		}

		ImGui::EndMenu(); // "Game"
	}
#endif // 0
}

void FImGuiToolsGameDebugger::RegisterActorComponentCustomization(const FActorComponentCustomization& Customization)
{
	// Unregister for any existing cusotmization of this component type
	UnregisterActorComponentCustomization(Customization.ComponentClass);

	ActorCompCustomizations.Add(Customization);
}

void FImGuiToolsGameDebugger::UnregisterActorComponentCustomization(const TSubclassOf<UActorComponent> ComponentClass)
{
	ActorCompCustomizations.RemoveAll([ComponentClass](const FActorComponentCustomization& ActorCompCustomization) -> bool {
		return (ActorCompCustomization.ComponentClass == ComponentClass);
	});
}
