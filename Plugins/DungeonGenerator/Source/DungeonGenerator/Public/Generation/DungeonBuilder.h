#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DungeonGrid.h"
#include "Data/DungeonConfig.h"
#include "DungeonBuilder.generated.h"

/**
 * Helper class to execute dungeon generation algorithms based on configuration.
 */
UCLASS()
class DUNGEONGENERATOR_API UDungeonBuilder : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Builds a dungeon grid based on the provided configuration.
	 * @param Config The generation configuration.
	 * @param Outer The outer object for creating algorithm instances.
	 * @return The generated dungeon grid.
	 */
	static FDungeonGrid BuildDungeon(const FDungeonGenConfig& Config, UObject* Outer);
};
