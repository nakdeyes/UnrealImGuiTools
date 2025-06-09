// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiMemoryDebugger.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 4
#include "Misc/StreamingTextureLevelContext.h"	// StreamingTexture copy for UE 4.XX where this is not exported in engine. (fixed in UE5)
#endif // #if ENGINE_MAJOR_VERSION == 4

#include "Utils/ImGuiUtils.h"

#include <Components/PrimitiveComponent.h>
#include <Engine/Texture2D.h>
#include <Engine/TextureCube.h>
#include <Engine/TextureLODSettings.h>
#include <Engine/TextureStreamingTypes.h>
#include <RenderUtils.h>
#include <UObject/UObjectIterator.h>

#include <imgui.h>

// Log Category
DEFINE_LOG_CATEGORY_STATIC(LogImGuiDebugMem, Warning, All);

namespace MemDebugUtils
{
	float IntBytesToFltMB(uint64 bytes)
	{
		return (float) bytes / 1000000.0f;
	}

	struct FSortedTexture
	{
		int32 MaxAllowedSizeX;	  // This is the disk size when cooked.
		int32 MaxAllowedSizeY;
		EPixelFormat Format;
		int32 CurSizeX;
		int32 CurSizeY;
		int32 LODBias;
		int32 MaxAllowedSize;
		int32 CurrentSize;
		FString Name;
		int32 LODGroup;
		bool bIsStreaming;
		int32 UsageCount;

		/** Constructor, initializing every member variable with passed in values. */
		FSortedTexture(int32 InMaxAllowedSizeX,
					   int32 InMaxAllowedSizeY,
					   EPixelFormat InFormat,
					   int32 InCurSizeX,
					   int32 InCurSizeY,
					   int32 InLODBias,
					   int32 InMaxAllowedSize,
					   int32 InCurrentSize,
					   const FString& InName,
					   int32 InLODGroup,
					   bool bInIsStreaming,
					   int32 InUsageCount)
			: MaxAllowedSizeX(InMaxAllowedSizeX)
			, MaxAllowedSizeY(InMaxAllowedSizeY)
			, Format(InFormat)
			, CurSizeX(InCurSizeX)
			, CurSizeY(InCurSizeY)
			, LODBias(InLODBias)
			, MaxAllowedSize(InMaxAllowedSize)
			, CurrentSize(InCurrentSize)
			, Name(InName)
			, LODGroup(InLODGroup)
			, bIsStreaming(bInIsStreaming)
			, UsageCount(InUsageCount)
		{}
	};

	struct FCompareFSortedTexture
	{
		bool bAlphaSort;
		FCompareFSortedTexture(bool InAlphaSort)
			: bAlphaSort(InAlphaSort)
		{}
		FORCEINLINE bool operator()(const FSortedTexture& A, const FSortedTexture& B) const
		{
			if (bAlphaSort || A.CurrentSize == B.CurrentSize)
			{
				return A.Name < B.Name;
			}

			return B.CurrentSize < A.CurrentSize;
		}
	};

	struct FMemInfo
	{
		float TotalMemoryMB = 0.0f;
		float UnknownMemoryMB = 0.0f;
		float DedSysMemoryMB = 0.0f;
		float DedVidMemoryMB = 0.0f;
		float SharedSysMemoryMB = 0.0f;
		float SharedVidMemoryMB = 0.0f;
	};

	struct FCachedClassInfo
	{
		// Class Info
		TWeakObjectPtr<UClass> Class;
		bool bAbstract = false;
		TArray<int> ChildIndicies;

		// Current MemReport Info
		FMemInfo MemInfo;
		int Instances = 0;
	};

	namespace EColumnTypes
	{
		enum Type
		{
			Instances = 0,
			IsDefaultObject,
			Outer,
			TotalMem,
			UnknownMem,
			DedSysMem,
			DedVidMem,
			SharedSysMem,
			SharedVidMem,

			COUNT
		};

		static bool DefaultVisibility[Type::COUNT] =
		{
			/*Instances*/		true,
			/*IsDefaultObject*/ true,
			/*Outer*/           true,
			/*TotalMem*/        true,
			/*UnknownMem*/      true,
			/*DedSysMem*/       true,
			/*DedVidMem*/       true,
			/*SharedSysMem*/    false,
			/*SharedVidMem*/    false,
		};
	}	// namespace EColumnTypes

	namespace EMemSortType
	{
		enum Type
		{
			None,
			Alpha,
			Instances,
			TotalMem,
			UnknownMem,
			DedSysMem,
			DedVidMem,
			SharedSysMem,
			SharedVidMem,
		};
	}	// namespace EMemSortType

	struct FCachedClassTree
	{
		void TryCacheEntries(bool ForceRecache = false)
		{
			if (Classes.Num() > 0)
			{
				if (!ForceRecache)
				{
					return;
				}
				
				Classes.Empty();
			}

			for (TObjectIterator<UClass> It; It; ++It)
			{
				FCachedClassInfo& ClassInfo = Classes.AddDefaulted_GetRef();
				ClassInfo.Class = *It;
				ClassInfo.bAbstract = It->HasAnyClassFlags(CLASS_Abstract);
			}

			const int NumClasses = Classes.Num();
			for (int i = 0; i < NumClasses; ++i)
			{
				FCachedClassInfo& ClassInfo = Classes[i];
				if (ClassInfo.Class == UObject::StaticClass())
				{
					RootIndex = i;
				}

				for (int j = 0; j < NumClasses; ++j)
				{
					FCachedClassInfo& PotentialChild = Classes[j];
					if (PotentialChild.Class.IsValid() && PotentialChild.Class->GetSuperClass() == ClassInfo.Class)
					{
						ClassInfo.ChildIndicies.Add(j);
					}
				}
			}
		}

		void UpdateMemoryStats(bool IncludeCDO, EResourceSizeMode::Type ResourceSizeMode = EResourceSizeMode::Exclusive)
		{
			// Caching Class Entires is quick compared to what we are about to do.. refresh latest class info.
			TryCacheEntries(/*ForceRecache*/true);

			for (FCachedClassInfo& ClassInfo : Classes)
			{
				ClassInfo.MemInfo = {};
				ClassInfo.Instances = 0;
			}

			for (FThreadSafeObjectIterator It; It; ++It)
			{
				if ((!IncludeCDO && It->IsTemplate(RF_ClassDefaultObject)) || !It->GetPackage()->GetHasBeenEndLoaded())
				{
					continue;
				}
				FResourceSizeEx TrueResourceSize = FResourceSizeEx(ResourceSizeMode);
				It->GetResourceSizeEx(TrueResourceSize);
				for (FCachedClassInfo& ClassInfo : Classes)
				{
					if (It->GetClass()->IsChildOf(ClassInfo.Class.Get()))
					{
						ClassInfo.MemInfo.TotalMemoryMB += (float) TrueResourceSize.GetTotalMemoryBytes();
						ClassInfo.MemInfo.UnknownMemoryMB += (float)TrueResourceSize.GetUnknownMemoryBytes();
						ClassInfo.MemInfo.DedSysMemoryMB += (float)TrueResourceSize.GetDedicatedSystemMemoryBytes();
						ClassInfo.MemInfo.DedVidMemoryMB += (float)TrueResourceSize.GetDedicatedVideoMemoryBytes();
#if ENGINE_MAJOR_VERSION == 4
						ClassInfo.MemInfo.SharedSysMemoryMB += (float)TrueResourceSize.GetSharedSystemMemoryBytes();
						ClassInfo.MemInfo.SharedVidMemoryMB += (float)TrueResourceSize.GetSharedVideoMemoryBytes();
#endif // #if ENGINE_MAJOR_VERSION == 4
						
						++ClassInfo.Instances;
					}
				}

			}

			// convert B to MB
			for (FCachedClassInfo& ClassInfo : Classes)
			{
				ClassInfo.MemInfo.TotalMemoryMB = ClassInfo.MemInfo.TotalMemoryMB /1024.0f/1024.0f;
				ClassInfo.MemInfo.UnknownMemoryMB = ClassInfo.MemInfo.UnknownMemoryMB / 1024.0f / 1024.0f;
				ClassInfo.MemInfo.DedSysMemoryMB = ClassInfo.MemInfo.DedSysMemoryMB / 1024.0f / 1024.0f;
				ClassInfo.MemInfo.DedVidMemoryMB = ClassInfo.MemInfo.DedVidMemoryMB / 1024.0f / 1024.0f;
				ClassInfo.MemInfo.SharedSysMemoryMB = ClassInfo.MemInfo.SharedSysMemoryMB / 1024.0f / 1024.0f;
				ClassInfo.MemInfo.SharedVidMemoryMB = ClassInfo.MemInfo.SharedVidMemoryMB / 1024.0f / 1024.0f;
			}
		}

