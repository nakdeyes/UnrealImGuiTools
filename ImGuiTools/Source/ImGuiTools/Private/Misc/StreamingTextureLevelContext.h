// Copyright Epic Games, Inc. All Rights Reserved.
//	NOTE (ImGuiDebugTools) : This UE Engine class is simply copied in here as it is not exported for use in other modules in UE 4.27

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 4

#include <SceneTypes.h>
#include <RHIDefinitions.h>

namespace ImGuiDebugToolsUtils
{
/**
	* The RHI's feature level indicates what level of support can be relied upon.
	* Note: these are named after graphics API's like ES3 but a feature level can be used with a different API (eg ERHIFeatureLevel::ES3.1 on D3D11)
	* As long as the graphics API supports all the features of the feature level (eg no ERHIFeatureLevel::SM5 on OpenGL ES3.1)
	*/
namespace ERHIFeatureLevel
{
	enum Type
	{
		/** Feature level defined by the core capabilities of OpenGL ES2. Deprecated */
		ES2_REMOVED,

		/** Feature level defined by the core capabilities of OpenGL ES3.1 & Metal/Vulkan. */
		ES3_1,

		/**
			* Feature level defined by the capabilities of DX10 Shader Model 4.
			* SUPPORT FOR THIS FEATURE LEVEL HAS BEEN ENTIRELY REMOVED.
			*/
			SM4_REMOVED,

			/**
			* Feature level defined by the capabilities of DX11 Shader Model 5.
			*   Compute shaders with shared memory, group sync, UAV writes, integer atomics
			*   Indirect drawing
			*   Pixel shaders with UAV writes
			*   Cubemap arrays
			*   Read-only depth or stencil views (eg read depth buffer as SRV while depth test and stencil write)
			* Tessellation is not considered part of Feature Level SM5 and has a separate capability flag.
			*/
			SM5,
			Num
	};
};
/**
 * Context used to resolve FStreamingTextureBuildInfo to FStreamingRenderAssetPrimitiveInfo
 * The context make sure that build data and each texture is only processed once per component (with constant time).
 * It manage internally structures used to accelerate the binding between precomputed data and textures,
 * so that there is only one map lookup per texture per level.
 * There is some complexity here because the build data does not reference directly texture objects to avoid hard references
 * which would load texture when the component is loaded, which could be wrong since the build data is built for a specific
 * feature level and quality level. The current feature and quality could reference more or less textures.
 * This requires the logic to not submit a streaming entry for precomputed data, as well as submit fallback data for
 * texture that were referenced in the texture streaming build.
 */
class FStreamingTextureLevelContext
{
	/** Reversed lookup for ULevel::StreamingTextureGuids. */
	const TMap<FGuid, int32>* TextureGuidToLevelIndex;

	/** Whether the precomputed relative bounds should be used or not.  Will be false if the transform level was rotated since the last texture streaming build. */
	bool bUseRelativeBoxes;

	/** An id used to identify the component build data. */
	int32 BuildDataTimestamp;

	/** The last bound component texture streaming build data. */
	const TArray<FStreamingTextureBuildInfo>* ComponentBuildData;

	struct FTextureBoundState
	{
		FTextureBoundState() {}

		FTextureBoundState(UTexture2D* InTexture) : BuildDataTimestamp(0), BuildDataIndex(0), Texture(InTexture) {}

		/** The timestamp of the build data to indentify whether BuildDataIndex is valid or not. */
		int32 BuildDataTimestamp;
		/** The ComponentBuildData Index referring this texture. */
		int32 BuildDataIndex;
		/**  The texture relative to this entry. */
		UTexture2D* Texture;
	};

	/*
	 * The component state of the each texture. Used to prevent processing each texture several time.
	 * Also used to find quickly the build data relating to each texture.
	 */
	TArray<FTextureBoundState> BoundStates;

	EMaterialQualityLevel::Type QualityLevel;
	ERHIFeatureLevel::Type FeatureLevel;

	int32* GetBuildDataIndexRef(UTexture2D* Texture2D);

public:

	// Needs InLevel to use precomputed data from 
	FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, const ULevel* InLevel = nullptr, const TMap<FGuid, int32>* InTextureGuidToLevelIndex = nullptr);
	FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, ERHIFeatureLevel::Type InFeatureLevel, bool InUseRelativeBoxes);
	FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, const UPrimitiveComponent* Primitive);
	~FStreamingTextureLevelContext();

	void BindBuildData(const TArray<FStreamingTextureBuildInfo>* PreBuiltData);
	void ProcessMaterial(const FBoxSphereBounds& ComponentBounds, const FPrimitiveMaterialInfo& MaterialData, float ComponentScaling, TArray<FStreamingRenderAssetPrimitiveInfo>& OutStreamingTextures);

	EMaterialQualityLevel::Type GetQualityLevel() { return QualityLevel; }
	ERHIFeatureLevel::Type GetFeatureLevel() { return FeatureLevel; }
};

} // namespace ImGuiDebugToolsUtils

#endif // #if ENGINE_MAJOR_VERSION == 4