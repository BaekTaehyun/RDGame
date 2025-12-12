#include "Algorithms/BSPGenerator.h"
#include "DungeonCoreAdapters.h"

void UBSPGenerator::Generate(FDungeonGrid& Grid, FRandomStream& RandomStream) {
    // 1. Convert to Core
    DungeonCore::CoreDungeonGrid CoreGrid = Grid.ToCore();

    // 2. Setup Adapters
    FUnrealRandomAdapter RandomAdapter(RandomStream);
    FUnrealLoggerAdapter LoggerAdapter;

    // 3. Run Core Algorithm
    DungeonCore::CoreBSPGenerator CoreGen;
    CoreGen.MinNodeSize = MinNodeSize;
    CoreGen.MinRoomSize = MinRoomSize;
    CoreGen.SplitRatio = SplitRatio;
    CoreGen.CorridorWidth = CorridorWidth;

    CoreGen.Generate(CoreGrid, RandomAdapter, &LoggerAdapter);

    // 4. Update Unreal Grid
    Grid.FromCore(CoreGrid);
}
