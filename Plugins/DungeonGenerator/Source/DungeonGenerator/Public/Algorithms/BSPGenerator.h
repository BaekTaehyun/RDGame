#pragma once

#include "Algorithms/DungeonAlgorithm.h"
#include "CoreMinimal.h"
#include "CoreBSPGenerator.h"
#include "BSPGenerator.generated.h"

USTRUCT()
struct FBSPNode {
    GENERATED_BODY()

    int32 X;
    int32 Y;
    int32 Width;
    int32 Height;

    TSharedPtr<FBSPNode> Left;
    TSharedPtr<FBSPNode> Right;

    int32 RoomX;
    int32 RoomY;
    int32 RoomWidth;
    int32 RoomHeight;

    bool bIsLeaf = true;

    FBSPNode()
        : X(0), Y(0), Width(0), Height(0), RoomX(0), RoomY(0), RoomWidth(0), RoomHeight(0) {}
    FBSPNode(int32 InX, int32 InY, int32 InWidth, int32 InHeight)
        : X(InX), Y(InY), Width(InWidth), Height(InHeight), RoomX(0), RoomY(0), RoomWidth(0), RoomHeight(0) {}
};

/**
 * Binary Space Partitioning Generator (Wrapper for CoreBSPGenerator).
 */
UCLASS()
class DUNGEONGENERATOR_API UBSPGenerator : public UDungeonAlgorithm {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BSP Settings")
    int32 MinNodeSize = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BSP Settings")
    int32 MinRoomSize = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BSP Settings")
    float SplitRatio = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BSP Settings")
    int32 CorridorWidth = 3; // 복도 폭 (1 이상, 3 권장)

    virtual void Generate(FDungeonGrid& Grid, FRandomStream& RandomStream) override;

    // Multi-Floor Generation
    UFUNCTION(BlueprintCallable, Category = "Dungeon|MultiFloor")
    static FMultiFloorDungeon GenerateMultiFloor(const FMultiFloorDungeonConfig& Config, FRandomStream& RandomStream);

private:
    // Helper functions for multi-floor generation
    static void EnforceVerticalAlignment(FMultiFloorDungeon& MultiFloor);
    static TArray<FStairPosition> PlaceStairs(FMultiFloorDungeon& MultiFloor, const FMultiFloorDungeonConfig& Config, FRandomStream& RandomStream);
    static void MarkStairTiles(FMultiFloorDungeon& MultiFloor);
};
