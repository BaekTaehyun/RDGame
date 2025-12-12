#include "DungeonFullTestActor.h"
#include "ObjectPlacer.h"
#include "Algorithms/BSPGenerator.h"
#include "Algorithms/CellularAutomataGenerator.h"
#include "Rendering/DungeonMeshMerger.h"

ADungeonFullTestActor::ADungeonFullTestActor() {
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    WallMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallMesh"));
    WallMesh->SetupAttachment(RootComponent);

    // HISM으로 생성 (자동 컬링/LOD)
    CeilingMesh = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CeilingMesh"));
    CeilingMesh->SetupAttachment(RootComponent);

    FloorMesh = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FloorMesh"));
    FloorMesh->SetupAttachment(RootComponent);

    PropMesh = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("PropMesh"));
    PropMesh->SetupAttachment(RootComponent);

    // Phase 4: 청크 스트리머
    ChunkStreamer = CreateDefaultSubobject<UDungeonChunkStreamer>(TEXT("ChunkStreamer"));
}

void ADungeonFullTestActor::Generate() {
    Clear();

    FDungeonGrid Grid;
    
    // Editor Generation Logic
    UE_LOG(LogTemp, Log, TEXT("DungeonFullTestActor: Generating Dungeon..."));
    
    UDungeonAlgorithm* Algo = nullptr;
    switch (Algorithm) {
    case EDungeonAlgorithmType::BSP:
        {
            UBSPGenerator* BSPAlgo = NewObject<UBSPGenerator>(this);
            BSPAlgo->CorridorWidth = CorridorWidth;
            Algo = BSPAlgo;
        }
        break;
    case EDungeonAlgorithmType::CellularAutomata:
        Algo = NewObject<UCellularAutomataGenerator>(this);
        break;
    }

    if (Algo) {
        Grid.Init(Width, Height, ETileType::Wall);
        FRandomStream RandomStream(Seed);
        Algo->Generate(Grid, RandomStream);
        
        // 레벨 저장용 그리드 저장
        StoredGrid = Grid;
        bWasGenerated = true;
    }

    // Configure Renderer
    UDungeonTileRenderer* Renderer = NewObject<UDungeonTileRenderer>(this);
    if (Renderer) {
        // Pass all properties down
        Renderer->TileSize = TileSize;
        Renderer->WallHeight = WallHeight;
        
        Renderer->WallPivotOffset = WallPivotOffset;
        Renderer->FloorPivotOffset = FloorPivotOffset;
        Renderer->CeilingPivotOffset = CeilingPivotOffset;

        Renderer->bGenerateCeiling = bGenerateCeiling;
        Renderer->bGenerateFloor = bGenerateFloor;
        Renderer->bGenerateFloorUnderWalls = bGenerateFloorUnderWalls;
        
        Renderer->bUseLOD = bUseLOD;
        Renderer->LODDistances = LODDistances;

        Renderer->bUseDynamicMaterials = bUseDynamicMaterials;
        Renderer->WetnessIntensity = WetnessIntensity;
        Renderer->MossIntensity = MossIntensity;

        // Chunking 설정
        Renderer->bUseChunking = bUseChunking;
        Renderer->ChunkSize = ChunkSize;

        // Mesh Setup - WallMeshTable is passed directly
        UE_LOG(LogTemp, Warning, TEXT("DungeonFullTestActor: WallMeshTable.Num() = %d"), WallMeshTable.Num());
        
        if (WallMeshTable.Num() > 0) {
            Renderer->WallMeshTable = WallMeshTable;
            UE_LOG(LogTemp, Warning, TEXT("DungeonFullTestActor: Using WallMeshTable with %d entries"), Renderer->WallMeshTable.Num());
            
            // 각 엔트리 로깅
            for (auto& Pair : Renderer->WallMeshTable) {
                UE_LOG(LogTemp, Warning, TEXT("  Mask %d: %s"), 
                    Pair.Key, 
                    Pair.Value ? *Pair.Value->GetName() : TEXT("NULL"));
            }
        } else if (WallMesh && WallMesh->GetStaticMesh()) {
            // Fallback to simple mode if table is empty
            Renderer->WallMeshTable.Add(0, WallMesh->GetStaticMesh());
            UE_LOG(LogTemp, Warning, TEXT("DungeonFullTestActor: Using WallMesh fallback"));
        } else {
            UE_LOG(LogTemp, Error, TEXT("DungeonFullTestActor: No wall meshes configured! WallMeshTable is empty and WallMesh has no mesh."));
        }

        if (FloorMesh && FloorMesh->GetStaticMesh()) {
            Renderer->FloorMesh = FloorMesh->GetStaticMesh();
        }
        if (CeilingMesh && CeilingMesh->GetStaticMesh()) {
            Renderer->CeilingMesh = CeilingMesh->GetStaticMesh();
        }

        // Execute Multi-Mesh Render (HISM 방식)
        Renderer->GenerateBSPTilesMultiMesh(Grid, this, CeilingMesh, FloorMesh, CreatedWallHISMs);
    }

    // Props (Simple Example)
    if (PropMesh && PropMesh->GetStaticMesh()) {
        FRandomStream RandomStream(Seed);
        TArray<FVector2D> PropPoints = UObjectPlacer::GeneratePoissonPoints(Grid, 5.0f, 100, RandomStream);
        for (const FVector2D& P : PropPoints) {
            FTransform Transform;
            Transform.SetLocation(FVector(P.X * TileSize, P.Y * TileSize, 50.0f) + FloorPivotOffset);
            PropMesh->AddInstance(Transform);
        }
        
        PropMesh->UpdateBounds();
        PropMesh->MarkRenderStateDirty();
        PropMesh->RecreatePhysicsState();
    }

    // NavMesh Rebuild
    UDungeonNavigationBuilder* NavBuilder = NewObject<UDungeonNavigationBuilder>(this);
    if (NavBuilder) {
        NavBuilder->TileSize = TileSize;
        NavBuilder->WallHeight = WallHeight;
        
        FBox NavBounds = NavBuilder->CalculateNavBounds(Grid, FloorPivotOffset);
        NavBuilder->TriggerNavMeshRebuild(GetWorld(), NavBounds);
    }



    // 청크 맵 및 스트리머 재구축
    RebuildChunkMaps();

    // Phase 4: 청크 단위 메시 머징 (옵션)
    if (bEnableChunkMerging && bUseChunking && ChunkHISMMap.Num() > 0) {
        ChunkMergedWallMap = UDungeonMeshMerger::MergeHISMsPerChunk(
            this, ChunkHISMMap, TEXT("MergedWall"));

        // 원본 HISM 제거 (옵션)
        if (bRemoveOriginalAfterMerge) {
            for (auto& ChunkPair : ChunkHISMMap) {
                for (auto* HISM : ChunkPair.Value) {
                    if (HISM) {
                        HISM->ClearInstances();
                        HISM->SetVisibility(false);
                    }
                }
            }
        }
        // 머지 후 스트리머 업데이트 필요
        if (ChunkStreamer) {
            ChunkStreamer->ChunkMergedMeshMap = ChunkMergedWallMap;
        }

        UE_LOG(LogTemp, Log, TEXT("DungeonFullTestActor: Chunk merging complete - %d merged meshes"), ChunkMergedWallMap.Num());
    }
}

