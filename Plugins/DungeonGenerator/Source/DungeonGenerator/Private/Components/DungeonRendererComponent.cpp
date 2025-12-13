#include "Components/DungeonRendererComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Data/DungeonThemeAsset.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodySetup.h"
#include "Rendering/DungeonChunkStreamer.h"
#include "Rendering/DungeonMeshMerger.h"


UDungeonRendererComponent::UDungeonRendererComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

void UDungeonRendererComponent::BeginPlay() {
  Super::BeginPlay();
  // Note: Streaming initialization is typically handled by the Actor or
  // Streamer Component
}

void UDungeonRendererComponent::GenerateDungeon(
    const FDungeonGrid &Grid, const FDungeonGenConfig &Config,
    const UDungeonThemeAsset *Theme, UDungeonChunkStreamer *ChunkStreamer) {
  if (!Theme) {
    UE_LOG(LogTemp, Error,
           TEXT("UDungeonRendererComponent: Missing Theme Asset!"));
    return;
  }

  // Always clear previous generation to avoid duplicates (Zombies)
  ClearDungeon();

  AActor *Owner = GetOwner();
  if (!Owner)
    return;

  // Ensure Renderer exists
  if (!TileRenderer) {
    TileRenderer = NewObject<UDungeonTileRenderer>(this);
  }

  // Apply Theme
  TileRenderer->ApplyTheme(Theme);

  // Set ChunkSize from Config (Used for grouping)
  TileRenderer->ChunkSize = Config.ChunkSize;
  TileRenderer->bUseChunking = Config.bUseChunking;

  // Ensure Floor/Ceiling HISMs exist
  if (!FloorHISM) {
    FloorHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
        Owner, TEXT("FloorHISM"));
    FloorHISM->SetupAttachment(Owner->GetRootComponent());
    // Navigation 비활성화 (StaticMesh 없이 Register 시 크래시 방지)
    FloorHISM->SetCanEverAffectNavigation(false);
    FloorHISM->RegisterComponent();
  } else {
    FloorHISM->ClearInstances();
  }

  if (!CeilingHISM) {
    CeilingHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
        Owner, TEXT("CeilingHISM"));
    CeilingHISM->SetupAttachment(Owner->GetRootComponent());
    // Navigation 비활성화 (StaticMesh 없이 Register 시 크래시 방지)
    CeilingHISM->SetCanEverAffectNavigation(false);
    CeilingHISM->RegisterComponent();
  } else {
    CeilingHISM->ClearInstances();
  }

  // Generate Meshes (HISMs)
  TileRenderer->GenerateBSPTilesMultiMesh(Grid, Owner, CeilingHISM, FloorHISM,
                                          CreatedWallHISMs);

  // Populate ChunkHISMMap from CreatedWallHISMs directly
  ChunkHISMMap.Empty();
  for (UHierarchicalInstancedStaticMeshComponent *HISM : CreatedWallHISMs) {
    if (IsValid(HISM)) {
      // Parse Tags for Chunk Coord (Faster than name parsing)
      // Assuming GenerateBSPTilesMultiMesh already added tags
      int32 ChunkX = 0;
      int32 ChunkY = 0;
      bool bFound = false;

      for (const FName &Tag : HISM->ComponentTags) {
        FString TagStr = Tag.ToString();
        if (TagStr.StartsWith(TEXT("ChunkX:"))) {
          ChunkX = FCString::Atoi(*TagStr.Mid(7));
          bFound = true;
        } else if (TagStr.StartsWith(TEXT("ChunkY:"))) {
          ChunkY = FCString::Atoi(*TagStr.Mid(7));
          bFound = true;
        }
      }

      if (bFound) {
        ChunkHISMMap.FindOrAdd(FIntPoint(ChunkX, ChunkY)).Add(HISM);
      }
    }
  }

  // Mesh Merging
  if (Config.bEnableChunkMerging) {
    // Merge HISMs per chunk
    MergedChunkMeshes = UDungeonMeshMerger::MergeHISMsPerChunk(
        Owner, ChunkHISMMap, TEXT("MergedDungeon"));

    if (Config.bRemoveOriginalAfterMerge) {
      // Destroy original HISMs
      for (UHierarchicalInstancedStaticMeshComponent *HISM : CreatedWallHISMs) {
        if (IsValid(HISM)) {
          HISM->DestroyComponent();
        }
      }
      CreatedWallHISMs.Empty();
      ChunkHISMMap.Empty();
    }
  }

  // Update Streamer if provided
  if (ChunkStreamer) {
    ChunkStreamer->ChunkHISMMap = ChunkHISMMap;
    ChunkStreamer->ChunkMergedMeshMap = MergedChunkMeshes;

    // Force Show All initially
    ChunkStreamer->ShowAllChunks();
  }
}

void UDungeonRendererComponent::ClearDungeon() {
  // Destroy all managed HISMs
  for (UHierarchicalInstancedStaticMeshComponent *HISM : CreatedWallHISMs) {
    if (IsValid(HISM)) {
      HISM->DestroyComponent();
    }
  }
  CreatedWallHISMs.Empty();
  ChunkHISMMap.Empty();

  // Destroy Merged Meshes
  for (auto &Pair : MergedChunkMeshes) {
    if (Pair.Value) {
      Pair.Value->DestroyComponent();
    }
  }
  MergedChunkMeshes.Empty();
}

