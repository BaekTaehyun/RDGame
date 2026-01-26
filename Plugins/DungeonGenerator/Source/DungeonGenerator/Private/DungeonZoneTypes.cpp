#include "DungeonZoneTypes.h"

TArray<EDungeonZoneType> FDungeonZoneHelper::ComputeZoneMap(
    const TArray<uint8>& TileTypes, 
    int32 Width, 
    int32 Height, 
    int32 PathEdgeWidth)
{
    TArray<EDungeonZoneType> ZoneMap;
    ZoneMap.SetNum(Width * Height);
    
    // First pass: assign basic zones based on tile type
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            int32 Index = Y * Width + X;
            uint8 Type = TileTypes[Index];
            
            // ETileType values: 0=None, 1=Floor, 2=Wall, 3=Corridor, 4=Door, 5=Stair
            if (Type == 1) // Floor
            {
                ZoneMap[Index] = EDungeonZoneType::Building;
            }
            else if (Type == 3 || Type == 4) // Corridor or Door
            {
                ZoneMap[Index] = EDungeonZoneType::Path;
            }
            else // Wall (2) or None (0)
            {
                ZoneMap[Index] = EDungeonZoneType::Nature;
            }
        }
    }
    
    // Second pass: find PathEdge zones (Wall tiles near walkable areas)
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            int32 Index = Y * Width + X;
            
            // Only process Nature (Wall) tiles
            if (ZoneMap[Index] != EDungeonZoneType::Nature)
                continue;
            
            // Check distance to nearest walkable tile
            bool bNearWalkable = false;
            for (int32 DY = -PathEdgeWidth; DY <= PathEdgeWidth && !bNearWalkable; DY++)
            {
                for (int32 DX = -PathEdgeWidth; DX <= PathEdgeWidth && !bNearWalkable; DX++)
                {
                    int32 NX = X + DX;
                    int32 NY = Y + DY;
                    
                    if (NX < 0 || NX >= Width || NY < 0 || NY >= Height)
                        continue;
                    
                    uint8 NeighborType = TileTypes[NY * Width + NX];
                    // Floor(1), Corridor(3), Door(4), Stair(5) are walkable
                    if (NeighborType == 1 || NeighborType == 3 || NeighborType == 4 || NeighborType == 5)
                    {
                        bNearWalkable = true;
                    }
                }
            }
            
            if (bNearWalkable)
            {
                ZoneMap[Index] = EDungeonZoneType::PathEdge;
            }
        }
    }
    
    return ZoneMap;
}

bool FDungeonZoneHelper::IsWalkable(uint8 TileType)
{
    // Floor(1), Corridor(3), Door(4), Stair(5)
    return TileType == 1 || TileType == 3 || TileType == 4 || TileType == 5;
}
