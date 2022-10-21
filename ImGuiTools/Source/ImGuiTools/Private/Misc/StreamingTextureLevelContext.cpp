// Copyright Epic Games, Inc. All Rights Reserved.
//	NOTE (ImGuiDebugTools) : This UE Engine class is simply copied in here as it is not exported for use in other modules in UE 4.27

#include "StreamingTextureLevelContext.h"

#if ENGINE_MAJOR_VERSION == 4

#include <RHI.h>
#include <Logging/MessageLog.h>
#include <Misc/UObjectToken.h>

namespace ImGuiDebugToolsUtils
{
FStreamingTextureLevelContext::FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, const UPrimitiveComponent* Primitive)
	: TextureGuidToLevelIndex(nullptr)
	, bUseRelativeBoxes(false)
	, BuildDataTimestamp(0)
	, ComponentBuildData(nullptr)
	, QualityLevel(InQualityLevel)
	, FeatureLevel(static_cast<ERHIFeatureLevel::Type>(GMaxRHIFeatureLevel))
{
	if (Primitive)
	{
		const UWorld* World = Primitive->GetWorld();
		if (World)
		{
			FeatureLevel = static_cast<ERHIFeatureLevel::Type>(World->FeatureLevel.GetValue());
		}
	}
}

FStreamingTextureLevelContext::FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, ERHIFeatureLevel::Type InFeatureLevel, bool InUseRelativeBoxes)
	: TextureGuidToLevelIndex(nullptr)
	, bUseRelativeBoxes(InUseRelativeBoxes)
	, BuildDataTimestamp(0)
	, ComponentBuildData(nullptr)
	, QualityLevel(InQualityLevel)
	, FeatureLevel(InFeatureLevel)
{}

FStreamingTextureLevelContext::FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, const ULevel* InLevel, const TMap<FGuid, int32>* InTextureGuidToLevelIndex)
	: TextureGuidToLevelIndex(nullptr)
	, bUseRelativeBoxes(false)
	, BuildDataTimestamp(0)
	, ComponentBuildData(nullptr)
	, QualityLevel(InQualityLevel)
	, FeatureLevel(static_cast<ERHIFeatureLevel::Type>(GMaxRHIFeatureLevel))
{
	if (InLevel)
	{
		const UWorld* World = InLevel->GetWorld();
		if (World)
		{
			FeatureLevel = static_cast<ERHIFeatureLevel::Type>(World->FeatureLevel.GetValue());
		}

		if (InLevel && InTextureGuidToLevelIndex && InLevel->StreamingTextureGuids.Num() > 0 && InLevel->StreamingTextureGuids.Num() == InTextureGuidToLevelIndex->Num())
		{
			bUseRelativeBoxes = !InLevel->bTextureStreamingRotationChanged;
			TextureGuidToLevelIndex = InTextureGuidToLevelIndex;

			// Extra transient data for each texture.
			BoundStates.AddZeroed(InLevel->StreamingTextureGuids.Num());
		}
	}
}

FStreamingTextureLevelContext::~FStreamingTextureLevelContext()
{
	// Reset the level indices for the next use.
	for (const FTextureBoundState& BoundState : BoundStates)
	{
		if (BoundState.Texture)
		{
			BoundState.Texture->LevelIndex = INDEX_NONE;
		}
	}
}

void FStreamingTextureLevelContext::BindBuildData(const TArray<FStreamingTextureBuildInfo>* BuildData)
{
	// Increment the component timestamp, used to know when a texture is processed by a component for the first time.
	// Using a timestamp allows to not reset state in between components.
	++BuildDataTimestamp;

	if (TextureGuidToLevelIndex && CVarStreamingUseNewMetrics.GetValueOnGameThread() != 0)	  // No point in binding data if there is no possible remapping.
	{
		// Process the build data in order to be able to map a texture object to the build data entry.
		ComponentBuildData = BuildData;
		if (BuildData && BoundStates.Num() > 0)
		{
			for (int32 Index = 0; Index < BuildData->Num(); ++Index)
			{
				int32 TextureLevelIndex = (*BuildData)[Index].TextureLevelIndex;
				if (BoundStates.IsValidIndex(TextureLevelIndex))
				{
					FTextureBoundState& BoundState = BoundStates[TextureLevelIndex];
					BoundState.BuildDataIndex = Index;					   // The index of this texture in the component build data.
					BoundState.BuildDataTimestamp = BuildDataTimestamp;	   // The component timestamp will indicate that the index is valid to be used.
				}
			}
		}
	}
	else
	{
		ComponentBuildData = nullptr;
	}
}