void UDungeonRendererComponent::RebuildChunkMaps() {
  ChunkHISMMap.Empty();
  CreatedWallHISMs.Empty();

  AActor *Owner = GetOwner();
  if (!Owner)
    return;

  TArray<UHierarchicalInstancedStaticMeshComponent *> AllHISMs;
  Owner->GetComponents<UHierarchicalInstancedStaticMeshComponent>(AllHISMs);

  int32 FoundChunks = 0;

  for (UHierarchicalInstancedStaticMeshComponent *HISM : AllHISMs) {
    if (!IsValid(HISM))
      continue;

    FString CompName = HISM->GetName();
    // Parse "Wall_C{X}_{Y}"
    if (CompName.StartsWith(TEXT("Wall_C"))) {
      // Ensure Visible
      HISM->SetVisibility(true, true);
      HISM->SetHiddenInGame(false, true);

      // Optimization: Try parsing Tags first
      bool bFoundTags = false;
      int32 ChunkX = 0;
      int32 ChunkY = 0;

      for (const FName &Tag : HISM->ComponentTags) {
        FString TagStr = Tag.ToString();
        if (TagStr.StartsWith(TEXT("ChunkX:"))) {
          ChunkX = FCString::Atoi(*TagStr.Mid(7));
          bFoundTags = true;
        } else if (TagStr.StartsWith(TEXT("ChunkY:"))) {
          ChunkY = FCString::Atoi(*TagStr.Mid(7));
          bFoundTags = true;
        }
      }

      bool bIsDungeonComp = false;

      if (bFoundTags) {
        // Tags found, use them
        FIntPoint ChunkCoord(ChunkX, ChunkY);
        ChunkHISMMap.FindOrAdd(ChunkCoord).Add(HISM);
        bIsDungeonComp = true;
        FoundChunks++;
      } else {
        // Fallback: Parse Name (Legacy)
        int32 CIndex = CompName.Find(TEXT("_C")) + 2;
        int32 UnderscoreIndex = CompName.Find(
            TEXT("_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, CIndex);
        int32 MIndex = CompName.Find(TEXT("_M"));

        if (UnderscoreIndex != INDEX_NONE && MIndex != INDEX_NONE) {
          FString XStr = CompName.Mid(CIndex, UnderscoreIndex - CIndex);
          FString YStr =
              CompName.Mid(UnderscoreIndex + 1, MIndex - UnderscoreIndex - 1);

          ChunkX = FCString::Atoi(*XStr);
          ChunkY = FCString::Atoi(*YStr);
          FIntPoint ChunkCoord(ChunkX, ChunkY);

          ChunkHISMMap.FindOrAdd(ChunkCoord).Add(HISM);
          bIsDungeonComp = true;
          FoundChunks++;
        }
      }

      if (bIsDungeonComp) {
        CreatedWallHISMs.Add(HISM);
      }

      // Force Collision Update (PIE Hotfix logic migrated)
      // Note: This matches the logic in DungeonFullTestActor::RebuildChunkMaps
      if (HISM->GetWorld()->IsPlayInEditor()) {
        HISM->SetCollisionProfileName(
            UCollisionProfile::BlockAllDynamic_ProfileName);
        HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        HISM->ReregisterComponent();
        HISM->BuildTreeIfOutdated(true, true);

        // Fallback for missing simple collision
        if (UStaticMesh *Mesh = HISM->GetStaticMesh()) {
          if (UBodySetup *BodySetup = Mesh->GetBodySetup()) {
            if (BodySetup->AggGeom.GetElementCount() == 0 &&
                BodySetup->CollisionTraceFlag != CTF_UseComplexAsSimple) {
              BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
            }
          }
        }
      }
    }
  }

  UE_LOG(LogTemp, Log,
         TEXT("UDungeonRendererComponent: Rebuilt map with %d chunks."),
         ChunkHISMMap.Num());
}

void UDungeonRendererComponent::HandlePostLoad(bool bIsPIE) {
  AActor *Owner = GetOwner();
  if (!Owner)
    return;

  // cleanup Zombie components
  TArray<UHierarchicalInstancedStaticMeshComponent *> AllHISMs;
  Owner->GetComponents<UHierarchicalInstancedStaticMeshComponent>(AllHISMs);

  int32 ValidWallCount = 0;
  for (UHierarchicalInstancedStaticMeshComponent *HISM : AllHISMs) {
    FString CompName = HISM->GetName();
    if (CompName.StartsWith(TEXT("Wall_C")) && HISM->GetInstanceCount() > 0) {
      ValidWallCount++;
    } else if (CompName.StartsWith(TEXT("TRASH_"))) {
      HISM->DestroyComponent(); // Hard kill
    }
  }

  // PIE Optimization: If walls exist, Skip Regen & Just Rebuild Map
  if (bIsPIE && ValidWallCount > 0) {
    UE_LOG(LogTemp, Log,
           TEXT("UDungeonRendererComponent: Found existing walls in PIE. "
                "Skipping regeneration."));
    RebuildChunkMaps();
    return;
  }

  // Editor or Missing Walls: Do nothing here?
  // Use Case: If logic says "Regenerate", the Actor should call
  // GenerateDungeon. HandlePostLoad should mainly ensure clean state or restore
  // maps if NOT regenerating. If we ARE regenerating, GenerateDungeon will
  // clear and build. The previous logic forced regeneration in Editor if
  // bWasGenerated was true. Here, the Component doesn't know "bWasGenerated"
  // (Actor state). So HandlePostLoad is just for Cleanup and Map Rebuild. The
  // Actor must decide whether to call GenerateDungeon.
}
