// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <imgui.h>

#define Ansi(ws) StringCast<char>(ws).Get()

namespace ImGuiTools
{
	namespace Colors
	{
		static const ImVec4 Blue	= ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
		static const ImVec4 Purple	= ImVec4(0.6f, 0.05f, 0.9f, 1.0f);
		static const ImVec4 Red		= ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		static const ImVec4 Orange	= ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
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
	}	// namespace Utils
}		// namespace ImGuiTools