int32* FStreamingTextureLevelContext::GetBuildDataIndexRef(UTexture2D* Texture2D)
{
	if (ComponentBuildData)	   // If there is some build data to map to.
	{
		if (Texture2D->LevelIndex == INDEX_NONE)
		{
			check(TextureGuidToLevelIndex);	   // Can't bind ComponentData without the remapping.
			const int32* LevelIndex = TextureGuidToLevelIndex->Find(Texture2D->GetLightingGuid());
			if (LevelIndex)	   // If the index is found in the map, the index is valid in BoundStates
			{
				// Here we need to support the invalid case where 2 textures have the same GUID.
				// If this happens, BoundState.Texture will already be set.
				FTextureBoundState& BoundState = BoundStates[*LevelIndex];
				if (!BoundState.Texture)
				{
					Texture2D->LevelIndex = *LevelIndex;
					BoundState.Texture = Texture2D;	   // Update the mapping now!
				}
				else	// Don't allow 2 textures to be using the same level index otherwise UTexture2D::LevelIndex won't be reset properly in the destructor.
				{
					FMessageLog("AssetCheck")
						.Error()
						->AddToken(FUObjectToken::Create(BoundState.Texture))
						->AddToken(FUObjectToken::Create(Texture2D))
						->AddToken(
							FTextToken::Create(NSLOCTEXT("AssetCheck", "TextureError_NonUniqueLightingGuid",
														 "Same lighting guid, modify or touch any property in the texture editor to generate a new guid and fix the issue.")));

					// This will fallback not using the precomputed data. Note also that the other texture might be using the wrong precomputed data.
					return nullptr;
				}
			}
			else	// Otherwise add a dummy entry to prevent having to search in the map multiple times.
			{
				Texture2D->LevelIndex = BoundStates.Add(FTextureBoundState(Texture2D));
			}
		}

		FTextureBoundState& BoundState = BoundStates[Texture2D->LevelIndex];
		check(BoundState.Texture == Texture2D);

		if (BoundState.BuildDataTimestamp == BuildDataTimestamp)
		{
			return &BoundState.BuildDataIndex;	  // Only return the bound static if it has data relative to this component.
		}
	}
	return nullptr;
}

void FStreamingTextureLevelContext::ProcessMaterial(const FBoxSphereBounds& ComponentBounds,
													const FPrimitiveMaterialInfo& MaterialData,
													float ComponentScaling,
													TArray<FStreamingRenderAssetPrimitiveInfo>& OutStreamingTextures)
{
	ensure(MaterialData.IsValid());

	TArray<UTexture*> Textures;
	MaterialData.Material->GetUsedTextures(Textures, QualityLevel, false, static_cast<::ERHIFeatureLevel::Type>(FeatureLevel), false);

	for (UTexture* Texture : Textures)
	{
		UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
		if (!Texture2D || !Texture2D->IsStreamable())
		{
			continue;
		}

		int32* BuildDataIndex = GetBuildDataIndexRef(Texture2D);
		if (BuildDataIndex)
		{
			if (*BuildDataIndex != INDEX_NONE)
			{
				FStreamingRenderAssetPrimitiveInfo& Info = *new (OutStreamingTextures) FStreamingRenderAssetPrimitiveInfo();
				const FStreamingTextureBuildInfo& BuildInfo = (*ComponentBuildData)[*BuildDataIndex];

				Info.RenderAsset = Texture2D;
				Info.TexelFactor = BuildInfo.TexelFactor * ComponentScaling;
				Info.PackedRelativeBox = bUseRelativeBoxes ? BuildInfo.PackedRelativeBox : PackedRelativeBox_Identity;
				UnpackRelativeBox(ComponentBounds, Info.PackedRelativeBox, Info.Bounds);

				// Indicate that this texture build data has already been processed.
				// The build data use the merged results of all material so it only needs to be processed once.
				*BuildDataIndex = INDEX_NONE;
			}
		}
		else	// Otherwise create an entry using the available data.
		{
			float TextureDensity = MaterialData.Material->GetTextureDensity(Texture->GetFName(), *MaterialData.UVChannelData);

			if (!TextureDensity)
			{
				// Fallback assuming a sampling scale of 1 using the UV channel 0;
				TextureDensity = MaterialData.UVChannelData->LocalUVDensities[0];
			}

			if (TextureDensity)
			{
				FStreamingRenderAssetPrimitiveInfo& Info = *new (OutStreamingTextures) FStreamingRenderAssetPrimitiveInfo();

				Info.RenderAsset = Texture2D;
				Info.TexelFactor = TextureDensity * ComponentScaling;
				Info.PackedRelativeBox = bUseRelativeBoxes ? MaterialData.PackedRelativeBox : PackedRelativeBox_Identity;
				UnpackRelativeBox(ComponentBounds, Info.PackedRelativeBox, Info.Bounds);
			}
		}
	}
}
} // namespace ImGuiDebugToolsUtils
#endif // #if ENGINE_MAJOR_VERSION == 4