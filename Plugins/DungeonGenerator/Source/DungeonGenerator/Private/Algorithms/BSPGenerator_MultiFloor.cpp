#include "Algorithms/BSPGenerator.h"
#include "DungeonCoreAdapters.h"

FMultiFloorDungeon UBSPGenerator::GenerateMultiFloor(
    const FMultiFloorDungeonConfig& Config,
    FRandomStream& RandomStream) {

    FMultiFloorDungeon MultiFloor;
    MultiFloor.NumFloors = Config.NumFloors;
    MultiFloor.Floors.SetNum(Config.NumFloors);

    UE_LOG(LogTemp, Log, TEXT("GenerateMultiFloor: Starting generation of %d floors"), Config.NumFloors);

    // Step 1: Generate each floor
    for (int32 FloorIndex = 0; FloorIndex < Config.NumFloors; FloorIndex++) {
        FDungeonGrid& Floor = MultiFloor.Floors[FloorIndex];
        Floor.Init(Config.Width, Config.Height, ETileType::Wall);

        // Create floor-specific random stream
        FRandomStream FloorRandom(RandomStream.GetInitialSeed() + FloorIndex * 1000);

        // Setup adapters
        FUnrealRandomAdapter RandomAdapter(FloorRandom);
        FUnrealLoggerAdapter LoggerAdapter;

        // Generate using core BSP
        DungeonCore::CoreDungeonGrid CoreGrid = Floor.ToCore();
        DungeonCore::CoreBSPGenerator CoreGen;
        CoreGen.MinNodeSize = 10;
        CoreGen.MinRoomSize = 6;
        CoreGen.SplitRatio = 0.4f;
        CoreGen.Generate(CoreGrid, RandomAdapter, &LoggerAdapter);

        Floor.FromCore(CoreGrid);
        UE_LOG(LogTemp, Log, TEXT("  Floor %d generated"), FloorIndex + 1);
    }

    // Step 2: Enforce vertical alignment
    if (Config.bEnforceVerticalAlignment) {
        EnforceVerticalAlignment(MultiFloor);
        UE_LOG(LogTemp, Log, TEXT("  Vertical alignment enforced"));
    }

    // Step 3: Place stairs
    MultiFloor.Stairs = PlaceStairs(MultiFloor, Config, RandomStream);
    UE_LOG(LogTemp, Log, TEXT("  Placed %d stairs"), MultiFloor.Stairs.Num());

    // Step 4: Mark stair tiles
    MarkStairTiles(MultiFloor);

    UE_LOG(LogTemp, Log, TEXT("GenerateMultiFloor: Complete"));
    return MultiFloor;
}

void UBSPGenerator::EnforceVerticalAlignment(FMultiFloorDungeon& MultiFloor) {
    for (int32 Floor = MultiFloor.NumFloors - 1; Floor > 0; Floor--) {
        FDungeonGrid& CurrentFloor = MultiFloor.Floors[Floor];
        FDungeonGrid& BelowFloor = MultiFloor.Floors[Floor - 1];

        for (int32 Y = 0; Y < CurrentFloor.Height; Y++) {
            for (int32 X = 0; X < CurrentFloor.Width; X++) {
                FDungeonTile& CurrentTile = CurrentFloor.GetTile(X, Y);
                const FDungeonTile& BelowTile = BelowFloor.GetTile(X, Y);

                if (BelowTile.Type == ETileType::Wall && CurrentTile.Type != ETileType::Wall) {
                    CurrentTile.Type = ETileType::Wall;
                }
            }
        }
    }
}

TArray<FStairPosition> UBSPGenerator::PlaceStairs(
    FMultiFloorDungeon& MultiFloor,
    const FMultiFloorDungeonConfig& Config,
    FRandomStream& RandomStream) {

    TArray<FStairPosition> AllStairs;
    const float MinDistSq = Config.MinStairDistance * Config.MinStairDistance;

    for (int32 Floor = 0; Floor < MultiFloor.NumFloors - 1; Floor++) {
        TArray<FIntPoint> ValidPositions;
        for (int32 Y = 0; Y < MultiFloor.Floors[Floor].Height; Y++) {
            for (int32 X = 0; X < MultiFloor.Floors[Floor].Width; X++) {
                if (MultiFloor.IsFloorTile(Floor, X, Y) && MultiFloor.IsFloorTile(Floor + 1, X, Y)) {
                    ValidPositions.Add(FIntPoint(X, Y));
                }
            }
        }

        if (ValidPositions.Num() == 0) {
            continue;
        }

        // Shuffle
        for (int32 i = ValidPositions.Num() - 1; i > 0; i--) {
            const int32 j = RandomStream.RandRange(0, i);
            ValidPositions.Swap(i, j);
        }

        int32 StairsPlaced = 0;
        for (const FIntPoint& Pos : ValidPositions) {
            if (StairsPlaced >= Config.StairsPerFloor) break;

            bool bTooClose = false;
            for (const FStairPosition& ExistingStair : AllStairs) {
                if (ExistingStair.FloorIndex == Floor) {
                    const float DistSq = FVector2D::DistSquared(
                        FVector2D(Pos.X, Pos.Y),
                        FVector2D(ExistingStair.X, ExistingStair.Y));
                    if (DistSq < MinDistSq) {
                        bTooClose = true;
                        break;
                    }
                }
            }

            if (!bTooClose) {
                FStairPosition Stair;
                Stair.FloorIndex = Floor;
                Stair.X = Pos.X;
                Stair.Y = Pos.Y;
                Stair.TargetFloor = Floor + 1;
                AllStairs.Add(Stair);
                StairsPlaced++;
            }
        }
    }

    return AllStairs;
}

void UBSPGenerator::MarkStairTiles(FMultiFloorDungeon& MultiFloor) {
    for (const FStairPosition& Stair : MultiFloor.Stairs) {
        if (Stair.FloorIndex >= 0 && Stair.FloorIndex < MultiFloor.Floors.Num()) {
            FDungeonGrid& Floor = MultiFloor.Floors[Stair.FloorIndex];
            if (Floor.IsValid(Stair.X, Stair.Y)) {
                FDungeonTile& Tile = Floor.GetTile(Stair.X, Stair.Y);
                Tile.Type = ETileType::Stair;
                Tile.StairTargetFloor = Stair.TargetFloor;
            }

            if (Stair.TargetFloor >= 0 && Stair.TargetFloor < MultiFloor.Floors.Num()) {
                FDungeonGrid& TargetFloor = MultiFloor.Floors[Stair.TargetFloor];
                if (TargetFloor.IsValid(Stair.X, Stair.Y)) {
                    FDungeonTile& TargetTile = TargetFloor.GetTile(Stair.X, Stair.Y);
                    TargetTile.Type = ETileType::Stair;
                    TargetTile.StairTargetFloor = Stair.FloorIndex;
                }
            }
        }
    }
}
