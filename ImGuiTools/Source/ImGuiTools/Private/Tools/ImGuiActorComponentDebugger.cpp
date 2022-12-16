// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiActorComponentDebugger.h"

#include "Utils/ClassHierarchyInfo.h"
#include "Utils/ImGuiUtils.h"

#include <imgui.h>
#include <EngineUtils.h>

namespace ImGuiActorCompUtils
{
    ///////////////////////////////////////
    /////////  Static Drawing functions

	// Get a nice string representations of an actors replication settings
	FString GetReplicationString_Actor(AActor* Act)
	{
		return Act->GetIsReplicated()
			? FString::Printf(TEXT("true - %s"), *StaticEnum<ENetRole>()->GetNameStringByValue(Act->GetLocalRole()))
			: FString(TEXT("false"));
	}
	FString GetReplicationString_Component(UActorComponent* Comp)
	{
		return Comp->GetIsReplicated()
			? FString::Printf(TEXT("true - %s"), *StaticEnum<ENetRole>()->GetNameStringByValue(Comp->GetOwnerRole()))
			: FString(TEXT("false"));
	}

	void DrawPropertyValue(FProperty* Prop, void* Obj)
	{
		UObject* ObjValuePtr = Prop->ContainerPtrToValuePtr<UObject>(Obj);

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
		else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			if (FProperty* Inner = ArrayProp->Inner)
			{
				ImGui::BeginChild(ImGui::GetID(Prop), ImVec2(0, 100.0f), true);

				auto* ObjArray = ArrayProp->ContainerPtrToValuePtr<TArray<float>>(Obj);
			
				FString CPPType;
				ArrayProp->GetCPPType(&CPPType, 0);
				FName CPPTypeName(CPPType);
				const TFunction<void(int32, void*)>* PrintItemFunc = nullptr;

				if (CPPTypeName == TEXT("<float>"))
				{
					static const TFunction<void(int32, void*)> StaticConverstionFunction = [](int32 index, void* VoidPtr) {
						float* TypePtr = static_cast<float*>(VoidPtr);
						ImGui::Text("%02d - % .03f", index, *TypePtr);
					};
					PrintItemFunc = &StaticConverstionFunction;
				}
				else if (CPPTypeName == TEXT("<uint8>"))
				{
					static const TFunction<void(int32, void*)> StaticConverstionFunction = [](int32 index, void* VoidPtr) {
						uint8* TypePtr = static_cast<uint8*>(VoidPtr);
						ImGui::Text("%02d - %2hhx", index, *TypePtr);
					};
					PrintItemFunc = &StaticConverstionFunction;
				}
				else if (CPPTypeName == TEXT("<UObject*>"))
				{
					static const TFunction<void(int32, void*)> StaticConverstionFunction = [](int32 index, void* VoidPtr) {
						UObject** TypePtr = static_cast<UObject**>(VoidPtr);
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

						(*PrintItemFunc)(i, static_cast<void*>(&ArrayEntryData));
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
				ImGui::Text("%lld", Value);
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
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			ImGui::Text("Struct!! Not Implemented :(");
		}
		else
		{
			ImGui::Text("Unimplemented!");
		}
	}

	void DrawUProperties(UObject* Obj)
	{
		ImGui::Columns(4);

		ImGui::Text("Property Name"); ImGui::NextColumn();
		ImGui::Text("Property Value"); ImGui::NextColumn();
		ImGui::Text("Property Type"); ImGui::NextColumn();
		ImGui::Text("Property CPP Type"); ImGui::NextColumn();
		
		ImGui::Columns(1);

		ImGui::BeginChild(Ansi(*FString::Printf(TEXT("%s-Props"), *Obj->GetName())), ImVec2(0, 300.0f), true);
		ImGui::Columns(4);

		UClass* ObjClass = Obj->GetClass();

		for (FProperty* Prop : TFieldRange<FProperty>(ObjClass))
		{
			ImGui::Text("%s", Ansi(*Prop->GetName())); ImGui::NextColumn();
			DrawPropertyValue(Prop, Obj); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*Prop->GetClass()->GetName())); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*Prop->GetCPPType())); ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::EndChild();
	}

    void DrawImGuiComponentTreeNode(USceneComponent* SceneComp)
    {
        TArray<USceneComponent*> ChildComps;
        SceneComp->GetChildrenComponents(/*IncludeAllDescendants*/false, ChildComps);
        const bool HasChildren = ChildComps.Num() > 0;

        const FString CompLabel = SceneComp->GetName();

        if (HasChildren)
        {
            const bool NodeOpen = ImGui::TreeNode(Ansi(*CompLabel));
            ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*SceneComp->GetClass()->GetName())); ImGui::NextColumn();
			ImGui::Text(SceneComp->IsActive() ? "true" : "false"); ImGui::NextColumn();
			ImGui::Text(SceneComp->IsComponentTickEnabled() ? "true" : "false"); ImGui::NextColumn();
			ImGui::Text(SceneComp->GetIsReplicated() ? "true" : "false"); ImGui::NextColumn();
            if (NodeOpen)
            {
                for (USceneComponent* ChildSceneComp : ChildComps)
                {
                    DrawImGuiComponentTreeNode(ChildSceneComp);
                }

                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::Text("%s", Ansi(*CompLabel)); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*SceneComp->GetClass()->GetName())); ImGui::NextColumn();
			ImGui::Text(SceneComp->IsActive() ? "true" : "false"); ImGui::NextColumn();
			ImGui::Text(SceneComp->IsComponentTickEnabled() ? "true" : "false"); ImGui::NextColumn();
			ImGui::Text(SceneComp->GetIsReplicated() ? "true" : "false"); ImGui::NextColumn();
        }
    }

    void DrawImGuiActorWindow(AActor* Act)
    {
        const TSet<UActorComponent*>& Components = Act->GetComponents();
        const int NumComps = Components.Num();

        if (ImGui::CollapsingHeader("Actor Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginChild(Ansi(*FString::Printf(TEXT("a:%sw:%s##ActorInfo"), *Act->GetName(), *Act->GetWorld()->GetName())), ImVec2(0.0f, 130.0f));
            ImGui::Columns(2);

            ImGui::Text("Actor Name"); ImGui::NextColumn();
            ImGui::Text("%s", Ansi(*Act->GetName())); ImGui::Separator(); ImGui::NextColumn();

            ImGui::Text("Replicates"); ImGui::NextColumn();
            ImGui::Text("%s", Ansi(*GetReplicationString_Actor(Act))); ImGui::NextColumn();

			ImGui::Text("Replicates Movement"); ImGui::NextColumn();
			ImGui::Text(Act->IsReplicatingMovement() ? "true" : "false"); ImGui::NextColumn();
            
            ImGui::Text("World"); ImGui::NextColumn();
            ImGui::Text("%s", Ansi(*Act->GetWorld()->GetName())); ImGui::NextColumn();
            
            ImGui::Text("Actor Tick"); ImGui::NextColumn();
            ImGui::Text(Act->IsActorTickEnabled() ? "true" : "false"); ImGui::NextColumn();

            ImGui::Text("Component Count"); ImGui::NextColumn();
            ImGui::Text("%d", NumComps); ImGui::NextColumn();

            int TickingComponentsCount = 0;
            for (UActorComponent* Comp : Components)
            {
                if (Comp && Comp->IsComponentTickEnabled())
                {
                    ++TickingComponentsCount;
                }
            }

			ImGui::Text("Ticking Component Count"); ImGui::NextColumn();
			ImGui::Text("%d", TickingComponentsCount); ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::EndChild();
        }

		if (ImGui::CollapsingHeader("Class Hierarchy"))
		{
			ImGuiTools::DrawClassHierarchy(Act, AActor::StaticClass());
		}

		if (ImGui::CollapsingHeader("UProperties"))
		{
			DrawUProperties(Act);
		}
        
        if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginChild(Ansi(*FString::Printf(TEXT("a:%sw:%s##Comps"), *Act->GetName(), *Act->GetWorld()->GetName())), ImVec2(0.0f, 250.0f));
            
            ImGui::Columns(5);

            ImGui::Text("Component Name"); ImGui::NextColumn();
			ImGui::Text("Component Class"); ImGui::NextColumn();
            ImGui::Text("Active"); ImGui::NextColumn();
            ImGui::Text("Ticks"); ImGui::NextColumn();
            ImGui::Text("Replicates"); ImGui::NextColumn();


            if (USceneComponent* RootSceneComp = Act->GetRootComponent())
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Scene Components:");
                ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn();
                DrawImGuiComponentTreeNode(RootSceneComp);
            }

            int CompCount = 1;
            bool AnyActorComps = false;
            for (UActorComponent* Comp : Components)
            {
                if (Comp && !Cast<USceneComponent>(Comp))
                {
                    if (!AnyActorComps)
                    {
                        AnyActorComps = true;
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Actor Components:");
                        ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn();
                    }
                    ImGui::Text("%s", Ansi(*FString::Printf(TEXT(" %d - %s "), CompCount, *Comp->GetName())));

					ImGui::NextColumn();
					ImGui::Text("%s", Ansi(*Comp->GetClass()->GetName())); ImGui::NextColumn();
					ImGui::Text(Comp->IsActive() ? "true" : "false"); ImGui::NextColumn();
					ImGui::Text(Comp->IsComponentTickEnabled() ? "true" : "false"); ImGui::NextColumn();
					ImGui::Text(Comp->GetIsReplicated() ? "true" : "false"); ImGui::NextColumn();

                    ++CompCount;
                }
            }

            ImGui::Columns(1);
            ImGui::EndChild();
        }
    }

	void DrawImGuiComponentWindow(UActorComponent* Comp)
	{
		if (ImGui::CollapsingHeader("Component Info", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginChild(Ansi(*FString::Printf(TEXT("a:%sw:%s##CompInfo"), *Comp->GetName(), *Comp->GetWorld()->GetName())), ImVec2(0.0f, 130.0f));
			ImGui::Columns(2);

			ImGui::Text("Component Name"); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*Comp->GetName())); ImGui::Separator(); ImGui::NextColumn();

			ImGui::Text("Owner"); ImGui::NextColumn();
			ImGui::Text("%s", Comp->GetOwner() ? Ansi(*Comp->GetOwner()->GetName()) : "*none*"); ImGui::NextColumn();

			ImGui::Text("World"); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*Comp->GetWorld()->GetName())); ImGui::NextColumn();

			ImGui::Text("Activated"); ImGui::NextColumn();
			ImGui::Text(Comp->IsActive() ? "active" : "not active"); ImGui::NextColumn();

			ImGui::Text("Ticking"); ImGui::NextColumn();
			ImGui::Text(Comp->IsComponentTickEnabled() ? "enabled" : "disabled"); ImGui::NextColumn();

			ImGui::Text("Replicates"); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*GetReplicationString_Component(Comp))); ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::EndChild();
		}

		if (ImGui::CollapsingHeader("Class Hierarchy"))
		{
			ImGuiTools::DrawClassHierarchy(Comp, UActorComponent::StaticClass());
		}

		if (ImGui::CollapsingHeader("UProperties"))
		{
			DrawUProperties(Comp);
		}
	}

	bool CachedClass_DoesAnyDescendantPassFilter(FCachedClassInfo& CachedClassInfo, ImGuiTextFilter& ClassFilter, TArray<FCachedClassInfo>& ParentContainer)
	{
		if (ClassFilter.PassFilter(Ansi(*CachedClassInfo.Class->GetName())))
		{
			// main class passes filter, we can just say we pass the filter
			return true;
		}
		
		// Check for child classes passing the filter
		for (int ChildClassIndex : CachedClassInfo.ChildClassIndicies)
		{
			FCachedClassInfo& ChildCachedInfo = ParentContainer[ChildClassIndex];
			if (CachedClass_DoesAnyDescendantPassFilter(ChildCachedInfo, ClassFilter, ParentContainer))
			{
				return true;
			}
		}

		// nothing passed filter. we fail!
		return false;
	}

	// Draw ImGui for this cached class - Actor version - provide a pointer to an optional parent container to draw Child classes
	void CachedClass_DrawImGui_Actor(FCachedClassInfo& CachedClassInfo, TArray<TWeakObjectPtr<AActor>>& ActorWindows, ImGuiTextFilter& ClassFilter, TArray<FCachedClassInfo>* OptionalParentContainer = nullptr, bool ForceOpen = false, bool ForceOpenChildren = false)
	{
		// Check to see if we should draw ourselves
		if (OptionalParentContainer)
		{
			// Hierarchical view.. draw ourselves if this class or any descendants pass name filter
			if (!CachedClass_DoesAnyDescendantPassFilter(CachedClassInfo, ClassFilter, *OptionalParentContainer))
			{
				return;
			}
		}
		else
		{
			// Non-Hierarchical view.. Draw ourselves if we pass name filter and we have child actors
			if (CachedClassInfo.Objects.Num() == 0)
			{
				return;
			}
			if (!ClassFilter.PassFilter(Ansi(*CachedClassInfo.Class->GetName())))
			{
				return;
			}
		}

		static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap;
		ImGuiTreeNodeFlags node_flags = base_flags;
		if (ForceOpen)
		{
			node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		const bool TreeOpen = ImGui::TreeNodeEx(Ansi(*FString::Printf(TEXT("%s"), *CachedClassInfo.Class->GetName())), node_flags);
		ImGui::NextColumn();
		ImGui::Text("%03d", CachedClassInfo.CachedActorHierarchyCount);
		ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn();
		if (TreeOpen)
		{
			// Sort actors of this class alphabetically if this node is open.
            CachedClassInfo.Objects.Sort([](const TWeakObjectPtr<UObject>& A, const TWeakObjectPtr<UObject>& B) { return A->GetName() < B->GetName(); });

			// Display Actors
			for (int i = 0; i < CachedClassInfo.Objects.Num(); ++i)
			{
				//ImGui::SameLine(16.0f);
				TWeakObjectPtr<UObject>& Actor = CachedClassInfo.Objects[i];
				AActor* ActorPtr = Cast<AActor>(Actor.Get());
				if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Inspect Actor %03d - %s"), i + 1, *Actor->GetName()))))
				{
					ActorWindows.AddUnique(TWeakObjectPtr<AActor>(ActorPtr));
				}
				ImGui::NextColumn(); ImGui::NextColumn();

				ImGui::Text("%s", Ansi(*GetReplicationString_Actor(ActorPtr))); ImGui::NextColumn();
				ImGui::Text(ActorPtr->IsActorTickEnabled() ? "true" : "false"); ImGui::NextColumn();

				int TickingComps = 0;
				const TSet<UActorComponent*>& ActorComps = ActorPtr->GetComponents();
				for (UActorComponent* ActorComp : ActorComps)
				{
					if (ActorComp && ActorComp->IsComponentTickEnabled())
					{
						++TickingComps;
					}
				}
				ImGui::Text("%d", ActorComps.Num()); ImGui::NextColumn();
				ImGui::Text("%d", TickingComps); ImGui::NextColumn();
			}

			// Display Child classes
			if (OptionalParentContainer)
			{
				TArray<FCachedClassInfo>& ParentContainer = *OptionalParentContainer;
				// If we were provided an optional parent container, we can display classes in hierarchy
				for (int ChildClassIndex : CachedClassInfo.ChildClassIndicies)
				{
                    CachedClass_DrawImGui_Actor(ParentContainer[ChildClassIndex], ActorWindows, ClassFilter, OptionalParentContainer, (CachedClassInfo.Objects.Num() == 0) || ForceOpenChildren, ForceOpenChildren);
				}
			}

			ImGui::Separator();
			ImGui::TreePop();
		}
	}

	// Draw ImGui for this cached class - Component version - provide a pointer to an optional parent container to draw Child classes
	void CachedClass_DrawImGui_Component(FCachedClassInfo& CachedClassInfo, TArray<TWeakObjectPtr<UActorComponent>>& ComponentWindows, TArray<FCachedClassInfo>* OptionalParentContainer = nullptr, bool ForceOpen = false, bool ForceOpenChildren = false)
	{
		static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap;
		ImGuiTreeNodeFlags node_flags = base_flags;
		if (ForceOpen)
		{
			node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		const bool TreeOpen = ImGui::TreeNodeEx(Ansi(*FString::Printf(TEXT("%s"), *CachedClassInfo.Class->GetName())), node_flags);
		ImGui::NextColumn();
		ImGui::Text("%03d", CachedClassInfo.CachedActorHierarchyCount);
		ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn();
		if (TreeOpen)
		{
			// Sort objects of this class alphabetically if this node is open.
			CachedClassInfo.Objects.Sort([](const TWeakObjectPtr<UObject>& A, const TWeakObjectPtr<UObject>& B) { return A->GetName() < B->GetName(); });

			// Display Components
			for (int i = 0; i < CachedClassInfo.Objects.Num(); ++i)
			{
				TWeakObjectPtr<UObject>& Comp = CachedClassInfo.Objects[i];
				UActorComponent* CompPtr = Cast<UActorComponent>(Comp.Get());
				if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Inspect Component %03d - %s"), i + 1, *CompPtr->GetName()))))
				{
					ComponentWindows.AddUnique(TWeakObjectPtr<UActorComponent>(CompPtr));
				}
				ImGui::NextColumn(); ImGui::NextColumn();

				
				ImGui::Text("%s", CompPtr->GetOwner() ? Ansi(*CompPtr->GetOwner()->GetName()) : "*none*"); ImGui::NextColumn();
				ImGui::Text(CompPtr->IsActive() ? "active" : "not active"); ImGui::NextColumn();
				ImGui::Text(CompPtr->IsComponentTickEnabled() ? "enabled" : "disabled"); ImGui::NextColumn();
				ImGui::Text("%s", Ansi(*GetReplicationString_Component(CompPtr))); ImGui::NextColumn();
			}

			// Display Child classes
			if (OptionalParentContainer)
			{
				TArray<FCachedClassInfo>& ParentContainer = *OptionalParentContainer;
				// If we were provided an optional parent container, we can display classes in hierarchy
				for (int ChildClassIndex : CachedClassInfo.ChildClassIndicies)
				{
					CachedClass_DrawImGui_Component(ParentContainer[ChildClassIndex], ComponentWindows, OptionalParentContainer, (CachedClassInfo.Objects.Num() == 0) || ForceOpenChildren, ForceOpenChildren);
				}
			}

			ImGui::Separator();
			ImGui::TreePop();
		}
	}


	///////////////////////////////////////
	/////////  Helper structs and enums

    enum class EClassSortType : int
    {
        Alphabetical = 0,
        ActorCount
    };

    // Settings for the entire window
    struct FSettings
    {
        // How often to refresh the actor cache. <= 0.0f means every frame
        float RefreshTimer = -1.0f;
    };

    // Per-World settings
    struct FWorldSettings
    {
        // Should the classes be displayed in a hierarchy or in a flat list
        bool            ClassHierarchy  = true;

        // How to sort the actor classes
        EClassSortType  ClassSortType   = EClassSortType::ActorCount;
    };

	// Will cache actor hierarchy count on this class cache ( and any children as a by product ) and return the result.
	int FCachedClassInfo::CacheActorHierarchyCount(TArray<FCachedClassInfo>& ParentContainer)
	{
		CachedActorHierarchyCount = Objects.Num();
		for (int ChildClassIndex : ChildClassIndicies)
		{
			CachedActorHierarchyCount += ParentContainer[ChildClassIndex].CacheActorHierarchyCount(ParentContainer);
		}
		return CachedActorHierarchyCount;
	}   

    // cached data for a single world
    struct FCachedWorldInfo
    {
        void SortAndBuildHierarchy(TArray<FCachedClassInfo>& CachedClassInfos)
        {
			// Loop through and cache actor counts ( do this before sorting by actor count! ) 
			//  NOTE: we know index 0 is the root class AActor right now as we are pre-sort.
			CachedClassInfos[0].CacheActorHierarchyCount(CachedClassInfos);

			// All classes and actors added. Now sort classes. 
			switch (WorldSettings.ClassSortType)
			{
			    default:
			    case ImGuiActorCompUtils::EClassSortType::Alphabetical:
				    CachedClassInfos.Sort([](const FCachedClassInfo& A, const FCachedClassInfo& B) { return A.Class->GetName() < B.Class->GetName(); });
				    break;

			    case ImGuiActorCompUtils::EClassSortType::ActorCount:
				    CachedClassInfos.Sort([](const FCachedClassInfo& A, const FCachedClassInfo& B) {
					    const int ASize = A.CachedActorHierarchyCount;
					    const int BSize = B.CachedActorHierarchyCount;
					    if (ASize == BSize)
					    {
						    // Sort alphabetically for class with same actor count
						    return A.Class->GetName() < B.Class->GetName();
					    }
					    return ASize > BSize;
					    });
				    break;
			}

			// Classes sorted but our child class indices will now be all messed up, now clear and traverse list and set child class indices with sorted array.
			const int ClassCount = CachedClassInfos.Num();
			for (int i = 0; i < ClassCount; ++i)
			{
				CachedClassInfos[i].ChildClassIndicies.Empty();
			}
			for (int i = 0; i < ClassCount; ++i)
			{
				UClass* SuperClass = CachedClassInfos[i].Class->GetSuperClass();

				// For each class, loop through again and check for our direct parent class, if found add our index as a child class index
				for (FCachedClassInfo& PotentialParentClassInfo : CachedClassInfos)
				{
					if (PotentialParentClassInfo.Class == SuperClass)
					{
						PotentialParentClassInfo.ChildClassIndicies.Add(i);
					}
				}
			}
        }

        void TryCacheActorHierarchy()
        {
            // Clear previous cached actor info
            ActorClassInfos.Empty();

            // Add a stub for root class AActor
            FCachedClassInfo& ActorCachedClassInfo = ActorClassInfos.AddDefaulted_GetRef();
            ActorCachedClassInfo.Class = AActor::StaticClass();

            // Grab actors from the associated world and cache them.
            for (TActorIterator<AActor> It(World.Get()); It; ++It)
            {
                AActor* Actor = *It;
                if (!Actor || !IsValid(Actor))
                {
                    continue;
                }

                UClass* ActorClass = Actor->GetClass();

                // See if we already have this class accounted for, else add a new entry
                FCachedClassInfo* CachedClassInfo = ActorClassInfos.FindByPredicate([ActorClass](const FCachedClassInfo& ClassInfo) { return (ClassInfo.Class.Get() == ActorClass); });
                if (CachedClassInfo)
                {
                    // Found the class info! Just add this actor
                    CachedClassInfo->Objects.Add(Actor);
                }
                else
                {
                    // New class not found, add a cached class info, then look for parent classes upwards until you hit some found class. 
                    FCachedClassInfo& NewClassInfo = ActorClassInfos.AddDefaulted_GetRef();
                    NewClassInfo.Class = ActorClass;
                    NewClassInfo.Objects.Add(Actor);
                    
                    UClass* NewClass = ActorClass;
                    int NewClassIndex = ActorClassInfos.Num() - 1;

                    bool FoundParentClass = false;
                    while (!FoundParentClass)
                    {
                        UClass* NewClassSuper = NewClass->GetSuperClass();

                        FCachedClassInfo* SuperClassInfo = ActorClassInfos.FindByPredicate([NewClassSuper](const FCachedClassInfo& ClassInfo) { return (ClassInfo.Class.Get() == NewClassSuper); });
                        if (SuperClassInfo)
                        {
                            // Found our super class! Add this new class as child 
                            SuperClassInfo->ChildClassIndicies.Add(NewClassIndex);
                            FoundParentClass = true;
                        }
                        else
                        {
                            // We didn't find our super class, so add an empty entry for our immediate super class,
                            //  link our index, and keep looking for our super class' super class.
                            FCachedClassInfo& NewSuperClassInfo = ActorClassInfos.AddDefaulted_GetRef();
                            NewSuperClassInfo.Class = NewClassSuper;
                            NewSuperClassInfo.ChildClassIndicies.Add(NewClassIndex);

                            NewClass = NewClassSuper;
                            NewClassIndex = ActorClassInfos.Num() - 1;
                        }
                    }
                }
               
            }

            SortAndBuildHierarchy(ActorClassInfos);
        }

		void TryCacheComponentHierarchy()
		{
			// Clear previous cached component info
			ComponentClassInfos.Empty();

			// Add a stub for root class AActorComp
			FCachedClassInfo& RootCachedClassInfo = ComponentClassInfos.AddDefaulted_GetRef();
            RootCachedClassInfo.Class = UActorComponent::StaticClass();

			// Grab actors from the associated world and cache them.
			//for (TActorIterator<AActor> It(World.Get()); It; ++It)
            for (TObjectIterator<UActorComponent> It(RF_ArchetypeObject | RF_ClassDefaultObject); It; ++It)
			{
				UActorComponent* ActorComp = *It;
				if (!ActorComp || !IsValid(ActorComp) || ActorComp->GetWorld() != World.Get())
				{
					continue;
				}

				UClass* ActorCompClass = ActorComp->GetClass();

				// See if we already have this class accounted for, else add a new entry
				FCachedClassInfo* CachedClassInfo = ComponentClassInfos.FindByPredicate([ActorCompClass](const FCachedClassInfo& ClassInfo) { return (ClassInfo.Class.Get() == ActorCompClass); });
				if (CachedClassInfo)
				{
					// Found the class info! Just add this actor
					CachedClassInfo->Objects.Add(ActorComp);
				}
				else
				{
					// New class not found, add a cached class info, then look for parent classes upwards until you hit some found class. 
					FCachedClassInfo& NewClassInfo = ComponentClassInfos.AddDefaulted_GetRef();
					NewClassInfo.Class = ActorCompClass;
					NewClassInfo.Objects.Add(ActorComp);

					UClass* NewClass = ActorCompClass;
					int NewClassIndex = ComponentClassInfos.Num() - 1;

					bool FoundParentClass = false;
					while (!FoundParentClass)
					{
						UClass* NewClassSuper = NewClass->GetSuperClass();

						FCachedClassInfo* SuperClassInfo = ComponentClassInfos.FindByPredicate([NewClassSuper](const FCachedClassInfo& ClassInfo) { return (ClassInfo.Class.Get() == NewClassSuper); });
						if (SuperClassInfo)
						{
							// Found our super class! Add this new class as child 
							SuperClassInfo->ChildClassIndicies.Add(NewClassIndex);
							FoundParentClass = true;
						}
						else
						{
							// We didn't find our super class, so add an empty entry for our immediate super class,
							//  link our index, and keep looking for our super class' super class.
							FCachedClassInfo& NewSuperClassInfo = ComponentClassInfos.AddDefaulted_GetRef();
							NewSuperClassInfo.Class = NewClassSuper;
							NewSuperClassInfo.ChildClassIndicies.Add(NewClassIndex);

							NewClass = NewClassSuper;
							NewClassIndex = ComponentClassInfos.Num() - 1;
						}
					}
				}

			}

			SortAndBuildHierarchy(ComponentClassInfos);
		}

        void DrawComponentsImGui(float DeltaTime)
        {
			// TODO: Cache based on timers and such as well. 
			TryCacheComponentHierarchy();

			if (ImGui::BeginTabItem(Ansi(*World->GetDebugDisplayName())))
			{
				ImGui::BeginChild(Ansi(*FString::Printf(TEXT("CompHeader##%s"), *World->GetDebugDisplayName())), ImVec2(0.0f, 80.0f), true);
				ImGui::Text("%s", Ansi(*World->GetDebugDisplayName()));

				ImGui::Separator();

				// layout config
				static const float ActorCountColWidth = 60.0f;

				ImGui::Columns(2);
				ImGui::SameLine((ImGui::GetColumnWidth() * 0.5f) - 50.0f);
				ImGui::Text("Class Info"); ImGui::NextColumn();
				ImGui::SameLine((ImGui::GetColumnWidth() * 0.5f) - 50.0f);
				ImGui::Text("Component Info"); ImGui::NextColumn();
				const float ClassInfoWidth = ImGui::GetColumnWidth(0);
				const float ActorInfoWidth = ImGui::GetColumnWidth(1);
				ImGui::Separator();
				ImGui::Columns(1);


				static const int Columns = 6;

				const float ActorInfoColWidth = ActorInfoWidth * 0.25f;
				float LabelColWidths[Columns] = {
					ClassInfoWidth - ActorCountColWidth,
					ActorCountColWidth,
					ActorInfoColWidth,
					ActorInfoColWidth,
					ActorInfoColWidth,
					ActorInfoColWidth
				};
				static auto SetColumnWidths = [LabelColWidths]() {
					for (int i = 0; i < 6; ++i)
					{
						ImGui::SetColumnWidth(i, LabelColWidths[i]);
					}
				};


				ImGui::Columns(Columns);
				ImGui::Text("Class Name"); ImGui::NextColumn();
				ImGui::Text("Comp\nCount"); ImGui::NextColumn();

				ImGui::Text("Owner"); ImGui::NextColumn();
				ImGui::Text("Component Active"); ImGui::NextColumn();
				ImGui::Text("Component Ticks"); ImGui::NextColumn();
				ImGui::Text("Component Replicates"); ImGui::NextColumn();

				SetColumnWidths();
				ImGui::Columns(1);

				ImGui::EndChild(); // Header


				ImGui::BeginChild(Ansi(*FString::Printf(TEXT("CompContents##%s"), *World->GetDebugDisplayName())), ImVec2(0, 0), true);
				ImGui::Columns(Columns);

				if (WorldSettings.ClassHierarchy)
				{
					// Draw with class hierarchy
					ImGuiActorCompUtils::FCachedClassInfo* ClassInfo = ComponentClassInfos.FindByPredicate([](const FCachedClassInfo& ClassInfo) {
						return (ClassInfo.Class == UActorComponent::StaticClass());
						});

					if (ClassInfo)
					{
						// Draw just the AActor class info and rely on it to draw all it's children.
						CachedClass_DrawImGui_Component(*ClassInfo, CompWindows, &ComponentClassInfos, true);
					}
				}
				else
				{
					// Draw as flat list
					for (ImGuiActorCompUtils::FCachedClassInfo& ClassInfo : ComponentClassInfos)
					{
						CachedClass_DrawImGui_Component(ClassInfo, CompWindows);
					}
				}
				SetColumnWidths();
				ImGui::Columns(1);
				ImGui::EndChild(); // Contents

				ImGui::EndTabItem();
			}
        }

        void DrawActorsImGui(float DeltaTime)
        {
            // TODO: Cache based on timers and such as well. 
            TryCacheActorHierarchy();

			if (ImGui::BeginTabItem(Ansi(*World->GetDebugDisplayName())))
			{

				ImGui::BeginChild(Ansi(*FString::Printf(TEXT("ActorHeader##%s"), *World->GetDebugDisplayName())), ImVec2(0, 110.0f), true);

				ImGui::Columns(2, 0, false);

				static ImGuiTextFilter ActorClassFilter;
				static ImGuiTextFilter ActorNameFilter;
				static bool ActorClassFilterEnabled = true;
				static bool ActorNameFilterEnabled = true;
				ImGui::Text("Class Filter"); ImGui::SameLine();
				ImGui::Checkbox("##ActorClassFilterEnabled", &ActorClassFilterEnabled); ImGui::SameLine();
				ActorClassFilter.Draw();

				ImGui::NextColumn();

				ImGui::Text("View Style:"); ImGui::SameLine();
				static int ViewStyleComboValue = WorldSettings.ClassHierarchy ? 0 : 1;
				ImGui::Combo("##ViewStyleCombo", &ViewStyleComboValue, "Hierarchical\0Flat List");
				WorldSettings.ClassHierarchy = (ViewStyleComboValue == 0);

				ImGui::NextColumn();

				ImGui::Text(" Name Filter"); ImGui::SameLine();
				ImGui::Checkbox("##ActorNameFilterEnabled", &ActorNameFilterEnabled); ImGui::SameLine();
				ActorNameFilter.Draw();

				ImGui::NextColumn();

				ImGui::Text(" Sort Type:"); ImGui::SameLine();
				static int SortTypeComboValue = (int)WorldSettings.ClassSortType;
				ImGui::Combo("##SortTypeCombo", &SortTypeComboValue, "Alphabetical\0Actor Count");
				WorldSettings.ClassSortType = (EClassSortType)SortTypeComboValue;

				ImGui::Separator();
                
				ImGui::Columns(0);

                // layout config
                static const float ActorCountColWidth = 60.0f;

				ImGui::Columns(2);
                ImGui::SameLine((ImGui::GetColumnWidth() * 0.5f) - 50.0f);
				ImGui::Text("Class Info"); ImGui::NextColumn();
                ImGui::SameLine((ImGui::GetColumnWidth() * 0.5f) - 50.0f);
				ImGui::Text("Actor Info"); ImGui::NextColumn();
                const float ClassInfoWidth = ImGui::GetColumnWidth(0);
                const float ActorInfoWidth = ImGui::GetColumnWidth(1);
                ImGui::Separator();
				ImGui::Columns(1);

				static const int Columns = 6;

                const float ActorInfoColWidth = ActorInfoWidth * 0.25f;
                float LabelColWidths[Columns] = { 
                    ClassInfoWidth - ActorCountColWidth,
                    ActorCountColWidth,
                    ActorInfoColWidth,
                    ActorInfoColWidth,
                    ActorInfoColWidth,
                    ActorInfoColWidth
                };
                static auto SetColumnWidths = [LabelColWidths]() {
                    for (int i = 0; i < 6; ++i)
                    {
                        ImGui::SetColumnWidth(i, LabelColWidths[i]);
                    }
                };

				ImGui::Columns(Columns);
				ImGui::Text("Class Name"); ImGui::NextColumn();
                ImGui::Text("Actor\nCount"); ImGui::NextColumn();

				ImGui::Text("Replicates"); ImGui::NextColumn();
				ImGui::Text("ActorTicks"); ImGui::NextColumn();
				ImGui::Text("Component Count"); ImGui::NextColumn();
				ImGui::Text("Ticking Comps"); ImGui::NextColumn();
				
                SetColumnWidths();
				ImGui::Columns(1);

				ImGui::EndChild(); // Header


				ImGui::BeginChild(Ansi(*FString::Printf(TEXT("ActorContents##%s"), *World->GetDebugDisplayName())), ImVec2(0, 0), true);
                ImGui::Columns(Columns);

				if (WorldSettings.ClassHierarchy)
				{
					// Draw with class hierarchy
					ImGuiActorCompUtils::FCachedClassInfo* ClassInfo = ActorClassInfos.FindByPredicate([](const FCachedClassInfo& ClassInfo) {
						return (ClassInfo.Class == AActor::StaticClass());
					});

					if (ClassInfo)
					{
						// Draw just the AActor class info and rely on it to draw all it's children.
                        CachedClass_DrawImGui_Actor(*ClassInfo, ActorWindows, ActorClassFilter, &ActorClassInfos, true, true);
					}
				}
				else
				{
					// Draw as flat list
					for (ImGuiActorCompUtils::FCachedClassInfo& ClassInfo : ActorClassInfos)
					{
                        CachedClass_DrawImGui_Actor(ClassInfo, ActorWindows, ActorClassFilter, nullptr, true);
					}
				}
                SetColumnWidths();
                ImGui::Columns(1);
				ImGui::EndChild(); // Contents

				ImGui::EndTabItem();
			}
        }

        // The World associated with this cache
        TWeakObjectPtr<UWorld>					World;
        // Sorted array of cached class info
        TArray<FCachedClassInfo>				ActorClassInfos;
        // Sorted array of cached components. 
        TArray<FCachedClassInfo>				ComponentClassInfos;

        // Array of weak object pointers to actors that should have their own windows
        TArray<TWeakObjectPtr<AActor>>			ActorWindows;

		TArray<TWeakObjectPtr<UActorComponent>> CompWindows;

        // Will this World display
        bool									Display = true;

        FWorldSettings							WorldSettings;
    };

    // cached data for all worlds. probably only one fo these!
    struct FCachedWorldsInfo
    {
        void TryCacheWorlds()
        {
            // Remove any WorldInfos that may now have an invalid World.
            WorldInfos.RemoveAll([](const FCachedWorldInfo& WorldInfo) {
                return !IsValid(WorldInfo.World.Get());
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
                if (!WorldInfos.ContainsByPredicate([World](const FCachedWorldInfo& WorldInfo) { return (WorldInfo.World.Get() == World); }))
                {
                    FCachedWorldInfo& WorldInfo = WorldInfos.AddDefaulted_GetRef();
                    WorldInfo.World = World;
                }
            }
        }

        TArray<FCachedWorldInfo>   WorldInfos;
    };
    

}   // namespace ImGuiActorCompUtils

// Cached data needed only in this file.
static ImGuiActorCompUtils::FCachedWorldsInfo   CachedWorlds;

FImGuiActorComponentDebugger::FImGuiActorComponentDebugger()
{
	ToolName = TEXT("Actor/Component Debugger");
    WindowFlags = ImGuiWindowFlags_MenuBar;
}

void FImGuiActorComponentDebugger::ImGuiUpdate(float DeltaTime)
{
    static float                                    TimeSinceLastRefresh = 0.0f;
	static ImGuiActorCompUtils::FSettings           Settings;

	CachedWorlds.TryCacheWorlds();

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::SliderFloat("Refresh Rate (<= 0 is every frame)", &Settings.RefreshTimer, -1.0f, 30.0f);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Worlds"))
        {
            static auto ToggleWorlds = [](ENetMode WorldMode, bool Enable, bool TurnOffOtherModes = false) {
                for (ImGuiActorCompUtils::FCachedWorldInfo& WorldInfo : CachedWorlds.WorldInfos)
                {
                    if (WorldInfo.World->GetNetMode() == WorldMode)
                    {
                        WorldInfo.Display = Enable;
                    }
                    else if (TurnOffOtherModes)
                    {
                        WorldInfo.Display = false;
                    }
                }
            };

            static auto DrawWorldNetModeSelector = [](const char* ButtonName, ENetMode WorldMode) {
                // Only draw options for world modes that have worlds in play
                bool HasWorldOfMode = false;
                for (const ImGuiActorCompUtils::FCachedWorldInfo& WorldInfo : CachedWorlds.WorldInfos)
                {
                    if (WorldInfo.World->GetNetMode() == WorldMode)
                    {
                        HasWorldOfMode = true;
                        break;
                    }
                }
                if (HasWorldOfMode)
                {
                    const FString ButtonNameString(ButtonName);
                    ImGui::Text("%s", ButtonName); ImGui::SameLine();
				    if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Enable##%s"), *ButtonNameString))))
				    {
					    ToggleWorlds(WorldMode, /*enable*/ true);
				    }
				    ImGui::SameLine();
                    if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Disable##%s"), *ButtonNameString))))
				    {
					    ToggleWorlds(WorldMode, /*enable*/ false);
				    }
				    ImGui::SameLine();
                    if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Exclusive##%s"), *ButtonNameString))))
				    {
					    ToggleWorlds(WorldMode, /*enable*/ true, /*TurnOffOtherModes*/ true);
				    }
                }
            };

            DrawWorldNetModeSelector("Client        ", ENetMode::NM_Client);
            DrawWorldNetModeSelector("Ded Server    ", ENetMode::NM_DedicatedServer);
            DrawWorldNetModeSelector("Listen Server ", ENetMode::NM_ListenServer);
            DrawWorldNetModeSelector("Standalone    ", ENetMode::NM_Standalone);

			ImGui::BeginChild("WorldSelector", ImVec2(400, 240), true);
			for (ImGuiActorCompUtils::FCachedWorldInfo& WorldInfo : CachedWorlds.WorldInfos)
			{
				ImGui::Checkbox(Ansi(*WorldInfo.World->GetDebugDisplayName()), &WorldInfo.Display);
			}
			ImGui::EndChild();  // World Selector
            ImGui::EndMenu();
        }

        if (Settings.RefreshTimer <= 0.0f)
        {
            ImGui::SameLine(ImGui::GetWindowWidth() - 180.0f);
            ImGui::Text("Refresh Every Frame");
        }
        else
        {
            ImGui::SameLine(ImGui::GetWindowWidth() - 250.0f);
            ImGui::ProgressBar((TimeSinceLastRefresh / Settings.RefreshTimer), ImVec2(230.0f, 0.0f), Ansi(*FString::Printf(TEXT("Refresh Timer %.01f/%.01f"), TimeSinceLastRefresh, Settings.RefreshTimer)));
        }

        ImGui::EndMenuBar();
    }
	static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("ActorComponents", tab_bar_flags))
    {
		if (ImGui::BeginTabItem("Actors"))
		{
			if (ImGui::BeginTabBar("ActorWorldTabs", tab_bar_flags))
			{
				for (ImGuiActorCompUtils::FCachedWorldInfo& WorldInfo : CachedWorlds.WorldInfos)
				{
					if (WorldInfo.Display)
					{
						WorldInfo.DrawActorsImGui(DeltaTime);
					}
				}

				ImGui::EndTabBar(); // WorldTabs
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Components"))
		{
			if (ImGui::BeginTabBar("CompWorldTabs", tab_bar_flags))
			{
				for (ImGuiActorCompUtils::FCachedWorldInfo& WorldInfo : CachedWorlds.WorldInfos)
				{
					if (WorldInfo.Display)
					{
                        WorldInfo.DrawComponentsImGui(DeltaTime);
					}
				}

				ImGui::EndTabBar(); // WorldTabs
			}
			ImGui::EndTabItem();
		}

        ImGui::EndTabBar();
    }
}

void FImGuiActorComponentDebugger::UpdateTool(float DeltaTime)
{
    FImGuiToolWindow::UpdateTool(DeltaTime);

    for (ImGuiActorCompUtils::FCachedWorldInfo& WorldInfo : CachedWorlds.WorldInfos)
    {
        if (WorldInfo.Display)
        {
            // Iterate backwards through the actor windows in case one closes itself.
            const int ActorWindowsCount = WorldInfo.ActorWindows.Num();
            for (int i = ActorWindowsCount - 1; i >= 0; --i)
            {
                TWeakObjectPtr<AActor>& ActorForWindow = WorldInfo.ActorWindows[i];
                if (ActorForWindow.IsValid())
                {
                    bool WindowOpen = true;
                    if (ImGui::Begin(Ansi(*FString::Printf(TEXT("actor %s - world %s"), *ActorForWindow->GetName(), *ActorForWindow->GetWorld()->GetName())), &WindowOpen))
                    {
                        ImGuiActorCompUtils::DrawImGuiActorWindow(ActorForWindow.Get());
                        ImGui::End();
                    }

                    if (!WindowOpen)
                    {
                        WorldInfo.ActorWindows.RemoveAt(i);
                    }
                }
            }

			// Iterate backwards through the actor windows in case one closes itself.
			const int CompWindowsCount = WorldInfo.CompWindows.Num();
			for (int i = CompWindowsCount - 1; i >= 0; --i)
			{
				TWeakObjectPtr<UActorComponent>& CompForWindow = WorldInfo.CompWindows[i];
				if (CompForWindow.IsValid())
				{
					bool WindowOpen = true;
					if (ImGui::Begin(Ansi(*FString::Printf(TEXT("comp %s - world %s"), *CompForWindow->GetName(), *CompForWindow->GetWorld()->GetName())), &WindowOpen))
					{
						ImGuiActorCompUtils::DrawImGuiComponentWindow(CompForWindow.Get());
						ImGui::End();
					}

					if (!WindowOpen)
					{
						WorldInfo.CompWindows.RemoveAt(i);
					}
				}
			}
        }
    }
}
