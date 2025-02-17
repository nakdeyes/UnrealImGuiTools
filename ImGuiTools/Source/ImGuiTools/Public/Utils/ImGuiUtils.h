// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <imgui.h>

// forward declarations
struct FInstancedStruct;

#define Ansi(ws) StringCast<char>(ws).Get()

namespace ImGuiTools
{
	namespace Colors
	{
		// 8 Bit (0<->255) RGBA to ImVec4 Color conversion utility
		constexpr ImVec4 RGBColorToImVec4Color(uint8 R, uint8 G, uint8 B, uint8 A = 255)
		{
			return ImVec4(
				static_cast<float>(R) / 255.0f,
				static_cast<float>(G) / 255.0f,
				static_cast<float>(B) / 255.0f,
				static_cast<float>(A) / 255.0f
			);
		}

		static constexpr ImVec4 Aqua			= RGBColorToImVec4Color(0,	255, 255);
		static constexpr ImVec4 Black			= RGBColorToImVec4Color(0,	0,	0);
		static constexpr ImVec4 Blue			= RGBColorToImVec4Color(0,	0,	255);
		static constexpr ImVec4 Blue_Dark		= RGBColorToImVec4Color(0,	0,	139);
		static constexpr ImVec4 Blue_Light		= RGBColorToImVec4Color(173, 216, 230);
		static constexpr ImVec4 Brown			= RGBColorToImVec4Color(165, 42,	42);
		static constexpr ImVec4 Brown_Dark		= RGBColorToImVec4Color(92,	64,	51);
		static constexpr ImVec4 Brown_Light		= RGBColorToImVec4Color(196, 164, 132);
		static constexpr ImVec4 Cyan			= RGBColorToImVec4Color(0,	255, 255);
		static constexpr ImVec4 Green			= RGBColorToImVec4Color(0,	128, 0);
		static constexpr ImVec4 Green_Dark		= RGBColorToImVec4Color(2,	48,	32);
		static constexpr ImVec4 Green_Light		= RGBColorToImVec4Color(144, 238, 144);
		static constexpr ImVec4 Gray			= RGBColorToImVec4Color(169, 169, 169);
		static constexpr ImVec4 Gray_Dark		= RGBColorToImVec4Color(128, 128, 128);
		static constexpr ImVec4 Gray_Light		= RGBColorToImVec4Color(211, 211, 211);
		static constexpr ImVec4 Orange			= RGBColorToImVec4Color(255, 165, 0);
		static constexpr ImVec4 Orange_Dark		= RGBColorToImVec4Color(210, 125, 45);
		static constexpr ImVec4 Orange_Light	= RGBColorToImVec4Color(255, 213, 128);
		static constexpr ImVec4 Purple			= RGBColorToImVec4Color(128, 0,	128);
		static constexpr ImVec4 Purple_Dark		= RGBColorToImVec4Color(48,	25,	52);
		static constexpr ImVec4 Purple_Light	= RGBColorToImVec4Color(203, 195, 227);
		static constexpr ImVec4 Red				= RGBColorToImVec4Color(255, 0,	0);
		static constexpr ImVec4 Red_Dark		= RGBColorToImVec4Color(139, 0,	0);
		static constexpr ImVec4 Red_Light		= RGBColorToImVec4Color(250, 128, 144);
		static constexpr ImVec4 Teal			= RGBColorToImVec4Color(0,	128, 128);
		static constexpr ImVec4 White			= RGBColorToImVec4Color(1,	1,	1);
		static constexpr ImVec4 Yellow			= RGBColorToImVec4Color(255, 255, 0);
		static constexpr ImVec4 Yellow_Dark		= RGBColorToImVec4Color(218, 165, 32);
		static constexpr ImVec4 Yellow_Light	= RGBColorToImVec4Color(250, 250, 51);
	}	// namespace Colors


	namespace Utils
	{
          // FShowCols - small object to manage column visibility states.
		struct IMGUITOOLS_API FShowCols
		{
			FShowCols(int ColCount, bool* OptionalDefaultColArray = nullptr);

			// We provide a way to cache col count so we can query multiple times in a frame without constantly traversing an array
			int CacheShowColCount();
			int GetCachedShowColCount();

			// Main interface to get and set a columns show flag
			bool& GetShowCol(int ColIndex);

		private:
			// Hide default constructor
			FShowCols() = default;

			// max columns used in a lame effort to avoid vector
			static constexpr int MAX_COLUMNS = 10;

			bool ShowColFlags[MAX_COLUMNS];
			int CachedShowColCount = 0;
			int TotalShowColCount = 0;
		};

        // FWorldCache - small object to manage caching valid worlds, for use in situations where there is no relevant world context.
        struct IMGUITOOLS_API FWorldCache
        {
            void TryCacheWorlds();
            TArray<TWeakObjectPtr<UWorld>> CachedWorlds;
        };

		// Draw details of an instanced struct in table form.
		void IMGUITOOLS_API DrawInstancedStructTable(const char* StringId, const FInstancedStruct& InstancedStruct, ImVec2 Size = ImVec2(0,0), bool DoTreeNode = false);

		// Draw the value of a property of unknown type on the provided uobject.
		void IMGUITOOLS_API DrawPropertyValue(const FProperty* Prop, const void* Obj);

		// Given a UStruct, draw it either explicitly or generically.
		void IMGUITOOLS_API DrawScriptStructProperty(const UScriptStruct* StructType, const void* Obj);

		// Given a start and end color, and a time 0 <-> 1, give the interpolated color at that time.
		ImVec4 IMGUITOOLS_API InterpolateImGuiColor(const ImVec4& StartColor, const ImVec4& EndColor, float Time);
	}	// namespace Utils
}		// namespace ImGuiTools
