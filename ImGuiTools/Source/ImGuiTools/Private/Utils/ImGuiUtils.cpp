// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "Utils/ImGuiUtils.h"

#include "GameplayTagContainer.h"
#if (ENGINE_MAJOR_VERSION > 5) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
#include "StructUtils/InstancedStruct.h"
#else
#include "InstancedStruct.h"
#endif

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

					ImGui::TableNextColumn();
					ImGui::Text("%s", Ansi(*Prop->GetName()));
					ImGui::Text("%s", Ansi(*Prop->GetClass()->GetName()));
					ImGui::TableNextColumn();

					DrawPropertyValue(Prop, ItemDataMem);
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

void ImGuiTools::Utils::DrawPropertyValue(const FProperty* Prop, const void* Obj)
{
	const void* ObjValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);

	if (Prop->IsA(FBoolProperty::StaticClass()))
	{
		const bool Value = *(const bool*)ObjValuePtr;
		ImGui::Text(Value ? "true" : "false");
	}
	else if (Prop->IsA(FFloatProperty::StaticClass()) ||
		Prop->IsA(FDoubleProperty::StaticClass()))
	{
		const float Value = *(const float*)ObjValuePtr;
		ImGui::Text("% .05f", Value);
	}
	else if (Prop->IsA(FIntProperty::StaticClass()))
	{
		const int32 Value = *(const int32*)ObjValuePtr;
		ImGui::Text("% d", Value);
	}
	else if (Prop->IsA(FInt64Property::StaticClass()))
	{
		const int64 Value = *(const int64*)ObjValuePtr;
		ImGui::Text("% lld", Value);
	}
	else if (Prop->IsA(FNameProperty::StaticClass()))
	{
		const FName Value = *(const FName*)ObjValuePtr;
		ImGui::Text("%s", Ansi(*Value.ToString()));
	}
	else if (Prop->IsA(FStrProperty::StaticClass()))
	{
		const FString Value = *(const FString*)ObjValuePtr;
		ImGui::Text("%s", Ansi(*Value));
	}
	else if (Prop->IsA(FTextProperty::StaticClass()))
	{
		const FText& Value = *(const FText*)ObjValuePtr;
		ImGui::Text("%s", Ansi(*Value.ToString()));
	}
	else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
	{
		UObject* ObjValue = ObjProp->GetObjectPropertyValue_InContainer(Obj);
		ImGui::Text("%s", ObjValue ? Ansi(*ObjValue->GetName()) : "*none*");
	}
	else if (const FWeakObjectProperty* WeakObjProp = CastField<FWeakObjectProperty>(Prop))
	{
		UObject* ObjValue = WeakObjProp->GetObjectPropertyValue_InContainer(Obj);
		ImGui::Text("%s", ObjValue ? Ansi(*ObjValue->GetName()) : "*none*");
	}
	else if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
	{
		UScriptStruct* ScriptStruct = StructProp->Struct.Get();
		
		DrawScriptStructProperty(ScriptStruct, ObjValuePtr);
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
	{
		if (FProperty* Inner = ArrayProp->Inner)
		{
			ImGui::BeginChild(ImGui::GetID(Prop), ImVec2(0, 80.0f), false);

			auto* ObjArray = ArrayProp->ContainerPtrToValuePtr<TArray<float>>(Obj);
		
			FString CPPType;
			ArrayProp->GetCPPType(&CPPType, 0);
			FName CPPTypeName(CPPType);
			const TFunction<void(int32, const void*)>* PrintItemFunc = nullptr;

			if (CPPTypeName == TEXT("<float>"))
			{
				static const TFunction<void(int32, const void*)> StaticConverstionFunction = [](int32 index, const void* VoidPtr) {
					const float* TypePtr = static_cast<const float*>(VoidPtr);
					ImGui::Text("%02d - % .03f", index, *TypePtr);
				};
				PrintItemFunc = &StaticConverstionFunction;
			}
			else if (CPPTypeName == TEXT("<uint8>"))
			{
				static const TFunction<void(int32, const void*)> StaticConverstionFunction = [](int32 index, const void* VoidPtr) {
					const uint8* TypePtr = static_cast<const uint8*>(VoidPtr);
					ImGui::Text("%02d - %2hhx", index, *TypePtr);
				};
				PrintItemFunc = &StaticConverstionFunction;
			}
			else if (CPPTypeName == TEXT("<UObject*>"))
			{
				static const TFunction<void(int32, const void*)> StaticConverstionFunction = [](int32 index, const void* VoidPtr) {
					//const UObject* TypePtr = static_cast<const UObject*>(VoidPtr);
					//ImGui::Text("%02d - %s", index, Ansi(*(*TypePtr)->GetName()));
				};
				PrintItemFunc = &StaticConverstionFunction;
			}
			
			// TODO - type size always reporting 4 with that auto.. but can get type size from the prop.. maybe cast to char size array and do multiples? 
			ImGui::Text("size: %d - type : %s - type size: %d - prop size: %d", ObjArray->Num(), Ansi(*CPPType), ObjArray->GetTypeSize(), (int)ArrayProp->GetMinAlignment());

			if (PrintItemFunc)
			{
				for (int i = 0; i < ObjArray->Num(); ++i)
				{
					auto& ArrayEntryData = (*ObjArray)[i];

					(*PrintItemFunc)(i, static_cast<const void*>(&ArrayEntryData));
				}
			}
			else
			{
				ImGui::Text(" type '%s' not implemented", Ansi(*CPPType));
			}
			
			ImGui::EndChild();
		}
		else
		{
			ImGui::Text("ARRAY. Inner Property not found :(");
		}
	}
	else if (const FMapProperty* MapProp = CastField<FMapProperty>(Prop))
	{
		ImGui::Text("MAP!! Not Implemented :(");
	}
	else if (const FSetProperty* SetProp = CastField<FSetProperty>(Prop))
	{
		ImGui::Text("SET!! Not Implemented :(");
	}
	else if (const FEnumProperty* ENumProp = CastField<FEnumProperty>(Prop))
	{
		if (UEnum* Enum = ENumProp->GetEnum())
		{
			const int64 Value = *(const int64*)ObjValuePtr;
			ImGui::Text("%s", Ansi(*FString::Printf(TEXT("(%d)%s::%s"), Value, *Enum->GetName(), *Enum->GetDisplayNameTextByValue(Value).ToString())));
		}
		else
		{
			ImGui::Text("ENUM not valid :(");
		}
	}
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
	{
		const int64 Value = ((int64)(*(const int8*)ObjValuePtr));
		if (UEnum* Enum = ByteProp->Enum)
		{
			ImGui::Text("%s", Ansi(*FString::Printf(TEXT("(%d)%s::%s"), Value, *Enum->GetName(), *Enum->GetDisplayNameTextByValue(Value).ToString())));
		}
		else
		{
			ImGui::Text("% lld", Value);
		}
	}
	else if (const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Prop))
	{
		if (UFunction* SignatureFunction = DelegateProp->SignatureFunction)
		{
			ImGui::Text("%s", Ansi(*SignatureFunction->GetName()));
		}
		else
		{
			ImGui::Text("Delegate has no signature function");
		}
	}
	else if (const FMulticastDelegateProperty* MultiDelegateProp = CastField<FMulticastDelegateProperty>(Prop))
	{
		//if (const FMulticastScriptDelegate* MultiDelegate = MultiDelegateProp->GetMulticastDelegate())
		//{

		//}
		ImGui::Text("MULTICAST DELEGATE!! Not Implemented :(");
	}
	else if (const FMulticastInlineDelegateProperty* MultiInlineDelegateProp = CastField<FMulticastInlineDelegateProperty>(Prop))
	{
		ImGui::Text("MULTICAST INLINE DELEGATE!! Not Implemented :(");
	}
	else if (const FMulticastSparseDelegateProperty* MultiSparseDelegateProp = CastField<FMulticastSparseDelegateProperty>(Prop))
	{
		ImGui::Text("MULTICAST SPARSE DELEGATE!! Not Implemented :(");
	}
	else
	{
		ImGui::Text("Unimplemented!");
	}
}

