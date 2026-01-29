#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGeneratorSubsystem.h"
#include "Rendering/DungeonTileRenderer.h"
#include "Rendering/DungeonNavigationBuilder.h"
#include "Rendering/DungeonChunkStreamer.h"
#include "DungeonFullTestActor.generated.h"

class UPCGComponent;
class UPCGGraph;
class UPCGPointData; // Forward Decl



/**
 * Advanced Test Actor for Dungeon Generator Plugin
 * Exposes all features of UDungeonTileRenderer for comprehensive testing.
 */
UCLASS()
class DUNGEONGENERATOR_API ADungeonFullTestActor : public AActor {
    GENERATED_BODY()

public:
    ADungeonFullTestActor();

    // --- Level Save/Load Support ---
    virtual void PostLoad() override;
    
    // ?성???전?? ?? (?벨 ??????께 ??됨)
    UPROPERTY(SaveGame)
    bool bWasGenerated = false;

    // ??된 그리???이??(?벨 ??????께 ??됨)
    UPROPERTY(SaveGame)
    FDungeonGrid StoredGrid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    class UDungeonThemeAsset* DungeonTheme;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    int32 Seed = 12345;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    int32 Width = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    int32 Height = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    EDungeonAlgorithmType Algorithm = EDungeonAlgorithmType::BSP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core", meta = (ClampMin = "1", ClampMax = "5"))
    int32 CorridorWidth = 3; // 복도 ??

    // --- Renderer Settings Exposed ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Metrics")
    float TileSize = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Metrics")
    float WallHeight = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Offsets")
    FVector WallPivotOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Offsets")
    FVector FloorPivotOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Offsets")
    FVector CeilingPivotOffset;

    // --- Generation Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Generation")
    bool bGenerateCeiling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Generation")
    bool bGenerateFloor = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Generation")
    bool bGenerateFloorUnderWalls = true; // ??래?도 바닥 ?성

    // --- LOD Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon LOD")
    bool bUseLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon LOD")
    TArray<float> LODDistances = { 5000.0f, 10000.0f, 20000.0f };

    // --- Asset Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Assets")
    TMap<int32, UStaticMesh*> WallMeshTable;

    // --- Material Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Materials")
    bool bUseDynamicMaterials = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Materials", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetnessIntensity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Materials", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MossIntensity = 0.0f;

    // --- Chunking Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Chunking")
    bool bUseChunking = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Chunking", meta = (ClampMin = "5", ClampMax = "50"))
    int32 ChunkSize = 10;

    // --- Mesh Merging Settings (Phase 4: Chunk-based) ---
    // ? ?위 메시 머징 ?성??(컬링 ?율 ???면???로?콜 감소)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Mesh Merging")
    bool bEnableChunkMerging = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Mesh Merging")
    bool bRemoveOriginalAfterMerge = true;

    // --- Streaming Settings (Phase 4) ---
    // 카메??기반 ? ?트리밍 ?성??
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Streaming")
    bool bEnableChunkStreaming = false;

    // ?트리밍 거리 (? ?위, ?? 3 = 7x7 ? ?성??
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Streaming", meta = (ClampMin = "1", ClampMax = "10", EditCondition = "bEnableChunkStreaming"))
    int32 StreamingDistance = 3;

    // ?트리밍 ?데?트 주기 (?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Streaming", meta = (ClampMin = "0.1", ClampMax = "5.0", EditCondition = "bEnableChunkStreaming"))
    float StreamingUpdateInterval = 0.5f;

    // --- Components (HISM for auto-culling) ---
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UInstancedStaticMeshComponent* WallMesh; // Legacy, not used

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UHierarchicalInstancedStaticMeshComponent* CeilingMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UHierarchicalInstancedStaticMeshComponent* FloorMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UHierarchicalInstancedStaticMeshComponent* PropMesh;

    // --- Chunking & Streamer ---

    // ?적 ?성??Wall HISM 추적??(for cleanup)
    UPROPERTY(VisibleAnywhere, Transient, Category = "Generated")
    TArray<UHierarchicalInstancedStaticMeshComponent*> CreatedWallHISMs;

    // ??HISM ?(ChunkCoord -> HISMs) - ?트리밍/머징??
    // Note: TMap<FIntPoint, TArray<...>>??UPROPERTY 지???됨
    TMap<FIntPoint, TArray<UHierarchicalInstancedStaticMeshComponent*>> ChunkHISMMap;

    // ??머???메시 ?(ChunkCoord -> MergedMesh)
    UPROPERTY(VisibleAnywhere, Transient, Category = "Generated")
    TMap<FIntPoint, UDynamicMeshComponent*> ChunkMergedWallMap;

    // ? ?트리머 컴포?트
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UDungeonChunkStreamer* ChunkStreamer;

    // --- Editor Functions ---
    UFUNCTION(CallInEditor, Category = "Dungeon Action")
    void Generate();

    UFUNCTION(CallInEditor, Category = "Dungeon Action")
    void Clear();
    
    // Note: Landscape Generation moved to ADungeonWorldBuilder

protected:
    virtual void BeginPlay() override;

private:
    void RebuildChunkMaps();
};
