#include "Generation/DungeonBuilder.h"
#include "Algorithms/BSPGenerator.h"
#include "Algorithms/CellularAutomataGenerator.h"
#include "Algorithms/PresetAssemblyGenerator.h"

FDungeonGrid UDungeonBuilder::BuildDungeon(const FDungeonGenConfig &Config,
                                           UObject *Outer) {
  FDungeonGrid Grid;
  Grid.Init(Config.Width, Config.Height, ETileType::Wall); // Init with default

  if (!Outer) {
    UE_LOG(LogTemp, Error, TEXT("UDungeonBuilder: Outer object is null"));
    return Grid;
  }

  UDungeonAlgorithm *Algo = nullptr;

  switch (Config.Algorithm) {
  case EDungeonAlgorithmType::BSP: {
    UBSPGenerator *BSPAlgo = NewObject<UBSPGenerator>(Outer);
    if (BSPAlgo) {
      BSPAlgo->MinRoomSize = Config.MinRoomSize;
      BSPAlgo->CorridorWidth = Config.CorridorWidth;
      Algo = BSPAlgo;
    }
    break;
  }
  case EDungeonAlgorithmType::CellularAutomata: {
    // TODO: Expose CA params in Config if needed
    Algo = NewObject<UCellularAutomataGenerator>(Outer);
    break;
  }
  case EDungeonAlgorithmType::PresetAssembly: {
    UPresetAssemblyGenerator *PresetAlgo =
        NewObject<UPresetAssemblyGenerator>(Outer);
    if (PresetAlgo) {
      PresetAlgo->MaxRoomCount = Config.MaxRoomCount;
      if (!Config.PresetDatabase.IsNull()) {
        PresetAlgo->ModuleDatabase = Config.PresetDatabase.LoadSynchronous();
      }
      Algo = PresetAlgo;
    }
    break;
  }
  }

  if (Algo) {
    FRandomStream RandomStream(Config.Seed);
    Algo->Generate(Grid, RandomStream);
    UE_LOG(
        LogTemp, Log,
        TEXT("UDungeonBuilder: Generated dungeon of size %dx%d using Seed %d"),
        Config.Width, Config.Height, Config.Seed);
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("UDungeonBuilder: Failed to create algorithm instance."));
  }

  return Grid;
}
