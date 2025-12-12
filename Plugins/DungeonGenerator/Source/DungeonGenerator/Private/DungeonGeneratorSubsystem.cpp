#include "DungeonGeneratorSubsystem.h"
#include "Algorithms/BSPGenerator.h"
#include "Algorithms/CellularAutomataGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Rendering/DungeonLightingManager.h"
#include "Rendering/DungeonMeshGenerator.h"
#include "Rendering/DungeonNavigationBuilder.h"
#include "Rendering/DungeonTileRenderer.h"


void UDungeonGeneratorSubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);

  // Create rendering managers
  TileRenderer = NewObject<UDungeonTileRenderer>(this);
  MeshGenerator = NewObject<UDungeonMeshGenerator>(this);
  LightingManager = NewObject<UDungeonLightingManager>(this);
  NavigationBuilder = NewObject<UDungeonNavigationBuilder>(this);

  UE_LOG(LogTemp, Log, TEXT("DungeonGeneratorSubsystem Initialized"));
}

void UDungeonGeneratorSubsystem::Deinitialize() {
  ClearRenderedDungeon();
  Super::Deinitialize();
  UE_LOG(LogTemp, Log, TEXT("DungeonGeneratorSubsystem Deinitialized"));
}

FDungeonGrid UDungeonGeneratorSubsystem::GenerateDungeon(
    int32 Width, int32 Height, int32 Seed,
    EDungeonAlgorithmType AlgorithmType) {

  // Validation
  if (Width < 10 || Height < 10) {
    UE_LOG(LogTemp, Error,
           TEXT("GenerateDungeon: Grid size too small (%dx%d), minimum 10x10"),
           Width, Height);
    return CurrentGrid; // Return empty/previous grid
  }

  // Initialize Grid (clears previous state)
  CurrentGrid.Init(Width, Height, ETileType::Wall);

  // Initialize RandomStream with new seed (ensures determinism)
  RandomStream.Initialize(Seed);

  UE_LOG(LogTemp, Log,
         TEXT("GenerateDungeon: Starting generation %dx%d with seed %d"), Width,
         Height, Seed);

  // Create algorithm instance (fresh instance for each generation)
  UDungeonAlgorithm *Algo = nullptr;

  switch (AlgorithmType) {
  case EDungeonAlgorithmType::BSP:
    Algo = NewObject<UBSPGenerator>(this);
    break;
  case EDungeonAlgorithmType::CellularAutomata:
    Algo = NewObject<UCellularAutomataGenerator>(this);
    break;
  default:
    UE_LOG(LogTemp, Error, TEXT("GenerateDungeon: Unknown algorithm type"));
    return CurrentGrid;
  }

  // Generate dungeon
  if (Algo) {
    Algo->Generate(CurrentGrid, RandomStream);
    UE_LOG(LogTemp, Log, TEXT("GenerateDungeon: Generation complete"));
  }

  return CurrentGrid;
}