		void SortBy(EMemSortType::Type SortType)
		{
			switch(SortType)
			{
				case EMemSortType::Alpha:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return GetNameSafe(Classes[LHS].Class.Get()) < GetNameSafe(Classes[RHS].Class.Get()); });
					}
					break;

				case EMemSortType::Instances:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return Classes[LHS].Instances > Classes[RHS].Instances; });
					}
					break;

				case EMemSortType::TotalMem:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return Classes[LHS].MemInfo.TotalMemoryMB > Classes[RHS].MemInfo.TotalMemoryMB; });
					}
					break;

				case EMemSortType::UnknownMem:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return Classes[LHS].MemInfo.UnknownMemoryMB > Classes[RHS].MemInfo.UnknownMemoryMB; });
					}
					break;

				case EMemSortType::DedSysMem:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return Classes[LHS].MemInfo.DedSysMemoryMB > Classes[RHS].MemInfo.DedSysMemoryMB; });
					}
					break;

				case EMemSortType::DedVidMem:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return Classes[LHS].MemInfo.DedVidMemoryMB > Classes[RHS].MemInfo.DedVidMemoryMB; });
					}
					break;

				case EMemSortType::SharedSysMem:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return Classes[LHS].MemInfo.SharedSysMemoryMB > Classes[RHS].MemInfo.SharedSysMemoryMB; });
					}
					break;

				case EMemSortType::SharedVidMem:
					for (FCachedClassInfo& ClassInfo : Classes)
					{
						ClassInfo.ChildIndicies.Sort([this](int LHS, int RHS) { return Classes[LHS].MemInfo.SharedVidMemoryMB > Classes[RHS].MemInfo.SharedVidMemoryMB; });
					}
					break;

				default:
					UE_LOG(LogImGuiDebugMem, Error, TEXT("FCachedClassTree::SortBy() - sorting with unimplemented type. not doing anything!"));
					break;
			}
		}

		int RootIndex;
		TArray<FCachedClassInfo> Classes;
	};

	// Struct for each instance in the list of all instances per-class
	struct FInstanceInspectorInstanceInfo
	{
		TWeakObjectPtr<UObject> InstanceWkPtr = nullptr;
		FMemInfo MemInfo;
	};

	// holds referencer info in a memory safe way
	struct FReferencerInfo
	{
		TWeakObjectPtr<UObject> ReferencerWkPtr = nullptr;
		TArray<FString> ReferencingProperties;
		int TotalReferences = 0;
	};

	// Struct for a single inspected instance
	struct FInspectedInstanceInfo
	{
		TWeakObjectPtr<UObject> InspectedWkPtr = nullptr;
		TArray<FReferencerInfo> CachedInternalReferencers;
		TArray<FReferencerInfo> CachedExternalReferencers;

		void UpdateInspectedInstanceReferencers()
		{
			// This caching exists because FReferencerInformation holds raw UObject pointers and UObject::RetrieveReferencers() is slllooooww
			//  so just run it on demand and cache the results in 
			CachedInternalReferencers.Empty();
			CachedExternalReferencers.Empty();

			UObject* InspectedInstanceRawPtr = InspectedWkPtr.Get();
			if (IsValid(InspectedInstanceRawPtr))
			{
				TArray<FReferencerInformation> ReferencerInfos_Internal;
				TArray<FReferencerInformation> ReferencerInfos_External;
				InspectedInstanceRawPtr->RetrieveReferencers(&ReferencerInfos_Internal, &ReferencerInfos_External);

				for (const FReferencerInformation& ReferencerInternal : ReferencerInfos_Internal)
				{
					FReferencerInfo& NewReferencerInfo = CachedInternalReferencers.AddDefaulted_GetRef();
					
					NewReferencerInfo.ReferencerWkPtr = ReferencerInternal.Referencer;
					NewReferencerInfo.TotalReferences = ReferencerInternal.TotalReferences;

					for (const FProperty* ReferencingProperty : ReferencerInternal.ReferencingProperties)
					{
						NewReferencerInfo.ReferencingProperties.Add(ReferencingProperty->GetNameCPP());
					}
				}
			}
		}

		void SetInpectedInstance(const TWeakObjectPtr<UObject>& NewInpectedInstance)
		{
			if (NewInpectedInstance.IsValid())
			{
				InspectedWkPtr = NewInpectedInstance;
				UpdateInspectedInstanceReferencers();
			}
		}
	};

	struct FInstanceInspectorInfo
	{
		TWeakObjectPtr<UClass> Class = nullptr;
		FMemInfo MemInfo;
		int Instances = 0;
		EMemSortType::Type SortType = EMemSortType::TotalMem;
		ImGuiTools::Utils::FShowCols ShowCols = ImGuiTools::Utils::FShowCols(EColumnTypes::COUNT, &EColumnTypes::DefaultVisibility[0]);
		FInspectedInstanceInfo InspectedInstance;
		TArray<FInstanceInspectorInstanceInfo> InstanceInfos;
		ImGuiTextFilter NameFilter;
		bool bAutoRefresh = false;	// option to auto refresh the instance view
		float AutoRefreshTime = 4.0f; // interval to auto refresh the instance view
		float AutoRefreshTimer = 0.0f;	// current timer from last time auto refresh occurred.

		void UpdateObjectMemoryInfo(bool IncludeCDO, EResourceSizeMode::Type ResourceSizeMode = EResourceSizeMode::Exclusive)
		{
			MemInfo = {};
			Instances = 0;
			InstanceInfos.Empty();

			for (FThreadSafeObjectIterator It; It; ++It)
			{
				if (!It->GetClass()->IsChildOf(Class.Get()) || (!IncludeCDO && It->IsTemplate(RF_ClassDefaultObject)))
				{
					continue;
				}

				FResourceSizeEx TrueResourceSize = FResourceSizeEx(ResourceSizeMode);
				It->GetResourceSizeEx(TrueResourceSize);
				const float InstTotalMemMB = (float)TrueResourceSize.GetTotalMemoryBytes()/1024.0f/1024.0f;
				const float InstUnknownMemMB = (float)TrueResourceSize.GetUnknownMemoryBytes() / 1024.0f / 1024.0f;
				const float InstDedSysMemMB = (float)TrueResourceSize.GetDedicatedSystemMemoryBytes() / 1024.0f / 1024.0f;
				const float InstDedVidMemMB = (float)TrueResourceSize.GetDedicatedVideoMemoryBytes() / 1024.0f / 1024.0f;
#if ENGINE_MAJOR_VERSION == 4
				const float InstSharedSysMemMB = (float)TrueResourceSize.GetSharedSystemMemoryBytes() / 1024.0f / 1024.0f;
				const float InstSharedVidMemMB = (float)TrueResourceSize.GetSharedVideoMemoryBytes() / 1024.0f / 1024.0f;
#endif // #if ENGINE_MAJOR_VERSION == 4
				MemInfo.TotalMemoryMB += InstTotalMemMB;
				MemInfo.UnknownMemoryMB += InstUnknownMemMB;
				MemInfo.DedSysMemoryMB += InstDedSysMemMB;
				MemInfo.DedVidMemoryMB += InstDedVidMemMB;
#if ENGINE_MAJOR_VERSION == 4
				MemInfo.SharedSysMemoryMB += InstSharedSysMemMB;
				MemInfo.SharedVidMemoryMB += InstSharedVidMemMB;
#endif // #if ENGINE_MAJOR_VERSION == 4
				++Instances;

				FInstanceInspectorInstanceInfo& InstInfo = InstanceInfos.AddZeroed_GetRef();
				InstInfo.InstanceWkPtr = *It;
				InstInfo.MemInfo.TotalMemoryMB = InstTotalMemMB;
				InstInfo.MemInfo.UnknownMemoryMB = InstUnknownMemMB;
				InstInfo.MemInfo.DedSysMemoryMB = InstDedSysMemMB;
				InstInfo.MemInfo.DedVidMemoryMB = InstDedVidMemMB;
#if ENGINE_MAJOR_VERSION == 4
				InstInfo.MemInfo.SharedSysMemoryMB = InstSharedSysMemMB;
				InstInfo.MemInfo.SharedVidMemoryMB = InstSharedVidMemMB;
#endif // #if ENGINE_MAJOR_VERSION == 4
			}

			SortBy(SortType);
		}

		void SortBy(EMemSortType::Type InSortType)
		{
			SortType = InSortType;
			switch (SortType)
			{
				case EMemSortType::Alpha:
					InstanceInfos.Sort([this](const FInstanceInspectorInstanceInfo& LHS, const FInstanceInspectorInstanceInfo& RHS) { return LHS.InstanceWkPtr->GetName() < RHS.InstanceWkPtr->GetName(); });
					break;

				case EMemSortType::TotalMem:
					InstanceInfos.Sort([this](const FInstanceInspectorInstanceInfo& LHS, const FInstanceInspectorInstanceInfo& RHS) { return LHS.MemInfo.TotalMemoryMB > RHS.MemInfo.TotalMemoryMB; });
					break;

				case EMemSortType::UnknownMem:
					InstanceInfos.Sort([this](const FInstanceInspectorInstanceInfo& LHS, const FInstanceInspectorInstanceInfo& RHS) { return LHS.MemInfo.UnknownMemoryMB > RHS.MemInfo.UnknownMemoryMB; });
					break;

				case EMemSortType::DedSysMem:
					InstanceInfos.Sort([this](const FInstanceInspectorInstanceInfo& LHS, const FInstanceInspectorInstanceInfo& RHS) { return LHS.MemInfo.DedSysMemoryMB > RHS.MemInfo.DedSysMemoryMB; });
					break;

				case EMemSortType::DedVidMem:
					InstanceInfos.Sort([this](const FInstanceInspectorInstanceInfo& LHS, const FInstanceInspectorInstanceInfo& RHS) { return LHS.MemInfo.DedVidMemoryMB > RHS.MemInfo.DedVidMemoryMB; });
					break;

				case EMemSortType::SharedSysMem:
					InstanceInfos.Sort([this](const FInstanceInspectorInstanceInfo& LHS, const FInstanceInspectorInstanceInfo& RHS) { return LHS.MemInfo.SharedSysMemoryMB > RHS.MemInfo.SharedSysMemoryMB; });
					break;

				case EMemSortType::SharedVidMem:
					InstanceInfos.Sort([this](const FInstanceInspectorInstanceInfo& LHS, const FInstanceInspectorInstanceInfo& RHS) { return LHS.MemInfo.SharedVidMemoryMB > RHS.MemInfo.SharedVidMemoryMB; });
					break;

				default:
				case EMemSortType::Instances:
					UE_LOG(LogImGuiDebugMem, Error, TEXT("FInstanceInspectorInfo::SortBy() - sorting with unimplemented type. not doing anything!"));
					break;
			}
		}
	};

	TArray<FInstanceInspectorInfo> InstanceInspectors;
	TWeakObjectPtr<UClass> PopupClass;
	bool DrawClassChildTreeIndex(FCachedClassTree& ClassTree, int Index, bool FilterZeroInstances, ImGuiTextFilter& ClassNameFilter, ImGuiTools::Utils::FShowCols& ShowCols)
	{
		FCachedClassInfo& CachedClassInfo = ClassTree.Classes[Index];
		if (CachedClassInfo.Class.IsStale() || !CachedClassInfo.Class.IsValid())
		{
			return false; // return false when data is bad ( or child data is bad )
		}

		if (FilterZeroInstances && (CachedClassInfo.Instances == 0))
		{
			return true;
		}

		if (ClassNameFilter.IsActive())
		{
			if (!ClassNameFilter.PassFilter(TCHAR_TO_ANSI(*CachedClassInfo.Class->GetName())))
			{
				for (int ChildIndex : CachedClassInfo.ChildIndicies)
				{
					if (!DrawClassChildTreeIndex(ClassTree, ChildIndex, FilterZeroInstances, ClassNameFilter, ShowCols))
					{
						return false;
					}
				}
				return true;
			}
		}

		static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;
		ImGuiTreeNodeFlags node_flags = base_flags;
		if (CachedClassInfo.ChildIndicies.Num() == 0)
		{
			node_flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (ClassNameFilter.IsActive() || (Index == ClassTree.RootIndex))
		{
			node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		const bool TreeOpen = ImGui::TreeNodeEx((void*)CachedClassInfo.Class.Get(), node_flags, "%s%s", Ansi(*CachedClassInfo.Class->GetName()), CachedClassInfo.bAbstract ? "(abstract)" : "");
		ImGui::SameLine(ImGui::GetWindowWidth() - ((ShowCols.GetCachedShowColCount() + 1) * 110.0f) + 44.0f);

		if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Inspect##%.54ls"), *CachedClassInfo.Class->GetName()))))
		{
			FInstanceInspectorInfo& InstInfo = InstanceInspectors.AddDefaulted_GetRef();
			InstInfo.Class = CachedClassInfo.Class;
		}

		ImGui::NextColumn();
		if (ShowCols.GetShowCol(EColumnTypes::Instances)) { ImGui::Text("%d", CachedClassInfo.Instances);ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(EColumnTypes::TotalMem)) { ImGui::Text("%.04f MB", CachedClassInfo.MemInfo.TotalMemoryMB); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(EColumnTypes::UnknownMem)) { ImGui::Text("%.04f MB", CachedClassInfo.MemInfo.UnknownMemoryMB); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(EColumnTypes::DedSysMem)) { ImGui::Text("%.04f MB", CachedClassInfo.MemInfo.DedSysMemoryMB); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(EColumnTypes::DedVidMem)) { ImGui::Text("%.04f MB", CachedClassInfo.MemInfo.DedVidMemoryMB); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(EColumnTypes::SharedSysMem)) { ImGui::Text("%.04f MB", CachedClassInfo.MemInfo.SharedSysMemoryMB); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(EColumnTypes::SharedVidMem)) { ImGui::Text("%.04f MB", CachedClassInfo.MemInfo.SharedVidMemoryMB); ImGui::NextColumn(); }
		ImGui::Separator();

		if (TreeOpen)
		{
			for (int ChildIndex : CachedClassInfo.ChildIndicies)
			{
				if (!DrawClassChildTreeIndex(ClassTree, ChildIndex, FilterZeroInstances, ClassNameFilter, ShowCols))
				{
					return false;
				}
			}

			ImGui::TreePop();
		}

		return true;
	}

	void DrawHoveredItemInstanceTooltip(UObject* InstanceObject)
	{
		ImGui::BeginTooltip();
		if (IsValid(InstanceObject))
		{
			ImGui::Text("        Object: %s", Ansi(*GetNameSafe(InstanceObject)));
			ImGui::Text("         Outer: %s", Ansi(*GetNameSafe(InstanceObject->GetOuter())));
			ImGui::Text("default SubObj: %d", InstanceObject->IsDefaultSubobject());
		}
		else
		{
			ImGui::Text(" Invalid / Stale Object!");
		}

		ImGui::EndTooltip();
	}

	void DrawObjectInspectorPopup(FInstanceInspectorInfo& InstInspInfo, float DeltaTime)
	{
		static EResourceSizeMode::Type ResourceSizeMode = EResourceSizeMode::EstimatedTotal;
		ImGui::Text("%s", Ansi(*InstInspInfo.Class->GetName()));
		ImGui::SameLine();
		if (ImGui::SmallButton("Update Instance Memory Data"))
		{
			InstInspInfo.UpdateObjectMemoryInfo(false, ResourceSizeMode);
		}
		
		ImGui::SameLine(0.0f, 50.0f);

		static int ResourceSizeModeComboValue = static_cast<int>(ResourceSizeMode);
		ImGui::BeginChild("ResourceSizeModeCombo", ImVec2(230.0f, 18.0f));
		ImGui::Combo("##ResourceSizeModeCombo", &ResourceSizeModeComboValue, "Exclusive\0Estimated Total");
		ResourceSizeMode = static_cast<EResourceSizeMode::Type>(ResourceSizeModeComboValue);
		ImGui::EndChild();

		ImGui::Checkbox("Auto-Update", &InstInspInfo.bAutoRefresh);
		if (InstInspInfo.bAutoRefresh)
		{
			ImGui::SameLine(); ImGui::DragFloat("Interval", &InstInspInfo.AutoRefreshTime, 0.01f, 0.01f, 10.0f);
			ImGui::SameLine(); ImGui::ProgressBar(FMath::Clamp<float>(InstInspInfo.AutoRefreshTimer / InstInspInfo.AutoRefreshTime, 0.0f, 1.0f));

			InstInspInfo.AutoRefreshTimer -= DeltaTime;

			if (InstInspInfo.AutoRefreshTimer <= 0.0f)
			{
				InstInspInfo.AutoRefreshTimer = InstInspInfo.AutoRefreshTime;
				InstInspInfo.UpdateObjectMemoryInfo(false, ResourceSizeMode);
			}
		}
		ImGui::Separator();
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 330.0f);
		ImGui::Text("Include/Sort:");
		ImGui::BeginChild("IncSort", ImVec2(0.0f, 90.0f));
		ImGui::Columns(2);
		static const float RadWidth = 54.0f;
		ImGui::SameLine(27.0f);
		if (ImGui::RadioButton("##Alpha", InstInspInfo.SortType == MemDebugUtils::EMemSortType::Alpha)) { InstInspInfo.SortBy(MemDebugUtils::EMemSortType::Alpha); } ImGui::SameLine();
		ImGui::Text("Alpha"); ImGui::NextColumn();

		ImGui::Checkbox("##IsDefaultObject", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::IsDefaultObject)); ImGui::SameLine();
		ImGui::Text("IsDefaultObject"); 
		ImGui::Checkbox("##Outer", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::Outer)); ImGui::SameLine();
		ImGui::Text("Outer"); ImGui::NextColumn();

		ImGui::Checkbox("##TotalMemCh", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::TotalMem)); ImGui::SameLine();
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::TotalMem))
		{ if (ImGui::RadioButton("##TotalMem", InstInspInfo.SortType == MemDebugUtils::EMemSortType::TotalMem)) { InstInspInfo.SortBy(MemDebugUtils::EMemSortType::TotalMem); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("TotalMem"); ImGui::NextColumn();

		ImGui::Checkbox("##UnknownMemCh", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::UnknownMem)); ImGui::SameLine();
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::UnknownMem))
		{ if (ImGui::RadioButton("##UnknownMem", InstInspInfo.SortType == MemDebugUtils::EMemSortType::UnknownMem)) { InstInspInfo.SortBy(MemDebugUtils::EMemSortType::UnknownMem); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("UnknownMem"); ImGui::NextColumn();

		ImGui::Checkbox("##DedSysMemCh", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedSysMem)); ImGui::SameLine();
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedSysMem))
		{ if (ImGui::RadioButton("##DedSysMem", InstInspInfo.SortType == MemDebugUtils::EMemSortType::DedSysMem)) { InstInspInfo.SortBy(MemDebugUtils::EMemSortType::DedSysMem); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("DedSysMem"); ImGui::NextColumn();

		ImGui::Checkbox("##DedVidMemCh", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedVidMem)); ImGui::SameLine();
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedVidMem))
		{ if (ImGui::RadioButton("##DedVidMem", InstInspInfo.SortType == MemDebugUtils::EMemSortType::DedVidMem)) { InstInspInfo.SortBy(MemDebugUtils::EMemSortType::DedVidMem); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("DedVidMem"); ImGui::NextColumn();

		ImGui::Checkbox("##SharedSysMemCh", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedSysMem)); ImGui::SameLine();
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedSysMem))
		{ if (ImGui::RadioButton("##SharedSysMem", InstInspInfo.SortType == MemDebugUtils::EMemSortType::SharedSysMem)) { InstInspInfo.SortBy(MemDebugUtils::EMemSortType::SharedSysMem); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("SharedSysMem"); ImGui::NextColumn();

		ImGui::Checkbox("##SharedVidMemCh", &InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedVidMem)); ImGui::SameLine();
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedVidMem))
		{ if (ImGui::RadioButton("##SharedVidMem", InstInspInfo.SortType == MemDebugUtils::EMemSortType::SharedVidMem)) { InstInspInfo.SortBy(MemDebugUtils::EMemSortType::SharedVidMem); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("SharedVidMem"); ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::EndChild(); // "IncSort"
		ImGui::SetColumnWidth(1, 330.0f);


		ImGui::NextColumn();
		InstInspInfo.NameFilter.Draw("Name Filter", ImGui::GetColumnWidth() - 100.0f);
		ImGui::Columns(1);
		ImGui::BeginChild("InstLabel", ImVec2(0, 50.0f), true);

		ImGui::Text("Total Instances: %d    Total Size: % .04fMB", InstInspInfo.Instances, InstInspInfo.MemInfo.TotalMemoryMB);
		ImGui::Separator();

		static constexpr int ColumnCount = 9;
		InstInspInfo.ShowCols.GetShowCol(EColumnTypes::Instances) = false; // gross: Instances is disabled for this view so ensure it is off so CacheColCount() is contextually correct
		const int VisibleColCount = InstInspInfo.ShowCols.CacheShowColCount() + 1;
		ImGui::Columns(VisibleColCount, "ObjInstCol");
		static float ColumnWidths[ColumnCount];
		static constexpr float InfoColWidth = 110.0f;
		ColumnWidths[0] = ImGui::GetWindowWidth() - (InfoColWidth * (VisibleColCount - 1)); // Object col
		if (VisibleColCount > 1)
		{
			for (int i = 0; i < VisibleColCount; ++i)
			{
				if (i > 0)
				{
					ColumnWidths[i] = InfoColWidth;
				}
				ImGui::SetColumnWidth(i, ColumnWidths[i]);
			}
		}
		ImGui::Text("Object"); ImGui::NextColumn();
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::IsDefaultObject)) { ImGui::Text("Is Default Obj"); ImGui::NextColumn(); }
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::Outer)) { ImGui::Text("Outer"); ImGui::NextColumn(); }
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::TotalMem)) { ImGui::Text("Total Mem"); ImGui::NextColumn(); }
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::UnknownMem)) { ImGui::Text("Unknown Mem"); ImGui::NextColumn(); }
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedSysMem)) { ImGui::Text("DedSys Mem"); ImGui::NextColumn(); }
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedVidMem)) { ImGui::Text("DedVid Mem"); ImGui::NextColumn(); }
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedSysMem)) { ImGui::Text("SharedSys Mem"); ImGui::NextColumn(); }
		if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedVidMem)) { ImGui::Text("SharedVid Mem"); ImGui::NextColumn(); }
		ImGui::Columns(1);
		ImGui::EndChild();	  // "InstLabel"

		if (ImGui::CollapsingHeader("All Instances", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginChild("InstInfo", ImVec2(0, 360.0f), true);
			ImGui::Columns(VisibleColCount, "ObjInstCol");
			if (VisibleColCount > 1)
			{
				for (int i = 0; i < VisibleColCount; ++i)
				{
					ImGui::SetColumnWidth(i, ColumnWidths[i]);
				}
			}
			for (const FInstanceInspectorInstanceInfo& InspInstance : InstInspInfo.InstanceInfos)
			{
				if (!InstInspInfo.NameFilter.IsActive() || InstInspInfo.NameFilter.PassFilter(TCHAR_TO_ANSI(*GetNameSafe(InspInstance.InstanceWkPtr.Get()))))
				{
					if (InspInstance.InstanceWkPtr.IsValid() && !InspInstance.InstanceWkPtr.IsStale())
					{
						if (ImGui::SmallButton(Ansi(*FString::Printf(TEXT("Inspect##%.54ls"), *GetNameSafe(InspInstance.InstanceWkPtr.Get())))))
						{
							InstInspInfo.InspectedInstance.SetInpectedInstance(InspInstance.InstanceWkPtr);
						}
						ImGui::SameLine();
						ImGui::Text("%s", Ansi(*InspInstance.InstanceWkPtr->GetName()));
						if (ImGui::IsItemHovered())
						{
							DrawHoveredItemInstanceTooltip(InspInstance.InstanceWkPtr.Get());
						}
						ImGui::NextColumn();

						if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::IsDefaultObject)) { ImGui::Text(InspInstance.InstanceWkPtr->IsDefaultSubobject() ? "true" : "false"); ImGui::NextColumn(); }
						if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::Outer)) { 
							ImGui::Text("%s", Ansi(*GetNameSafe(InspInstance.InstanceWkPtr->GetOuter()))); 
							if (ImGui::IsItemHovered())
							{
								DrawHoveredItemInstanceTooltip(InspInstance.InstanceWkPtr.Get());
							}
						
							ImGui::NextColumn(); 
						}
					}
					else
					{
						ImGui::Text("STALE/INVALID PTR"); ImGui::NextColumn();
						if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::IsDefaultObject)) { ImGui::NextColumn(); }
						if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::Outer)) { ImGui::NextColumn(); }
					}
				
					if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::TotalMem)) { ImGui::Text("%.04f MB", InspInstance.MemInfo.TotalMemoryMB); ImGui::NextColumn(); }
					if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::UnknownMem)) { ImGui::Text("%.04f MB", InspInstance.MemInfo.UnknownMemoryMB); ImGui::NextColumn(); }
					if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedSysMem)) { ImGui::Text("%.04f MB", InspInstance.MemInfo.DedSysMemoryMB); ImGui::NextColumn(); }
					if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::DedVidMem)) { ImGui::Text("%.04f MB", InspInstance.MemInfo.DedVidMemoryMB); ImGui::NextColumn(); }
					if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedSysMem)) { ImGui::Text("%.04f MB", InspInstance.MemInfo.SharedSysMemoryMB); ImGui::NextColumn(); }
					if (InstInspInfo.ShowCols.GetShowCol(EColumnTypes::SharedVidMem)) { ImGui::Text("%.04f MB", InspInstance.MemInfo.SharedVidMemoryMB); ImGui::NextColumn(); }
				}
			}
			ImGui::Columns(1);
			ImGui::EndChild(); // "InstInfo"
		}

		if (ImGui::CollapsingHeader("Inspected Instance", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginChild("InspectedInstance", ImVec2(0, 440.0f), true);

			UObject* InspectedInstanceRawPtr = InstInspInfo.InspectedInstance.InspectedWkPtr.Get();
			if (IsValid(InspectedInstanceRawPtr))
			{
				ImGui::Text("        Object: %s (%p)", Ansi(*GetNameSafe(InspectedInstanceRawPtr)), InspectedInstanceRawPtr);
				ImGui::Text("         Class: %s", Ansi(*GetNameSafe(InspectedInstanceRawPtr->GetClass())));
				ImGui::Text("         World: %s (%p)", Ansi(*GetNameSafe(InspectedInstanceRawPtr->GetWorld())), InspectedInstanceRawPtr->GetWorld());
				ImGui::Text("         Outer: %s", Ansi(*GetNameSafe(InspectedInstanceRawPtr->GetOuter())));
				ImGui::Text("default SubObj: %d", InspectedInstanceRawPtr->IsDefaultSubobject());

				if (ImGui::Button("Refresh Referencers (SLOW!)"))
				{
					InstInspInfo.InspectedInstance.UpdateInspectedInstanceReferencers();
				}

				const auto DrawReferencers = [](const TArray<FReferencerInfo>& CachedReferencerInfo, const FString& HeaderString)
				{
					static constexpr int RefInfoColumnCount = 4;
					ImGui::Columns(RefInfoColumnCount);
					ImGui::Text("Name"); ImGui::NextColumn();
					ImGui::Text("Class"); ImGui::NextColumn();
					ImGui::Text("World"); ImGui::NextColumn();
					ImGui::Text("Properties"); ImGui::NextColumn();
					ImGui::Columns(1);

					ImGui::BeginChild(Ansi(*HeaderString), ImVec2(0, 200.0f), true);
					ImGui::Columns(RefInfoColumnCount);

					int RefIter = 0;
					for (const FReferencerInfo& ReferencerInfo : CachedReferencerInfo)
					{
						UObject* ReferencerRawPtr = ReferencerInfo.ReferencerWkPtr.Get();
						ImGui::Text("%03d: %s(%p) (refs: %d)", ++RefIter, Ansi(*GetNameSafe(ReferencerRawPtr)), ReferencerRawPtr, ReferencerInfo.TotalReferences); ImGui::NextColumn();

						if (IsValid(ReferencerRawPtr))
						{
							ImGui::Text("%s", Ansi(*GetNameSafe(ReferencerRawPtr->GetClass()))); ImGui::NextColumn();
							ImGui::Text("%s", Ansi(*GetNameSafe(ReferencerRawPtr->GetWorld()))); ImGui::NextColumn();
							for (int i = 0; i < ReferencerInfo.ReferencingProperties.Num(); ++i)
							{
								const FString& RefPropName = ReferencerInfo.ReferencingProperties[i];
								ImGui::Text("%02d: %s", (i+1), Ansi(*RefPropName));
							}
							ImGui::NextColumn();
						}
						else
						{
							ImGui::Text("*invalid*"); ImGui::NextColumn();
							ImGui::Text("*invalid*"); ImGui::NextColumn();
							ImGui::Text("*invalid*"); ImGui::NextColumn();
						}
					}

					ImGui::Columns(1);
					ImGui::EndChild();
				};


				const FString& InternalRefHeader = FString::Printf(TEXT("Internal Referencers count: %d"), InstInspInfo.InspectedInstance.CachedInternalReferencers.Num());
				const FString& ExternalRefHeader = FString::Printf(TEXT("External Referencers count: %d"), InstInspInfo.InspectedInstance.CachedExternalReferencers.Num());
				if (ImGui::CollapsingHeader(Ansi(*InternalRefHeader)))
				{
					DrawReferencers(InstInspInfo.InspectedInstance.CachedInternalReferencers, InternalRefHeader);
				}
				if (ImGui::CollapsingHeader(Ansi(*ExternalRefHeader), ImGuiTreeNodeFlags_DefaultOpen))
				{
					DrawReferencers(InstInspInfo.InspectedInstance.CachedExternalReferencers, InternalRefHeader);
				}
			}
			else
			{
				ImGui::Text("Inspected Instance Invalid / Stale / Null - Please select an instance in the list above.");
			}

			ImGui::EndChild(); // "InspectedInstance"
		}
	}
}