void ADungeonFullTestActor::Clear() {
    // 동적 생성된 모든 Wall HISM들 제거 (청크 포함)
    TArray<UHierarchicalInstancedStaticMeshComponent*> Components;
    GetComponents<UHierarchicalInstancedStaticMeshComponent>(Components);
    
    for (UHierarchicalInstancedStaticMeshComponent* Comp : Components) {
        // Wall_ 또는 WallMesh_ 로 시작하는 동적 생성 컴포넌트만 제거
        FString CompName = Comp->GetName();
        if (CompName.StartsWith(TEXT("Wall_")) || CompName.StartsWith(TEXT("WallMesh_"))) {
            Comp->DestroyComponent();
        }
    }
    CreatedWallHISMs.Empty();

    // 저장 상태 리셋
    bWasGenerated = false;
    StoredGrid = FDungeonGrid();

    // 기본 컴포넌트 정리
    if (WallMesh) WallMesh->ClearInstances();
    if (CeilingMesh) {
        CeilingMesh->ClearInstances();
        CeilingMesh->SetVisibility(true);  // 머징 후 숨겨진 경우 복원
    }
    if (FloorMesh) {
        FloorMesh->ClearInstances();
        FloorMesh->SetVisibility(true);
    }
    if (PropMesh) PropMesh->ClearInstances();

    // Phase 4: 청크별 머지된 메시 정리
    for (auto& Pair : ChunkMergedWallMap) {
        if (Pair.Value) {
            Pair.Value->DestroyComponent();
        }
    }
    ChunkMergedWallMap.Empty();
    ChunkHISMMap.Empty();

    // 스트리머 데이터 초기화
    if (ChunkStreamer) {
        ChunkStreamer->ClearChunkData();
    }
}