void UDungeonGeneratorSubsystem::RenderDungeon(
    const FDungeonGrid &Grid, EDungeonAlgorithmType AlgorithmType,
    AActor *OwnerActor) {
  if (!OwnerActor) {
    UE_LOG(LogTemp, Error, TEXT("RenderDungeon: OwnerActor is null"));
    return;
  }

  // Clear previous rendering
  ClearRenderedDungeon();

  UWorld *World = OwnerActor->GetWorld();
  if (!World) {
    return;
  }

  // Render based on algorithm type
  if (AlgorithmType == EDungeonAlgorithmType::BSP && bEnableWallRendering) {
    // BSP: Use Bitmasking with ISMC
    UInstancedStaticMeshComponent *WallISMC =
        NewObject<UInstancedStaticMeshComponent>(OwnerActor);
    WallISMC->RegisterComponent();
    WallISMC->AttachToComponent(
        OwnerActor->GetRootComponent(),
        FAttachmentTransformRules::KeepRelativeTransform);
    InstancedMeshComponents.Add(WallISMC);

    UInstancedStaticMeshComponent *CeilingISMC = nullptr;
    UInstancedStaticMeshComponent *FloorISMC = nullptr;

    if (bEnableCeiling) {
      CeilingISMC = NewObject<UInstancedStaticMeshComponent>(OwnerActor);
      CeilingISMC->RegisterComponent();
      CeilingISMC->AttachToComponent(
          OwnerActor->GetRootComponent(),
          FAttachmentTransformRules::KeepRelativeTransform);
      InstancedMeshComponents.Add(CeilingISMC);
    }

    if (bEnableFloor) {
      FloorISMC = NewObject<UInstancedStaticMeshComponent>(OwnerActor);
      FloorISMC->RegisterComponent();
      FloorISMC->AttachToComponent(
          OwnerActor->GetRootComponent(),
          FAttachmentTransformRules::KeepRelativeTransform);
      InstancedMeshComponents.Add(FloorISMC);
    }

    if (TileRenderer) {
      TileRenderer->WallMeshTable = BSPWallMeshes;
      TileRenderer->CeilingMesh = BSPCeilingMesh;
      TileRenderer->FloorMesh = BSPFloorMesh;
      TileRenderer->bGenerateCeiling = bEnableCeiling;
      TileRenderer->bGenerateFloor = bEnableFloor;
      TileRenderer->GenerateBSPTiles(Grid, WallISMC, CeilingISMC, FloorISMC);
    }
  } else if (AlgorithmType == EDungeonAlgorithmType::CellularAutomata &&
             bEnableWallRendering) {
    // CA: Use Marching Squares with Procedural Mesh
    UProceduralMeshComponent *WallMesh =
        NewObject<UProceduralMeshComponent>(OwnerActor);
    WallMesh->RegisterComponent();
    WallMesh->AttachToComponent(
        OwnerActor->GetRootComponent(),
        FAttachmentTransformRules::KeepRelativeTransform);
    ProceduralMeshComponents.Add(WallMesh);

    UProceduralMeshComponent *FloorMesh = nullptr;
    UProceduralMeshComponent *CeilingMesh = nullptr;

    if (bEnableFloor) {
      FloorMesh = NewObject<UProceduralMeshComponent>(OwnerActor);
      FloorMesh->RegisterComponent();
      FloorMesh->AttachToComponent(
          OwnerActor->GetRootComponent(),
          FAttachmentTransformRules::KeepRelativeTransform);
      ProceduralMeshComponents.Add(FloorMesh);
    }

    if (bEnableCeiling) {
      CeilingMesh = NewObject<UProceduralMeshComponent>(OwnerActor);
      CeilingMesh->RegisterComponent();
      CeilingMesh->AttachToComponent(
          OwnerActor->GetRootComponent(),
          FAttachmentTransformRules::KeepRelativeTransform);
      ProceduralMeshComponents.Add(CeilingMesh);
    }

    if (MeshGenerator) {
      MeshGenerator->CaveWallMaterial = CaveWallMaterial;
      MeshGenerator->CaveFloorMaterial = CaveFloorMaterial;
      MeshGenerator->CaveCeilingMaterial = CaveCeilingMaterial;
      MeshGenerator->bGenerateCeiling = bEnableCeiling;
      MeshGenerator->bGenerateFloor = bEnableFloor;
      MeshGenerator->GenerateCaveWalls(Grid, WallMesh, FloorMesh, CeilingMesh);
    }
  }

  // Place lights
  if (bAutoPlaceLights && LightingManager) {
    LightingManager->RoomLightClass = RoomLightClass;
    LightingManager->CorridorLightClass = CorridorLightClass;

    if (AlgorithmType == EDungeonAlgorithmType::BSP) {
      LightingManager->PlaceDungeonLights(Grid, World, SpawnedLights);
    } else {
      LightingManager->PlaceCaveLights(Grid, World, SpawnedLights);
    }
  }

  // Build NavMesh
  if (bGenerateNavMesh && NavigationBuilder) {
    NavigationBuilder->BuildNavMesh(Grid, World);
  }

  UE_LOG(LogTemp, Log, TEXT("RenderDungeon: Complete"));
}

void UDungeonGeneratorSubsystem::ClearRenderedDungeon() {
  // Destroy ISMC
  for (UInstancedStaticMeshComponent *ISMC : InstancedMeshComponents) {
    if (ISMC) {
      ISMC->DestroyComponent();
    }
  }
  InstancedMeshComponents.Empty();

  // Destroy Procedural Meshes
  for (UProceduralMeshComponent *PMC : ProceduralMeshComponents) {
    if (PMC) {
      PMC->DestroyComponent();
    }
  }
  ProceduralMeshComponents.Empty();

  // Destroy Lights
  for (APointLight *Light : SpawnedLights) {
    if (Light) {
      Light->Destroy();
    }
  }
  SpawnedLights.Empty();

  UE_LOG(LogTemp, Log, TEXT("ClearRenderedDungeon: Cleanup complete"));
}