void ImGuiTools::Utils::DrawScriptStructProperty(const UScriptStruct* StructType, const void* Obj)
{
	if (StructType == FGameplayTag::StaticStruct())
	{
		const FGameplayTag& Value = *(const FGameplayTag*)Obj;
		ImGui::Text("FGameplayTag: \n%s", Ansi(*Value.ToString()));
	}
	else if (StructType == FGameplayTagContainer::StaticStruct())
	{
		const FGameplayTagContainer& Value = *(const FGameplayTagContainer*)Obj;
		ImGui::Text("FGameplayTagContainer: \n%s", Ansi(*Value.ToString()));
	}
	else if (StructType == FGameplayTagQuery::StaticStruct())
	{
		const FGameplayTagQuery& Value = *(const FGameplayTagQuery*)Obj;
		ImGui::Text("FGameplayTagQuery: \n%s", Ansi(*Value.GetDescription()));
	}
	else if (StructType == TBaseStructure<FVector>::Get())
	{
		const FVector& Value = *(const FVector*)Obj;
		ImGui::Text("FVector| \nx: %.03f, y: %.03f, z: %.03f", Value.X, Value.Y, Value.Z);
	}
	else if (StructType == TBaseStructure<FRotator>::Get())
	{
		const FRotator& Value = *(const FRotator*)Obj;
		ImGui::Text("FRotator| \np: %.03f, r: %.03f, y: %.03f", Value.Pitch, Value.Roll, Value.Yaw);
	}
	else if (StructType == TBaseStructure<FQuat>::Get())
	{
		const FQuat& Value = *(const FQuat*)Obj;
		ImGui::Text("FQuat| \nx: %.03f, y: %.03f, z: %.03f, w: % .03f", Value.X, Value.Y, Value.Z, Value.W);
	}
	else if (StructType == TBaseStructure<FTransform>::Get())
	{
		const FTransform& Value = *(const FTransform*)Obj;
		ImGui::Text("FTransform| loc| x:% .03f, y:% .03f, z:% .03f", Value.GetLocation().X, Value.GetLocation().Y, Value.GetLocation().Z);
		ImGui::Text("            rot| x:% .03f, y:% .03f, z:% .03f, w:% .03f", Value.GetRotation().X, Value.GetRotation().Y, Value.GetRotation().Z, Value.GetRotation().W);
		ImGui::Text("          scale| x:% .03f, y:% .03f, z:% .03f", Value.GetScale3D().X, Value.GetScale3D().Y, Value.GetScale3D().Z);
	}
	else
	{
		const FString NodeTitle = FString::Printf(TEXT("%s (struct)##%p"), *GetNameSafe(StructType), Obj);
		if (ImGui::TreeNode(Ansi(*NodeTitle)))
		{
			static constexpr ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
			if (ImGui::BeginTable("PropertyTable", 2, flags))
			{
				ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
					
				for (FProperty* StructChildProp : TFieldRange<FProperty>(StructType))
				{
					if (StructChildProp)
					{
						ImGui::TableNextColumn();
						ImGui::Text("%s", Ansi(*StructChildProp->GetName()));
						ImGui::Text("%s", Ansi(*StructChildProp->GetClass()->GetName()));
						ImGui::TableNextColumn();
			
						DrawPropertyValue(StructChildProp, Obj);
					}
				}
											
				ImGui::EndTable();
			}
				
			ImGui::TreePop();
		}
	}
}

ImVec4 ImGuiTools::Utils::InterpolateImGuiColor(const ImVec4& StartColor, const ImVec4& EndColor, float Time)
{
	return ImVec4(
		StartColor.x + ((EndColor.x - StartColor.x) * Time),
		StartColor.y + ((EndColor.y - StartColor.y) * Time),
		StartColor.z + ((EndColor.z - StartColor.z) * Time),
		StartColor.w + ((EndColor.w - StartColor.w) * Time)
	);
}