void ADungeonFullTestActor::PostLoad() {
    Super::PostLoad();
    
    // HISM 인스턴스 데이터는 직렬화되지 않으므로, StoredGrid로 재생성해야 함
    // Wall_C 컴포넌트는 AddInstanceComponent가 호출되지 않아 저장되지 않음
    if (bWasGenerated && StoredGrid.Width > 0 && StoredGrid.Height > 0) {
        
        // 1. Check if Wall_C components already exist (e.g. via serialization or PIE duplication)
        TArray<UHierarchicalInstancedStaticMeshComponent*> ExistingHISMs;
        GetComponents<UHierarchicalInstancedStaticMeshComponent>(ExistingHISMs);
        
        int32 ValidWallCount = 0;
        for (UHierarchicalInstancedStaticMeshComponent* HISM : ExistingHISMs) {
            if (HISM && HISM->GetName().StartsWith(TEXT("Wall_C")) && HISM->GetInstanceCount() > 0) {
                ValidWallCount++;
            }
        }
        
        // If we found valid walls, we assume the engine successfully serialized or duplicated them.
        // We SKIP regeneration to avoid destroying valid components, which causes issues in PIE.
        // BUT: In Editor Load (not PIE), we want to regenerate to ensure consistency with StoredGrid if stuff is missing.
        bool bIsPIE = (GetPackage()->HasAnyPackageFlags(PKG_PlayInEditor));
        
        if (bIsPIE && ValidWallCount > 0) {
             UE_LOG(LogTemp, Log, TEXT("ADungeonFullTestActor::PostLoad - Found %d existing Wall HISMs (Instances: %d) in PIE. Skipping regeneration."), 
                ValidWallCount, ExistingHISMs[0]->GetInstanceCount());
             RebuildChunkMaps();
             return;
        }

        UE_LOG(LogTemp, Log, TEXT("ADungeonFullTestActor::PostLoad - Regenerating dungeon from stored grid (%dx%d)"), 
            StoredGrid.Width, StoredGrid.Height);
        
        // 1. 기존 Wall_C* 컴포넌트들 제거 (좀비 객체 정리)
        // 이전에 저장된 잘못된 데이터나 충돌을 일으키는 컴포넌트를 모두 삭제
        TArray<UHierarchicalInstancedStaticMeshComponent*> AllHISMs;
        GetComponents<UHierarchicalInstancedStaticMeshComponent>(AllHISMs);
        
        for (UHierarchicalInstancedStaticMeshComponent* HISM : AllHISMs) {
            FString CompName = HISM->GetName();
            if (CompName.StartsWith(TEXT("Wall_C"))) {
                // Rename to avoid name collision with new components
                HISM->Rename(*FString::Printf(TEXT("TRASH_%s"), *CompName), nullptr, REN_DontCreateRedirectors);

                if (HISM->IsRegistered()) {
                    HISM->UnregisterComponent();
                }
                HISM->DestroyComponent();
            }
        }
        
        // 맵 초기화
        CreatedWallHISMs.Empty();
        ChunkHISMMap.Empty();
        
        // 1.5 Clean up potential Zombie Merged Meshes (UDynamicMeshComponent)
        // These might be leftovers from previous "EnableChunkMerging" sessions that are not managed by HISMs
        TArray<UActorComponent*> AllComps;
        GetComponents(UActorComponent::StaticClass(), AllComps);
        for (UActorComponent* Comp : AllComps) {
            if (Comp && (Comp->GetName().StartsWith(TEXT("MergedWall")) || Comp->GetName().StartsWith(TEXT("TRASH_MergedWall")))) {
                 Comp->DestroyComponent();
            }
        }

        // 2. 렌더러로 재생성
        UDungeonTileRenderer* Renderer = NewObject<UDungeonTileRenderer>(this);
        if (Renderer) {
            Renderer->TileSize = TileSize;
            Renderer->WallHeight = WallHeight;
            Renderer->WallPivotOffset = WallPivotOffset;
            Renderer->FloorPivotOffset = FloorPivotOffset;
            Renderer->CeilingPivotOffset = CeilingPivotOffset;
            Renderer->bGenerateCeiling = bGenerateCeiling;
            Renderer->bGenerateFloor = bGenerateFloor;
            Renderer->bGenerateFloorUnderWalls = bGenerateFloorUnderWalls;
            Renderer->bUseLOD = bUseLOD;
            Renderer->LODDistances = LODDistances;
            Renderer->bUseDynamicMaterials = bUseDynamicMaterials;
            Renderer->WetnessIntensity = WetnessIntensity;
            Renderer->MossIntensity = MossIntensity;
            Renderer->bUseChunking = bUseChunking;
            Renderer->ChunkSize = ChunkSize;
            
            if (WallMeshTable.Num() > 0) {
                Renderer->WallMeshTable = WallMeshTable;
            }
            if (FloorMesh && FloorMesh->GetStaticMesh()) {
                Renderer->FloorMesh = FloorMesh->GetStaticMesh();
            }
            if (CeilingMesh && CeilingMesh->GetStaticMesh()) {
                Renderer->CeilingMesh = CeilingMesh->GetStaticMesh();
            }
            
            Renderer->GenerateBSPTilesMultiMesh(StoredGrid, this, CeilingMesh, FloorMesh, CreatedWallHISMs);
            
            UE_LOG(LogTemp, Log, TEXT("ADungeonFullTestActor::PostLoad - Regenerated %d wall HISMs"), CreatedWallHISMs.Num());
            
            // 3. 청크 맵 및 스트리머 재구축
            RebuildChunkMaps();
        }
    }
}
void ADungeonFullTestActor::BeginPlay() {
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] DungeonFullTestActor::BeginPlay"));
    UE_LOG(LogTemp, Warning, TEXT("   - Actor Location: %s"), *GetActorLocation().ToString());
    UE_LOG(LogTemp, Warning, TEXT("   - TileSize: %f, ChunkSize: %d"), TileSize, ChunkSize);

    if (UWorld* World = GetWorld()) {
        if (APlayerController* PC = World->GetFirstPlayerController()) {
            if (APawn* Pawn = PC->GetPawn()) {
                UE_LOG(LogTemp, Warning, TEXT("   - Player Pawn Location: %s"), *Pawn->GetActorLocation().ToString());
            } else {
                UE_LOG(LogTemp, Warning, TEXT("   - Player Pawn is NULL"));
            }
        } else {
             UE_LOG(LogTemp, Warning, TEXT("   - Player Controller is NULL"));
        }
    }

    // PIE 시작 시 청크 맵 복구
    RebuildChunkMaps();
    
    // 스트리밍 시작
    if (ChunkStreamer && bEnableChunkStreaming) {
        ChunkStreamer->UpdateActiveChunks(GetActorLocation());
    }
}

