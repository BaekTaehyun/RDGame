#pragma once

#include "CoreMinimal.h"
#include "DungeonZoneTypes.generated.h"

/**
 * Zone types for PCG placement
 * Used to determine which PCG graph to execute for each area
 */
UENUM(BlueprintType)
enum class EDungeonZoneType : uint8 
{
    None UMETA(DisplayName = "None"),
    PathEdge UMETA(DisplayName = "PathEdge"),   // Roadside - near walkable areas
    Path UMETA(DisplayName = "Path"),            // Corridor/Road 
    Building UMETA(DisplayName = "Building"),    // Room interior (Floor tiles)
    Nature UMETA(DisplayName = "Nature")         // Forest/Wall area
};

/**
 * Helper class for zone computation
 */
class DUNGEONGENERATOR_API FDungeonZoneHelper
{
public:
    /**
     * Compute zone map from tile types
     * @param Tiles - Array of tile types (Width * Height)
     * @param Width - Grid width
     * @param Height - Grid height
     * @param PathEdgeWidth - Distance from walkable area to consider as PathEdge
     * @return Array of zone types for each tile
     */
    static TArray<EDungeonZoneType> ComputeZoneMap(
        const TArray<uint8>& TileTypes, 
        int32 Width, 
        int32 Height, 
        int32 PathEdgeWidth = 3);
    
    /**
     * Check if tile type is walkable
     */
    static bool IsWalkable(uint8 TileType);
};
