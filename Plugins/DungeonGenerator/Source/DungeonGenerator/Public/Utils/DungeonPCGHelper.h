#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DungeonGrid.h"
#include "Data/PCGPointData.h"
#include "PCGComponent.h"
#include "DungeonPCGHelper.generated.h"

/**
 * Helper library to bridge DungeonGenerator data to PCG.
 */
UCLASS()
class DUNGEONGENERATOR_API UDungeonPCGHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Converts specific tiles from the Dungeon Grid into PCG Point Data.
	 * @param Grid The source dungeon grid.
	 * @param TargetTypes Bitmask or check for these tile types. (Currently just exact match or logic)
	 * @param TileSize Size of each tile in world units.
	 * @return A new UPCGPointData asset containing the points.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|PCG")
	static UPCGPointData* GenerateDungeonPoints(const FDungeonGrid& Grid, ETileType TargetType, float TileSize);

	/**
	 * Helper to set data on a PCG Component.
	 */
	static void FillPCGComponent(UPCGComponent* Component, const FDungeonGrid& Grid);
};