void ADungeonFullTestActor::RebuildChunkMaps() {
    if (!bUseChunking) return;
    
    ChunkHISMMap.Empty();
    CreatedWallHISMs.Empty(); // 다시 채움
    
    TArray<UHierarchicalInstancedStaticMeshComponent*> AllWallHISMs;
    GetComponents<UHierarchicalInstancedStaticMeshComponent>(AllWallHISMs);
    
    int32 FoundChunks = 0;
    
    for (UHierarchicalInstancedStaticMeshComponent* HISM : AllWallHISMs) {
        if (!IsValid(HISM)) continue;

        FString CompName = HISM->GetName();
        // Wall_C{X}_{Y}_M{Mask} 형식 파싱
        if (CompName.StartsWith(TEXT("Wall_C"))) {
            // 로드된 HISM 가시성 보장 (기본값)
            if (bEnableChunkStreaming && ChunkStreamer) {
                // 스트리밍 켜져있으면 일단 숨기고 시작할 수도 있지만,
                // 스트리머가 알아서 처리하도록 둠.
                // 하지만 안전을 위해 Visible로 둠.
                HISM->SetVisibility(true, true);
                HISM->SetHiddenInGame(false, true);
                
                // Force Collision Update in PIE
                HISM->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
                HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                HISM->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
                HISM->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
                
                // Reregister to ensure physics state is created directly
                HISM->ReregisterComponent(); 

                // Force BVH Rebuild for collision
                HISM->BuildTreeIfOutdated(true, true);
                
                HISM->MarkRenderStateDirty(); // Force update

                // Fallback: If StaticMesh has no Simple Collision (AggGeom), force use of Complex as Simple
                if (UStaticMesh* Mesh = HISM->GetStaticMesh()) {
                    if (UBodySetup* BodySetup = Mesh->GetBodySetup()) {
                        if (BodySetup->AggGeom.GetElementCount() == 0 && BodySetup->CollisionTraceFlag != CTF_UseComplexAsSimple) {
                            UE_LOG(LogTemp, Warning, TEXT("[DungeonFullTestActor] Mesh %s has no Simple Collision. Forcing 'UseComplexAsSimple' for PIE."), *Mesh->GetName());
                            BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
                            // Note: Modifying Asset BodySetup in PIE is temporary but ensures collision works
                        }
                    }
                }
            }

            int32 CIndex = CompName.Find(TEXT("_C")) + 2;
            int32 UnderscoreIndex = CompName.Find(TEXT("_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, CIndex);
            int32 MIndex = CompName.Find(TEXT("_M"));
            
            if (UnderscoreIndex != INDEX_NONE && MIndex != INDEX_NONE) {
                FString XStr = CompName.Mid(CIndex, UnderscoreIndex - CIndex);
                FString YStr = CompName.Mid(UnderscoreIndex + 1, MIndex - UnderscoreIndex - 1);
                
                int32 ChunkX = FCString::Atoi(*XStr);
                int32 ChunkY = FCString::Atoi(*YStr);
                FIntPoint ChunkCoord(ChunkX, ChunkY);
                
                ChunkHISMMap.FindOrAdd(ChunkCoord).Add(HISM);
                
                if (!CreatedWallHISMs.Contains(HISM)) {
                    CreatedWallHISMs.Add(HISM);
                }
                
                FoundChunks++;

                // DEBUG: Log details for the first few chunks to verify PIE state
                if (FoundChunks <= 5) {
                    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] Found Wall HISM: %s"), *CompName);
                }
            }
        }
    }
    
           
    
    // 2. Rebuild ChunkMergedWallMap (Scan for existing Merged Meshes)
    ChunkMergedWallMap.Empty();
    TArray<UDynamicMeshComponent*> AllMergedMeshes;
    GetComponents<UDynamicMeshComponent>(AllMergedMeshes);
    
    int32 FoundMerged = 0;
    for (UDynamicMeshComponent* Mesh : AllMergedMeshes)
    {
         if (!Mesh) continue;
         FString CompName = Mesh->GetName();
         
         // Pattern: MergedWall_C{X}_{Y}
         if (CompName.StartsWith(TEXT("MergedWall_C")))
         {
             // Force visibility settings for streaming control
             if (bEnableChunkStreaming && ChunkStreamer) {
                 Mesh->SetVisibility(true, true);
             }
             
             // Parse Coord
             int32 CIndex = CompName.Find(TEXT("_C")) + 2;
             int32 UnderscoreIndex = CompName.Find(TEXT("_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, CIndex);
             
             if (UnderscoreIndex != INDEX_NONE)
             {
                 FString XStr = CompName.Mid(CIndex, UnderscoreIndex - CIndex);
                 FString YStr = CompName.Mid(UnderscoreIndex + 1);
                 
                 int32 ChunkX = FCString::Atoi(*XStr);
                 int32 ChunkY = FCString::Atoi(*YStr);
                 FIntPoint ChunkCoord(ChunkX, ChunkY);
                 
                 ChunkMergedWallMap.Add(ChunkCoord, Mesh);
                 FoundMerged++;
             }
         }
    }

    UE_LOG(LogTemp, Log, TEXT("DungeonFullTestActor: Rebuilt chunk map with %d HISM chunks and %d Merged meshes"), 
        ChunkHISMMap.Num(), FoundMerged);
        
    // 스트리머 설정 업데이트
    if (ChunkStreamer) {
        ChunkStreamer->bEnableStreaming = bEnableChunkStreaming;
        ChunkStreamer->StreamingDistance = StreamingDistance;
        ChunkStreamer->UpdateInterval = StreamingUpdateInterval;
        ChunkStreamer->TileSize = TileSize;
        ChunkStreamer->ChunkSize = ChunkSize;
        ChunkStreamer->ChunkHISMMap = ChunkHISMMap;
        ChunkStreamer->ChunkMergedMeshMap = ChunkMergedWallMap; // MergedMap은 아직 복구 안함(나중에 추가 필요)
        
        if (!bEnableChunkStreaming) {
            ChunkStreamer->ShowAllChunks();
        }
    }
}
