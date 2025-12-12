#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGeneratorSubsystem.h"
#include "Rendering/DungeonTileRenderer.h"
#include "Rendering/DungeonNavigationBuilder.h"
#include "Rendering/DungeonChunkStreamer.h"
#include "DungeonFullTestActor.generated.h"

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
    
    // 생성된 던전인지 여부 (레벨 저장 시 함께 저장됨)
    UPROPERTY(SaveGame)
    bool bWasGenerated = false;

    // 저장된 그리드 데이터 (레벨 저장 시 함께 저장됨)
    UPROPERTY(SaveGame)
    FDungeonGrid StoredGrid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    int32 Seed = 12345;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    int32 Width = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    int32 Height = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
    EDungeonAlgorithmType Algorithm = EDungeonAlgorithmType::BSP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core", meta = (ClampMin = "1", ClampMax = "5"))
    int32 CorridorWidth = 3; // 복도 폭

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
    bool bGenerateFloorUnderWalls = true; // 벽 아래에도 바닥 생성

    // --- LOD Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon LOD")
    bool bUseLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon LOD")
    TArray<float> LODDistances = { 5000.0f, 10000.0f, 20000.0f };

    // --- Asset Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Assets")
    TMap<uint8, UStaticMesh*> WallMeshTable;

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
    // 청크 단위 메시 머징 활성화 (컬링 효율 유지하면서 드로우콜 감소)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Mesh Merging")
    bool bEnableChunkMerging = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Mesh Merging")
    bool bRemoveOriginalAfterMerge = true;

    // --- Streaming Settings (Phase 4) ---
    // 카메라 기반 청크 스트리밍 활성화
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Streaming")
    bool bEnableChunkStreaming = false;

    // 스트리밍 거리 (청크 단위, 예: 3 = 7x7 청크 활성화)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Streaming", meta = (ClampMin = "1", ClampMax = "10", EditCondition = "bEnableChunkStreaming"))
    int32 StreamingDistance = 3;

    // 스트리밍 업데이트 주기 (초)
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

    // 동적 생성된 Wall HISM 추적용 (for cleanup)
    UPROPERTY(VisibleAnywhere, Transient, Category = "Generated")
    TArray<UHierarchicalInstancedStaticMeshComponent*> CreatedWallHISMs;

    // 청크별 HISM 맵 (ChunkCoord -> HISMs) - 스트리밍/머징용
    // Note: TMap<FIntPoint, TArray<...>>는 UPROPERTY 지원 안됨
    TMap<FIntPoint, TArray<UHierarchicalInstancedStaticMeshComponent*>> ChunkHISMMap;

    // 청크별 머지된 메시 맵 (ChunkCoord -> MergedMesh)
    UPROPERTY(VisibleAnywhere, Transient, Category = "Generated")
    TMap<FIntPoint, UDynamicMeshComponent*> ChunkMergedWallMap;

    // 청크 스트리머 컴포넌트
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UDungeonChunkStreamer* ChunkStreamer;

    // --- Editor Functions ---
    UFUNCTION(CallInEditor, Category = "Dungeon Action")
    void Generate();

    UFUNCTION(CallInEditor, Category = "Dungeon Action")
    void Clear();

protected:
    virtual void BeginPlay() override;

private:
    void RebuildChunkMaps();
};
