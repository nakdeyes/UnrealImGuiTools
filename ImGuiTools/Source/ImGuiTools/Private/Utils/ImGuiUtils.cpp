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
