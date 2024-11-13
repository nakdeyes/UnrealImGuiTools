// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "Utils/ImGuiUtils.h"

#include "InstancedStruct.h"

ImGuiTools::Utils::FShowCols::FShowCols(int ColCount, bool* OptionalDefaultColArray /*= nullptr*/)
{
	if (ColCount > MAX_COLUMNS)
	{
		// horrible error!
	}

	TotalShowColCount = ColCount;
	CachedShowColCount = 0;
	for (int i = 0; i < TotalShowColCount; ++i)
	{
		ShowColFlags[i] = (OptionalDefaultColArray == nullptr) ? 0 : OptionalDefaultColArray[i];
	}
}

bool& ImGuiTools::Utils::FShowCols::GetShowCol(int ColIndex)
{
	// error if ColIndex out of bounds?
	return ShowColFlags[ColIndex];
}

//void ImGuiTools::Utils::FShowCols::SetShowCol(int ColIndex, bool NewShowValue)
//{
//	// error if ColIndex out of bounds?
//	ShowColFlags[ColIndex] = NewShowValue;
//}

int ImGuiTools::Utils::FShowCols::CacheShowColCount()
{
	CachedShowColCount = 0;
	for (int i = 0; i < TotalShowColCount; ++i)
	{
		if (ShowColFlags[i])
		{
			++CachedShowColCount;
		}
	}
	return CachedShowColCount;
}

int ImGuiTools::Utils::FShowCols::GetCachedShowColCount()
{
	return CachedShowColCount;
}

void ImGuiTools::Utils::FWorldCache::TryCacheWorlds()
{
    // Remove any Worlds that may now be invalid.
    CachedWorlds.RemoveAll([](const TWeakObjectPtr<UWorld>& World) {
        return !IsValid(World.Get());
    });

    // What types of worlds to display: 
    static const auto WorldShouldDisplay = [](UWorld* World)
    {
        return (IsValid(World) && !World->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && World->bIsWorldInitialized
            && (World->WorldType != EWorldType::Editor) && (World->WorldType != EWorldType::EditorPreview));
    };

    for (TObjectIterator<UWorld> It; It; ++It)
    {
        UWorld* World = *It;
        if (!WorldShouldDisplay(World))
        {
            continue;
        }

        // if this is a new world, add a new cached world actor object.
        if (!CachedWorlds.ContainsByPredicate([World](const TWeakObjectPtr<UWorld>& CachedWorldWeakPtr) { return (CachedWorldWeakPtr.Get() == World); }))
        {
            CachedWorlds.Add(TWeakObjectPtr<UWorld>(World));
        }
    }
}

void ImGuiTools::Utils::DrawInstancedStructTable(const char* StringId, const FInstancedStruct& InstancedStruct, ImVec2 Size /*= ImVec2(0,0)*/, bool DoTreeNode /*= false*/)
{
	if (!InstancedStruct.IsValid())
	{
		return;
	}


	const bool TreeNodeOpen = DoTreeNode && ImGui::TreeNode(StringId);
	
	static constexpr ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
	if (!DoTreeNode || TreeNodeOpen)
	{
		ImGui::BeginChild(ImGui::GetID(InstancedStruct.GetMemory()), Size);

		if (ImGui::BeginTable("PropertyTable", 2, flags))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			const void* ItemDataMem = static_cast<const void*>(InstancedStruct.GetMemory());

			for (FProperty* Prop : TFieldRange<FProperty>(InstancedStruct.GetScriptStruct()))
			{
				if (Prop)
				{
					const void* ObjValuePtr = Prop->ContainerPtrToValuePtr<void>(ItemDataMem);
												
					ImGui::TableNextColumn();
					ImGui::Text("%s", Ansi(*Prop->GetName()));
					ImGui::Text("%s", Ansi(*Prop->GetClass()->GetName()));
					ImGui::TableNextColumn();
					if (Prop->IsA(FFloatProperty::StaticClass()))
					{
						const float Value = *(const float*)ObjValuePtr;
						ImGui::Text("% .02f", Value);
					}
					else if (Prop->IsA(FIntProperty::StaticClass()))
					{
						const int32 Value = *(const int32*)ObjValuePtr;
						ImGui::Text("% d", Value);
					}
					else if (Prop->IsA(FBoolProperty::StaticClass()))
					{
						const bool Value = *(const bool*)ObjValuePtr;
						ImGui::Text(Value ? "true" : "false");
					}
					else
					{
						ImGui::Text("-");
					}
				}
			}
										
			ImGui::EndTable();
			ImGui::EndChild();
		}
	}

	if (TreeNodeOpen)
	{
		ImGui::TreePop();
	}

}