FImGuiMemoryDebugger::FImGuiMemoryDebugger()
{
	ToolName = TEXT("MemoryDebugger");
}

void FImGuiMemoryDebugger::ImGuiUpdate(float DeltaTime)
{
	if (ImGui::CollapsingHeader("Platform Memory Stats", ImGuiTreeNodeFlags_DefaultOpen))
	{
		FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		// Physical mem
		const float PhysMemPercent = (float)MemoryStats.UsedPhysical / (float)MemoryStats.TotalPhysical;

		ImGui::Text("Physical Memory:");
		ImGui::SameLine();
		{
			const FString& ProgStr = FString::Printf(TEXT("%.02f MB / %.02f MB (%.02f%%)"), MemDebugUtils::IntBytesToFltMB(MemoryStats.UsedPhysical), MemDebugUtils::IntBytesToFltMB(MemoryStats.TotalPhysical), PhysMemPercent * 100.0f);
			ImGui::ProgressBar(PhysMemPercent, ImVec2(-1,0), Ansi(*ProgStr));
		}

		ImGui::Columns(3, "memcols");
		ImGui::Text("Peak Used"); ImGui::NextColumn();
		ImGui::Text("Cur Used"); ImGui::NextColumn();
		ImGui::Text("Total Available"); ImGui::NextColumn();

		ImGui::Text("%.02fMB", MemDebugUtils::IntBytesToFltMB(MemoryStats.PeakUsedPhysical)); ImGui::NextColumn();
		ImGui::Text("%.02fMB", MemDebugUtils::IntBytesToFltMB(MemoryStats.UsedPhysical)); ImGui::NextColumn();
		ImGui::Text("%.02fMB", MemDebugUtils::IntBytesToFltMB(MemoryStats.TotalPhysical)); ImGui::NextColumn();
		ImGui::Columns(1);
		ImGui::Separator();


		// Virtual mem
		const float VirtMemPercent = (float) MemoryStats.UsedVirtual / (float) MemoryStats.TotalVirtual;

		ImGui::Text("Virtual Memory:");
		ImGui::SameLine();
		{
			const FString& ProgStr = FString::Printf(TEXT("%.02f MB / %.02f MB (%.02f%%)"), MemDebugUtils::IntBytesToFltMB(MemoryStats.UsedVirtual), MemDebugUtils::IntBytesToFltMB(MemoryStats.TotalVirtual), VirtMemPercent * 100.0f);
			ImGui::ProgressBar(VirtMemPercent, ImVec2(-1, 0), Ansi(*ProgStr));
		}

		ImGui::Columns(3, "memcols");
		ImGui::Text("Peak Used");
		ImGui::NextColumn();
		ImGui::Text("Cur Used");
		ImGui::NextColumn();
		ImGui::Text("Total Available");
		ImGui::NextColumn();

		ImGui::Text("%.02fMB", MemDebugUtils::IntBytesToFltMB(MemoryStats.PeakUsedVirtual));
		ImGui::NextColumn();
		ImGui::Text("%.02fMB", MemDebugUtils::IntBytesToFltMB(MemoryStats.UsedVirtual));
		ImGui::NextColumn();
		ImGui::Text("%.02fMB", MemDebugUtils::IntBytesToFltMB(MemoryStats.TotalVirtual));
		ImGui::NextColumn();
		ImGui::Columns(1);
		ImGui::Separator();
	}

	if (ImGui::CollapsingHeader("Texture Memory"))
	{
		ImGui::Text("Currently Loaded Textures: "); ImGui::SameLine();
		enum ETextureListMode
		{
			All,
			Streaming,
			NonStreaming,
			Forced
		};
		static int TextureMode = 0;
		if (ImGui::RadioButton("All", TextureMode == ETextureListMode::All)) { TextureMode = ETextureListMode::All; } ImGui::SameLine();
		if (ImGui::RadioButton("Streaming", TextureMode == ETextureListMode::Streaming)) { TextureMode = ETextureListMode::Streaming; } ImGui::SameLine();
		if (ImGui::RadioButton("NonStreaming", TextureMode == ETextureListMode::NonStreaming)) { TextureMode = ETextureListMode::NonStreaming; } ImGui::SameLine();
		if (ImGui::RadioButton("Forced", TextureMode == ETextureListMode::Forced)) { TextureMode = ETextureListMode::Forced; } ImGui::SameLine();
		static bool bAlphaSort = false;
		ImGui::Checkbox("Alpha Sort", &bAlphaSort);
		ImGui::Separator();

		// Find out how many primitive components reference a texture.
		TMap<UTexture2D*, int32> TextureToUsageMap;
		for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
		{
			UPrimitiveComponent* PrimitiveComponent = *It;

			// Use the existing texture streaming functionality to gather referenced textures. Worth noting
			// that GetStreamingTextureInfo doesn't check whether a texture is actually streamable or not
			// and is also implemented for skeletal meshes and such.
#if ENGINE_MAJOR_VERSION == 4
			ImGuiDebugToolsUtils::FStreamingTextureLevelContext LevelContext(EMaterialQualityLevel::Num, PrimitiveComponent);
#elif ENGINE_MAJOR_VERSION == 5
			FStreamingTextureLevelContext LevelContext(EMaterialQualityLevel::Num, PrimitiveComponent);
#endif
			TArray<FStreamingRenderAssetPrimitiveInfo> StreamingTextures;
			PrimitiveComponent->GetStreamingRenderAssetInfo((FStreamingTextureLevelContext&)LevelContext, StreamingTextures);

			//// Increase usage count for all referenced textures
			for (int32 TextureIndex = 0; TextureIndex < StreamingTextures.Num(); TextureIndex++)
			{
				UTexture2D* Texture = Cast<UTexture2D>(StreamingTextures[TextureIndex].RenderAsset);
				if (Texture)
				{
					// Initializes UsageCount to 0 if texture is not found.
					int32 UsageCount = TextureToUsageMap.FindRef(Texture);
					TextureToUsageMap.Add(Texture, UsageCount + 1);
				}
			}
		}
		int32 NumApplicableToMinSize = 0;
		// Collect textures.
		TArray<MemDebugUtils::FSortedTexture> SortedTextures;
		for (TObjectIterator<UTexture> It; It; ++It)
		{
			UTexture* Texture = *It;
			UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
			UTextureCube* TextureCube = Cast<UTextureCube>(Texture);

			int32 LODGroup = Texture->LODGroup;
			int32 NumMips = 0;
			int32 MaxResLODBias = 0;
			int32 MaxAllowedSizeX = 0;
			int32 MaxAllowedSizeY = 0;
			EPixelFormat Format = PF_Unknown;
			int32 DroppedMips = 0;
			int32 CurSizeX = 0;
			int32 CurSizeY = 0;
			bool bIsStreamingTexture = false;
			int32 MaxAllowedSize = Texture->CalcTextureMemorySizeEnum(TMC_AllMipsBiased);
			int32 CurrentSize = Texture->CalcTextureMemorySizeEnum(TMC_ResidentMips);
			int32 UsageCount = 0;
			bool bIsForced = false;

			if (Texture2D != nullptr)
			{
				NumMips = Texture2D->GetNumMips();
				MaxResLODBias = NumMips - Texture2D->GetNumMipsAllowed(false);
				MaxAllowedSizeX = FMath::Max<int32>(Texture2D->GetSizeX() >> MaxResLODBias, 1);
				MaxAllowedSizeY = FMath::Max<int32>(Texture2D->GetSizeY() >> MaxResLODBias, 1);
				Format = Texture2D->GetPixelFormat();
				DroppedMips = Texture2D->GetNumMips() - Texture2D->GetNumResidentMips();
				CurSizeX = FMath::Max<int32>(Texture2D->GetSizeX() >> DroppedMips, 1);
				CurSizeY = FMath::Max<int32>(Texture2D->GetSizeY() >> DroppedMips, 1);
				bIsStreamingTexture = Texture2D->GetStreamingIndex() != INDEX_NONE;
				UsageCount = TextureToUsageMap.FindRef(Texture2D);
				bIsForced = Texture2D->ShouldMipLevelsBeForcedResident() && bIsStreamingTexture;

				if ((NumMips >= Texture2D->GetMinTextureResidentMipCount()) && bIsStreamingTexture)
				{
					NumApplicableToMinSize++;
				}
			}
			else if (TextureCube != nullptr)
			{
				NumMips = TextureCube->GetNumMips();
				Format = TextureCube->GetPixelFormat();
			}

			if (((TextureMode == ETextureListMode::Streaming) && bIsStreamingTexture) ||
				((TextureMode == ETextureListMode::NonStreaming) && !bIsStreamingTexture) ||
				((TextureMode == ETextureListMode::Forced) && bIsForced) ||
				(TextureMode == ETextureListMode::All))
			{
				new (SortedTextures) MemDebugUtils::FSortedTexture(MaxAllowedSizeX, MaxAllowedSizeY, Format, CurSizeX, CurSizeY, MaxResLODBias, MaxAllowedSize, CurrentSize,
													Texture->GetPathName(), LODGroup, bIsStreamingTexture, UsageCount);
			}
		}

		// Retrieve mapping from LOD group enum value to text representation.
		TArray<FString> TextureGroupNames = UTextureLODSettings::GetTextureGroupNames();

		TArray<uint64> TextureGroupCurrentSizes;
		TArray<uint64> TextureGroupMaxAllowedSizes;

		TArray<uint64> FormatCurrentSizes;
		TArray<uint64> FormatMaxAllowedSizes;

		TextureGroupCurrentSizes.AddZeroed(TextureGroupNames.Num());
		TextureGroupMaxAllowedSizes.AddZeroed(TextureGroupNames.Num());

		FormatCurrentSizes.AddZeroed(PF_MAX);
		FormatMaxAllowedSizes.AddZeroed(PF_MAX);

		// Display.
		uint64 TotalMaxAllowedSize = 0;
		uint64 TotalCurrentSize = 0;
		ImGui::BeginChild("TextureList", ImVec2(0, 500.0f));

		// Sort textures by cost.
		SortedTextures.Sort(MemDebugUtils::FCompareFSortedTexture(bAlphaSort));

		ImGui::Columns(7);
		static float ColumnWidths[7];
		for (int i = 0; i <= 6; ++i)
		{
			ImGui::SetColumnWidth(i, ColumnWidths[i]);
		}
		ImGui::Text("OnDisk: Size, Bias- W x H"); ImGui::NextColumn();
		ImGui::Text("InMem: Size- W x H"); ImGui::NextColumn();
		ImGui::Text("Format"); ImGui::NextColumn();
		ImGui::Text("LODGroup"); ImGui::NextColumn();
		ImGui::Text("Name"); ImGui::NextColumn();
		ImGui::Text("Streaming"); ImGui::NextColumn();
		ImGui::Text("Usage Count"); ImGui::NextColumn();
		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::BeginChild("TextureListContents", ImVec2(0, ImGui::GetFrameHeight() - 60.0f));
		ImGui::Columns(7);
		for (int32 TextureIndex = 0; TextureIndex < SortedTextures.Num(); TextureIndex++)
		{
			const MemDebugUtils::FSortedTexture& SortedTexture = SortedTextures[TextureIndex];
			const bool bValidTextureGroup = TextureGroupNames.IsValidIndex(SortedTexture.LODGroup);

			FString AuthoredBiasString(TEXT("?"));
			if (!FPlatformProperties::RequiresCookedData())
			{
				AuthoredBiasString.Empty();
				AuthoredBiasString.AppendInt(SortedTexture.LODBias);
			}
			ImGui::Text(" %i KB, %s- %ix%i", (SortedTexture.MaxAllowedSize + 512) / 1024, Ansi(*AuthoredBiasString), SortedTexture.MaxAllowedSizeX, SortedTexture.MaxAllowedSizeY); ImGui::NextColumn();
			ImGui::Text("%i KB- %ix%i", (SortedTexture.CurrentSize + 512) / 1024, SortedTexture.CurSizeX, SortedTexture.CurSizeY); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(GetPixelFormatString(SortedTexture.Format))); ImGui::NextColumn();
			ImGui::Text("%s", bValidTextureGroup ? Ansi(*TextureGroupNames[SortedTexture.LODGroup]) : "INVALID"); ImGui::NextColumn();
			ImGui::Text("%s", Ansi(*SortedTexture.Name)); ImGui::NextColumn();
			ImGui::Text("%s", SortedTexture.bIsStreaming ? "YES" : "NO"); ImGui::NextColumn();
			ImGui::Text("%i", SortedTexture.UsageCount); ImGui::NextColumn();

			if (bValidTextureGroup)
			{
				TextureGroupCurrentSizes[SortedTexture.LODGroup] += SortedTexture.CurrentSize;
				TextureGroupMaxAllowedSizes[SortedTexture.LODGroup] += SortedTexture.MaxAllowedSize;
			}

			if (SortedTexture.Format >= 0 && SortedTexture.Format < PF_MAX)
			{
				FormatCurrentSizes[SortedTexture.Format] += SortedTexture.CurrentSize;
				FormatMaxAllowedSizes[SortedTexture.Format] += SortedTexture.MaxAllowedSize;
			}

			TotalMaxAllowedSize += SortedTexture.MaxAllowedSize;
			TotalCurrentSize += SortedTexture.CurrentSize;
		}
		for (int i = 0; i <= 6; ++i)
		{
			ColumnWidths[i] = ImGui::GetColumnWidth(i);
		}
		ImGui::Columns(1);
		ImGui::EndChild(); // "TextureListContents"

		ImGui::Separator();
		ImGui::Columns(4);
		ImGui::Text("Total OnDisk"); ImGui::NextColumn();
		ImGui::Text("Total InMem"); ImGui::NextColumn();
		ImGui::Text("Count"); ImGui::NextColumn();
		ImGui::Text("CountApplicableToMin"); ImGui::NextColumn();

		ImGui::Text("%.2f MB", (double)TotalMaxAllowedSize / 1024. / 1024.); ImGui::NextColumn();
		ImGui::Text("%.2f MB", (double)TotalCurrentSize / 1024. / 1024.); ImGui::NextColumn();
		ImGui::Text("%d", SortedTextures.Num()); ImGui::NextColumn();
		ImGui::Text("%d", NumApplicableToMinSize); ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::EndChild(); // "TextureList"
	}


	if (ImGui::CollapsingHeader("Object Memory"))
	{
		static MemDebugUtils::EMemSortType::Type SortMode = MemDebugUtils::EMemSortType::TotalMem;
		static MemDebugUtils::FCachedClassTree CachedClassTree;
		static bool IncludeCDO = false;
		CachedClassTree.TryCacheEntries();
		static EResourceSizeMode::Type ResourceSizeMode = EResourceSizeMode::EstimatedTotal;
		ImGui::Columns(3);
		if (ImGui::Button("Update (SLOW!)"))
		{
			CachedClassTree.UpdateMemoryStats(IncludeCDO, ResourceSizeMode);
			CachedClassTree.SortBy(SortMode);
		}

		static bool AutoUpdateOnStale = true;
		ImGui::Checkbox("Update on Stale", &AutoUpdateOnStale);
		ImGui::BeginChild("##ResourceSizeModeComboMain", ImVec2(210.0f, 18.0f));
		static int ResourceSizeModeComboValue = static_cast<int>(ResourceSizeMode);
		ImGui::Combo("##ResourceSizeModeCombo", &ResourceSizeModeComboValue, "Exclusive\0Estimated Total");
		ResourceSizeMode = static_cast<EResourceSizeMode::Type>(ResourceSizeModeComboValue);
		ImGui::EndChild();
		ImGui::SetColumnWidth(0, 150.0f);
		ImGui::NextColumn();
		ImGui::Text("Include/Sort:");
		ImGui::BeginChild("IncSort", ImVec2(0.0f, 90.0f));
		ImGui::Columns(2);
		static const float RadWidth = 54.0f;
		static ImGuiTools::Utils::FShowCols ShowCols(MemDebugUtils::EColumnTypes::COUNT, &MemDebugUtils::EColumnTypes::DefaultVisibility[0]);
		ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::IsDefaultObject) = false;
		ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::Outer) = false;

		ImGui::SameLine(27.0f);
		if (ImGui::RadioButton("##Alpha", SortMode == MemDebugUtils::EMemSortType::Alpha)) { SortMode = MemDebugUtils::EMemSortType::Alpha; CachedClassTree.SortBy(SortMode); } ImGui::SameLine();
		ImGui::Text("Alpha"); ImGui::NextColumn();

		ImGui::Checkbox("##InstancesCh", &ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::Instances)); ImGui::SameLine();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::Instances))
		{	if (ImGui::RadioButton("##Instances", SortMode == MemDebugUtils::EMemSortType::Instances)) { SortMode = MemDebugUtils::EMemSortType::Instances; CachedClassTree.SortBy(SortMode); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("Instances"); ImGui::NextColumn();

		ImGui::Checkbox("##TotalMemCh", &ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::TotalMem)); ImGui::SameLine();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::TotalMem))
		{ if (ImGui::RadioButton("##TotalMem", SortMode == MemDebugUtils::EMemSortType::TotalMem)) { SortMode = MemDebugUtils::EMemSortType::TotalMem; CachedClassTree.SortBy(SortMode); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("TotalMem"); ImGui::NextColumn();

		ImGui::Checkbox("##UnknownMemCh", &ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::UnknownMem)); ImGui::SameLine();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::UnknownMem))
		{	if (ImGui::RadioButton("##UnknownMem", SortMode == MemDebugUtils::EMemSortType::UnknownMem)) { SortMode = MemDebugUtils::EMemSortType::UnknownMem; CachedClassTree.SortBy(SortMode); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("UnknownMem"); ImGui::NextColumn();

		ImGui::Checkbox("##DedSysMemCh", &ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::DedSysMem)); ImGui::SameLine();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::DedSysMem))
		{	if (ImGui::RadioButton("##DedSysMem", SortMode == MemDebugUtils::EMemSortType::DedSysMem)) { SortMode = MemDebugUtils::EMemSortType::DedSysMem; CachedClassTree.SortBy(SortMode); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("DedSysMem"); ImGui::NextColumn();

		ImGui::Checkbox("##DedVidMemCh", &ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::DedVidMem)); ImGui::SameLine();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::DedVidMem))
		{	if (ImGui::RadioButton("##DedVidMem", SortMode == MemDebugUtils::EMemSortType::DedVidMem)) { SortMode = MemDebugUtils::EMemSortType::DedVidMem; CachedClassTree.SortBy(SortMode); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("DedVidMem"); ImGui::NextColumn();

		ImGui::Checkbox("##SharedSysMemCh", &ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::SharedSysMem)); ImGui::SameLine();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::SharedSysMem))
		{	if (ImGui::RadioButton("##SharedSysMem", SortMode == MemDebugUtils::EMemSortType::SharedSysMem)) { SortMode = MemDebugUtils::EMemSortType::SharedSysMem; CachedClassTree.SortBy(SortMode); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("SharedSysMem"); ImGui::NextColumn();

		ImGui::Checkbox("##SharedVidMemCh", &ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::SharedVidMem)); ImGui::SameLine();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::SharedVidMem))
		{	if (ImGui::RadioButton("##SharedVidMem", SortMode == MemDebugUtils::EMemSortType::SharedVidMem)) { SortMode = MemDebugUtils::EMemSortType::SharedVidMem; CachedClassTree.SortBy(SortMode); } ImGui::SameLine(); }
		else { ImGui::SameLine(RadWidth); }
		ImGui::Text("SharedVidMem"); ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::EndChild(); // "IncSort"
		ImGui::SetColumnWidth(1, 330.0f);
		ImGui::NextColumn();
		static ImGuiTextFilter ClassNameFilter;
		static bool FilterZeroInstances = false;
		ClassNameFilter.Draw("Name Filter", ImGui::GetColumnWidth() - 100.0f);
		ImGui::Checkbox("Filter 0 instances", &FilterZeroInstances);
		ImGui::Checkbox("Include CDO", &IncludeCDO);
		ImGui::Columns(1);
		
		ImGui::BeginChild("ClassListHeader", ImVec2(0, 30.0f), true);
		static constexpr int ColumnCount = 8;
		const int VisibleColCount = ShowCols.CacheShowColCount() + 1;
		ImGui::Columns(VisibleColCount, "ObjMemCol");
		static float ColumnWidths[ColumnCount];
		static constexpr float InfoColWidth = 110.0f;
		ColumnWidths[0] = ImGui::GetWindowWidth() - (InfoColWidth * (VisibleColCount - 1));
		if (VisibleColCount > 1)
		{
			for (int i = 0; i < VisibleColCount; ++i)
			{
				if (i > 0)
				{
					ColumnWidths[i] = InfoColWidth;
				}
				ImGui::SetColumnWidth(i, ColumnWidths[i]);
			}
		}
		ImGui::Text("Class"); ImGui::NextColumn();
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::Instances)) { ImGui::Text("InstanceCount"); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::TotalMem)) { ImGui::Text("Total Mem"); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::UnknownMem)) { ImGui::Text("Unknown Mem"); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::DedSysMem)) { ImGui::Text("DedSys Mem"); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::DedVidMem)) { ImGui::Text("DedVid Mem"); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::SharedSysMem)) { ImGui::Text("SharedSys Mem"); ImGui::NextColumn(); }
		if (ShowCols.GetShowCol(MemDebugUtils::EColumnTypes::SharedVidMem)) { ImGui::Text("SharedVid Mem"); ImGui::NextColumn(); }
		ImGui::Columns(1);
		ImGui::EndChild(); // "ClassListHeader"

		ImGui::BeginChild("ClassList", ImVec2(0, 350.0f), true);
		ImGui::Columns(VisibleColCount, "ObjMemCol");
		if (VisibleColCount > 1)
		{
			for (int i = 0; i < VisibleColCount; ++i)
			{
				ImGui::SetColumnWidth(i, ColumnWidths[i]);
			}
		}
		
		if (!MemDebugUtils::DrawClassChildTreeIndex(CachedClassTree, CachedClassTree.RootIndex, FilterZeroInstances, ClassNameFilter, ShowCols))
		{
			if (AutoUpdateOnStale)
			{
				// Update on Stale will be slow, but will run full memory stats when class data goes stale.
				CachedClassTree.UpdateMemoryStats(IncludeCDO, ResourceSizeMode);
			}
			else
			{
				// If not auto updating, at least re-fetch Class data, which is quick and allows us to show something
				CachedClassTree.TryCacheEntries(/*ForceRecache*/ true);
			}

			// re-sort
			CachedClassTree.SortBy(SortMode);
		}
		ImGui::Columns(1);

		ImGui::EndChild(); // "ClassList"
	}
}

void FImGuiMemoryDebugger::UpdateTool(float DeltaTime)
{
	FImGuiToolWindow::UpdateTool(DeltaTime);

	for (int i = MemDebugUtils::InstanceInspectors.Num() - 1; i >= 0; --i)
	{
		MemDebugUtils::FInstanceInspectorInfo& InstInsp = MemDebugUtils::InstanceInspectors[i];
		if (InstInsp.Class.IsStale() || !InstInsp.Class.IsValid())
		{
			MemDebugUtils::InstanceInspectors.RemoveAt(i);
			continue;
		}

		const FString& InspName = FString::Printf(TEXT("Insp-%.25ls"), *InstInsp.Class->GetName());
		bool WindowOpen = true;
		if (ImGui::Begin(Ansi(*InspName), &WindowOpen))
		{
			MemDebugUtils::DrawObjectInspectorPopup(InstInsp, DeltaTime);
			ImGui::End();
		}

		if (!WindowOpen)
		{
			MemDebugUtils::InstanceInspectors.RemoveAt(i);
		}
	}
}
