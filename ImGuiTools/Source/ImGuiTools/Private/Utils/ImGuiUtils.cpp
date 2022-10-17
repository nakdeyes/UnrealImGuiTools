// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "Utils/ImGuiUtils.h"

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
