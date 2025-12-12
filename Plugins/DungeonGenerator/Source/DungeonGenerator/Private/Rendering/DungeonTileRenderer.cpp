#include "Rendering/DungeonTileRenderer.h"
#include "Rendering/DungeonChunkStreamer.h" 
#include "Data/DungeonThemeAsset.h"
#include "Materials/MaterialInstanceDynamic.h"

UDungeonTileRenderer::UDungeonTileRenderer() {
    TileSize = 100.0f;
    WallHeight = 300.0f;
    WallPivotOffset = FVector(0.0f, 0.0f, 0.0f);
    FloorPivotOffset = FVector(0.0f, 0.0f, 0.0f);
    CeilingPivotOffset = FVector(0.0f, 0.0f, 0.0f);
    bGenerateCeiling = true;
    bGenerateFloor = true;
}

void UDungeonTileRenderer::GenerateBSPTiles(
    const FDungeonGrid& Grid, UInstancedStaticMeshComponent* WallISMC,
    UInstancedStaticMeshComponent* CeilingISMC,
    UInstancedStaticMeshComponent* FloorISMC) {
    if (!WallISMC) {
        UE_LOG(LogTemp, Error, TEXT("DungeonTileRenderer: WallISMC is null"));
        return;
    }

    // 기존 인스턴스 제거
    WallISMC->ClearInstances();
    if (CeilingISMC)
        CeilingISMC->ClearInstances();
    if (FloorISMC)
        FloorISMC->ClearInstances();

    // 벽 타일 생성
    TArray<FTransform> WallTransforms;
    WallTransforms.Reserve(Grid.Width * Grid.Height / 4); // 대략적인 예약

    // 디버그: 비트마스크 분포 카운트
    TMap<uint8, int32> MaskCounts;

    for (int32 Y = 0; Y < Grid.Height; Y++) {
        for (int32 X = 0; X < Grid.Width; X++) {
            const FDungeonTile& Tile = Grid.GetTile(X, Y);

            if (Tile.Type == ETileType::Wall) {
                // 비트마스크 계산
                uint8 Mask = GetWallBitmask(X, Y, Grid);

                // 디버그: 카운트 증가
                MaskCounts.FindOrAdd(Mask)++;

                // 메시 룩업
                UStaticMesh** MeshPtr = WallMeshTable.Find(Mask);
                if (MeshPtr && *MeshPtr) {
                    // 메시가 ISMC에 설정되지 않았다면 설정
                    if (WallISMC->GetStaticMesh() == nullptr) {
                        WallISMC->SetStaticMesh(*MeshPtr);
                    }

                    // 트랜스폼 계산
                    FTransform Transform;
                    Transform.SetLocation(FVector(X * TileSize, Y * TileSize, 0.0f) + WallPivotOffset);
                    Transform.SetRotation(FQuat::Identity);
                    Transform.SetScale3D(FVector::OneVector);

                    // 인스턴스 목록에 추가
                    WallTransforms.Add(Transform);
                }
                else {
                    // 기본 벽 메시 사용 (Mask 0)
                    UStaticMesh** DefaultMesh = WallMeshTable.Find(0);
                    if (DefaultMesh && *DefaultMesh) {
                        if (WallISMC->GetStaticMesh() == nullptr) {
                            WallISMC->SetStaticMesh(*DefaultMesh);
                        }

                        FTransform Transform;
                        Transform.SetLocation(FVector(X * TileSize, Y * TileSize, 0.0f) + WallPivotOffset);
                        WallTransforms.Add(Transform);
                    }
                }
            }
        }
    }

    // 디버그: 비트마스크 분포 로그 출력
    UE_LOG(LogTemp, Warning, TEXT("=== DungeonTileRenderer: Bitmask Distribution ==="));
    for (auto& Pair : MaskCounts) {
        UE_LOG(LogTemp, Warning, TEXT("  Mask %d (Binary: %d%d%d%d): %d tiles"), 
            Pair.Key, 
            (Pair.Key >> 3) & 1, (Pair.Key >> 2) & 1, (Pair.Key >> 1) & 1, Pair.Key & 1,
            Pair.Value);
    }
    UE_LOG(LogTemp, Warning, TEXT("==============================================="));

    // 일괄 추가 (NavMesh 업데이트 부하 감소)
    if (WallTransforms.Num() > 0) {
        WallISMC->AddInstances(WallTransforms, false);
    }

    // 천장 생성 (ISMC 버전 - 레거시)
    if (bGenerateCeiling && CeilingISMC) {
        if (UHierarchicalInstancedStaticMeshComponent* CeilingHISM = Cast<UHierarchicalInstancedStaticMeshComponent>(CeilingISMC)) {
            GenerateCeiling(Grid, CeilingHISM);
        } else {
            // ISMC 인라인 처리
            if (CeilingMesh) {
                CeilingISMC->SetStaticMesh(CeilingMesh);
                TArray<FTransform> CeilingTransforms;
                for (int32 Y = 0; Y < Grid.Height; Y++) {
                    for (int32 X = 0; X < Grid.Width; X++) {
                        const ETileType Type = Grid.GetTile(X, Y).Type;
                        if (Type == ETileType::Floor || Type == ETileType::Corridor) {
                            FTransform Transform;
                            Transform.SetLocation(FVector(X * TileSize, Y * TileSize, WallHeight) + CeilingPivotOffset);
                            CeilingTransforms.Add(Transform);
                        }
                    }
                }
                if (CeilingTransforms.Num() > 0) {
                    CeilingISMC->AddInstances(CeilingTransforms, false);
                }
            }
        }
    }

    // 바닥 생성 (ISMC 버전 - 레거시)
    if (bGenerateFloor && FloorISMC) {
        if (UHierarchicalInstancedStaticMeshComponent* FloorHISM = Cast<UHierarchicalInstancedStaticMeshComponent>(FloorISMC)) {
            GenerateFloor(Grid, FloorHISM);
        } else {
            // ISMC 인라인 처리
            if (FloorMesh) {
                FloorISMC->SetStaticMesh(FloorMesh);
                TArray<FTransform> FloorTransforms;
                for (int32 Y = 0; Y < Grid.Height; Y++) {
                    for (int32 X = 0; X < Grid.Width; X++) {
                        const ETileType Type = Grid.GetTile(X, Y).Type;
                        bool bShouldGenerate = (Type == ETileType::Floor || Type == ETileType::Corridor);
                        if (bGenerateFloorUnderWalls && Type == ETileType::Wall) {
                            bShouldGenerate = true;
                        }
                        if (bShouldGenerate) {
                            FTransform Transform;
                            Transform.SetLocation(FVector(X * TileSize, Y * TileSize, 0.0f) + FloorPivotOffset);
                            FloorTransforms.Add(Transform);
                        }
                    }
                }
                if (FloorTransforms.Num() > 0) {
                    FloorISMC->AddInstances(FloorTransforms, false);
                }
            }
        }
    }

    // LOD 설정 (ISMC는 기본 LOD 사용)
    if (bUseLOD) {
        float MaxDrawDistance = LODDistances.Num() > 0 ? LODDistances.Last() : 20000.0f;
        WallISMC->SetCachedMaxDrawDistance(MaxDrawDistance);
        if (CeilingISMC)
            CeilingISMC->SetCachedMaxDrawDistance(MaxDrawDistance);
        if (FloorISMC)
            FloorISMC->SetCachedMaxDrawDistance(MaxDrawDistance);
    }

    // 동적 머티리얼 적용 (ISMC 버전)
    if (bUseDynamicMaterials) {
        auto ApplyMaterialISMC = [this](UInstancedStaticMeshComponent* ISMC) {
            if (!ISMC) return;
            UMaterialInterface* CurrentMaterial = ISMC->GetMaterial(0);
            if (!CurrentMaterial) return;
            UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(CurrentMaterial, ISMC);
            if (DynMaterial) {
                DynMaterial->SetScalarParameterValue(FName("Wetness"), WetnessIntensity);
                DynMaterial->SetScalarParameterValue(FName("Moss"), MossIntensity);
                DynMaterial->SetScalarParameterValue(FName("RandomSeed"), FMath::FRand());
                ISMC->SetMaterial(0, DynMaterial);
            }
        };
        ApplyMaterialISMC(WallISMC);
        ApplyMaterialISMC(CeilingISMC);
        ApplyMaterialISMC(FloorISMC);
    }

    UE_LOG(LogTemp, Log, TEXT("DungeonTileRenderer: Generated %d wall instances (Legacy ISMC)"),
        WallISMC->GetInstanceCount());
}

void UDungeonTileRenderer::GenerateBSPTilesMultiMesh(
    const FDungeonGrid& Grid,
    AActor* OwnerActor,
    UHierarchicalInstancedStaticMeshComponent* CeilingHISM,
    UHierarchicalInstancedStaticMeshComponent* FloorHISM,
    TArray<UHierarchicalInstancedStaticMeshComponent*>& OutCreatedHISMs)
{
    if (!OwnerActor) {
        UE_LOG(LogTemp, Error, TEXT("DungeonTileRenderer: OwnerActor is null"));
        return;
    }

    // 기존 HISM 정리
    OutCreatedHISMs.Empty();
    if (CeilingHISM) CeilingHISM->ClearInstances();
    if (FloorHISM) FloorHISM->ClearInstances();

    // 청크 좌표 계산용 헬퍼 람다
    auto GetChunkCoord = [this](int32 X, int32 Y) -> FIntPoint {
        return FIntPoint(X / ChunkSize, Y / ChunkSize);
    };

    // 청크별 + 마스크별 트랜스폼 맵
    // Key: ChunkCoord, Value: (Mask -> Transforms)
    TMap<FIntPoint, TMap<uint8, TArray<FTransform>>> ChunkMaskTransforms;

    // 디버그: 비트마스크 분포 카운트
    TMap<uint8, int32> MaskCounts;

    // 1단계: 모든 벽 타일의 청크/마스크/트랜스폼 수집
    for (int32 Y = 0; Y < Grid.Height; Y++) {
        for (int32 X = 0; X < Grid.Width; X++) {
            const FDungeonTile& Tile = Grid.GetTile(X, Y);

            if (Tile.Type == ETileType::Wall) {
                uint8 Mask = GetWallBitmask(X, Y, Grid);
                MaskCounts.FindOrAdd(Mask)++;

                FIntPoint ChunkCoord = bUseChunking ? GetChunkCoord(X, Y) : FIntPoint(0, 0);

                // 해당 마스크에 메시가 있는 경우만 처리
                UStaticMesh** MeshPtr = WallMeshTable.Find(Mask);
                uint8 ActualMask = Mask;
                
                if (!MeshPtr || !*MeshPtr) {
                    // 폴백: 마스크 0 사용
                    ActualMask = 0;
                    UE_LOG(LogTemp, Warning, TEXT("DungeonTileRenderer: Mask %d has no mesh, falling back to 0 at (%d, %d)"), 
                        Mask, X, Y);
                }
                
                UStaticMesh** FinalMeshPtr = WallMeshTable.Find(ActualMask);
                if (FinalMeshPtr && *FinalMeshPtr) {
                    FTransform Transform;
                    Transform.SetLocation(FVector(X * TileSize, Y * TileSize, 0.0f) + WallPivotOffset);
                    Transform.SetRotation(FQuat::Identity);
                    Transform.SetScale3D(FVector::OneVector);

                    ChunkMaskTransforms.FindOrAdd(ChunkCoord).FindOrAdd(ActualMask).Add(Transform);
                }
            }
        }
    }

    // 디버그: 비트마스크 분포 로그 출력
    UE_LOG(LogTemp, Warning, TEXT("=== DungeonTileRenderer (MultiMesh): Bitmask Distribution ==="));
    for (auto& Pair : MaskCounts) {
        UE_LOG(LogTemp, Warning, TEXT("  Mask %d (Binary: %d%d%d%d): %d tiles"), 
            Pair.Key, 
            (Pair.Key >> 3) & 1, (Pair.Key >> 2) & 1, (Pair.Key >> 1) & 1, Pair.Key & 1,
            Pair.Value);
    }
    
    int32 NumChunks = ChunkMaskTransforms.Num();
    UE_LOG(LogTemp, Warning, TEXT("  Total Chunks: %d (ChunkSize: %d, Chunking: %s)"), 
        NumChunks, ChunkSize, bUseChunking ? TEXT("ON") : TEXT("OFF"));
    UE_LOG(LogTemp, Warning, TEXT("============================================================"));

    // 디버그: 그리드 데이터를 파일로 출력 (옵션)
    if (bDebugOutputGrid) {
        FString DebugFilePath = FPaths::ProjectSavedDir() / TEXT("DungeonDebug_Grid.txt");
        FString GridOutput;
        
        // 헤더
        GridOutput += FString::Printf(TEXT("=== Dungeon Grid Debug (Seed: 12345, Size: %dx%d) ===\n"), Grid.Width, Grid.Height);
        GridOutput += TEXT("Legend: W=Wall(mask), F=Floor, C=Corridor, .=None\n");
        GridOutput += TEXT("Coordinate: X+ = Right (→), Y+ = Down (↓) in this view\n");
        GridOutput += TEXT("            (Unreal viewport: X+ = Right, Y+ = Forward/Up)\n\n");
        
        // X 좌표 헤더 (10의 자리)
        GridOutput += TEXT("    ");
        for (int32 X = 0; X < Grid.Width; X++) {
            if (X % 10 == 0) {
                GridOutput += FString::Printf(TEXT("%d"), (X / 10) % 10);
            } else {
                GridOutput += TEXT(" ");
            }
        }
        GridOutput += TEXT("\n");
        
        // X 좌표 헤더 (1의 자리)
        GridOutput += TEXT("    ");
        for (int32 X = 0; X < Grid.Width; X++) {
            GridOutput += FString::Printf(TEXT("%d"), X % 10);
        }
        GridOutput += TEXT("\n");
        GridOutput += TEXT("    ");
        for (int32 X = 0; X < Grid.Width; X++) {
            GridOutput += TEXT("-");
        }
        GridOutput += TEXT("\n");
        
        // 그리드 출력 (Y=0이 위쪽)
        for (int32 Y = 0; Y < Grid.Height; Y++) {
            // Y 좌표
            GridOutput += FString::Printf(TEXT("%2d| "), Y);
            
            for (int32 X = 0; X < Grid.Width; X++) {
                const FDungeonTile& Tile = Grid.GetTile(X, Y);
                
                if (Tile.Type == ETileType::Wall) {
                    uint8 Mask = GetWallBitmask(X, Y, Grid);
                    // 16진수로 출력 (0-F)
                    GridOutput += FString::Printf(TEXT("%X"), Mask);
                } else if (Tile.Type == ETileType::Floor) {
                    GridOutput += TEXT(".");
                } else if (Tile.Type == ETileType::Corridor) {
                    GridOutput += TEXT("+");
                } else {
                    GridOutput += TEXT(" ");
                }
            }
            GridOutput += TEXT("\n");
        }
        
        GridOutput += TEXT("\n=== Bitmask Legend ===\n");
        GridOutput += TEXT("N=1, E=2, S=4, W=8\n");
        GridOutput += TEXT("0=Isolated, 5=N+S(vertical), A=E+W(horizontal), F=All\n");
        
        FFileHelper::SaveStringToFile(GridOutput, *DebugFilePath);
        UE_LOG(LogTemp, Warning, TEXT("DungeonTileRenderer: Debug grid exported to: %s"), *DebugFilePath);
    }

    // 2단계: 각 청크/마스크별로 ISMC 생성
    int32 TotalISMCs = 0;
    for (auto& ChunkPair : ChunkMaskTransforms) {
        FIntPoint ChunkCoord = ChunkPair.Key;
        TMap<uint8, TArray<FTransform>>& MaskTransforms = ChunkPair.Value;

        for (auto& MaskPair : MaskTransforms) {
            uint8 Mask = MaskPair.Key;
            TArray<FTransform>& Transforms = MaskPair.Value;

            if (Transforms.Num() == 0) continue;

            // 해당 마스크의 메시 가져오기
            UStaticMesh* Mesh = nullptr;
            if (UStaticMesh** MeshPtr = WallMeshTable.Find(Mask)) {
                Mesh = *MeshPtr;
            } else if (UStaticMesh** DefaultMesh = WallMeshTable.Find(0)) {
                Mesh = *DefaultMesh;
            }

            if (!Mesh) continue;

            // 새 HISM 동적 생성 (청크 + 마스크 기반 이름)
            FName ComponentName = FName(*FString::Printf(TEXT("Wall_C%d_%d_M%d"), 
                ChunkCoord.X, ChunkCoord.Y, Mask));
            
            // 에디터에서 저장되도록 Outer를 액터로 설정하고 플래그 지정
            // 중요: 이름 충돌을 원천 차단하기 위해 MakeUniqueObjectName 사용
            FString BaseName = FString::Printf(TEXT("Wall_C%d_%d_M%d"), ChunkCoord.X, ChunkCoord.Y, Mask);
            FName UniqueName = MakeUniqueObjectName(OwnerActor, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *BaseName);
            
            UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
                OwnerActor, 
                UHierarchicalInstancedStaticMeshComponent::StaticClass(),
                UniqueName,
                RF_Transactional);
            
            // 처음 생성 시 Attach 및 Register 수행
            NewHISM->AttachToComponent(OwnerActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            NewHISM->RegisterComponent();

            // 태그 추가 (Optimization: Name Parsing 대체)
            NewHISM->ComponentTags.Add(FName("DungeonComponent"));
            NewHISM->ComponentTags.Add(FName(*FString::Printf(TEXT("ChunkX:%d"), ChunkCoord.X)));
            NewHISM->ComponentTags.Add(FName(*FString::Printf(TEXT("ChunkY:%d"), ChunkCoord.Y)));
            NewHISM->ComponentTags.Add(FName(*FString::Printf(TEXT("Mask:%d"), Mask)));
            
            // 기존 이름 규칙도 유지 (Legacy support)
            NewHISM->SetStaticMesh(Mesh);
            NewHISM->SetMobility(EComponentMobility::Movable);
            
            // 콜리전 설정 (캐릭터와 충돌)
            NewHISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            NewHISM->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
            NewHISM->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
            NewHISM->SetGenerateOverlapEvents(false);
            
            // Note: AddInstanceComponent를 호출하지 않음
            // HISM 인스턴스 데이터는 직렬화되지 않으므로, StoredGrid로 항상 재생성함

            // 인스턴스 일괄 추가
            NewHISM->AddInstances(Transforms, false);

            // LOD 설정
            if (bUseLOD) {
                SetupLOD(NewHISM);
            }

            // 다이나믹 머티리얼
            if (bUseDynamicMaterials) {
                ApplyDynamicMaterial(NewHISM);
            }

            // Add to output array
            OutCreatedHISMs.Add(NewHISM);

            TotalISMCs++;
        }
    }

    // 천장 생성 (HISM)
    if (bGenerateCeiling && CeilingHISM) {
        GenerateCeiling(Grid, CeilingHISM);
    }

    // 바닥 생성 (HISM)
    if (bGenerateFloor && FloorHISM) {
        GenerateFloor(Grid, FloorHISM);
    }

    UE_LOG(LogTemp, Log, TEXT("DungeonTileRenderer: MultiMesh generation complete. Created %d HISMs across %d chunks."), 
        TotalISMCs, NumChunks);
}

uint8 UDungeonTileRenderer::GetWallBitmask(int32 X, int32 Y,
    const FDungeonGrid& Grid) const {
    uint8 Mask = 0;

    // 4방향 검사 (Unreal 좌표계: Y+ = North, X+ = West, X- = East, Y- = South)
    if (IsWall(X, Y + 1, Grid))
        Mask |= 1; // North (bit 0) - Y+ 방향
    if (IsWall(X - 1, Y, Grid))
        Mask |= 2; // East  (bit 1) - X- 방향 (오른쪽)
    if (IsWall(X, Y - 1, Grid))
        Mask |= 4; // South (bit 2) - Y- 방향
    if (IsWall(X + 1, Y, Grid))
        Mask |= 8; // West  (bit 3) - X+ 방향 (왼쪽)

    return Mask; // 0~15
}

uint8 UDungeonTileRenderer::GetWallBitmask8Dir(int32 X, int32 Y,
    const FDungeonGrid& Grid) const {
    uint8 Mask = 0;

    // 8방향 검사
    if (IsWall(X - 1, Y - 1, Grid))
        Mask |= 1; // NW  (bit 0)
    if (IsWall(X, Y - 1, Grid))
        Mask |= 2; // N   (bit 1)
    if (IsWall(X + 1, Y - 1, Grid))
        Mask |= 4; // NE  (bit 2)
    if (IsWall(X + 1, Y, Grid))
        Mask |= 8; // E   (bit 3)
    if (IsWall(X + 1, Y + 1, Grid))
        Mask |= 16; // SE  (bit 4)
    if (IsWall(X, Y + 1, Grid))
        Mask |= 32; // S   (bit 5)
    if (IsWall(X - 1, Y + 1, Grid))
        Mask |= 64; // SW  (bit 6)
    if (IsWall(X - 1, Y, Grid))
        Mask |= 128; // W   (bit 7)

    return Mask; // 0~255
}

bool UDungeonTileRenderer::IsWall(int32 X, int32 Y,
    const FDungeonGrid& Grid) const {
    if (!Grid.IsValid(X, Y)) {
        return false; // 경계 밖은 벽이 아님
    }

    const ETileType Type = Grid.GetTile(X, Y).Type;
    return Type == ETileType::Wall;
}

void UDungeonTileRenderer::GenerateCeiling(
    const FDungeonGrid& Grid, UHierarchicalInstancedStaticMeshComponent* CeilingHISM) {
    if (!CeilingHISM || !CeilingMesh) {
        return;
    }

    CeilingHISM->SetStaticMesh(CeilingMesh);

    TArray<FTransform> CeilingTransforms;
    CeilingTransforms.Reserve(Grid.Width * Grid.Height / 2);

    for (int32 Y = 0; Y < Grid.Height; Y++) {
        for (int32 X = 0; X < Grid.Width; X++) {
            const ETileType Type = Grid.GetTile(X, Y).Type;

            // 바닥 타일 위에 천장 생성
            if (Type == ETileType::Floor || Type == ETileType::Corridor) {
                FTransform Transform;
                Transform.SetLocation(FVector(X * TileSize, Y * TileSize, WallHeight) + CeilingPivotOffset);
                Transform.SetRotation(FQuat::Identity);
                Transform.SetScale3D(FVector::OneVector);

                CeilingTransforms.Add(Transform);
            }
        }
    }

    if (CeilingTransforms.Num() > 0) {
        CeilingHISM->AddInstances(CeilingTransforms, false);
    }

    UE_LOG(LogTemp, Log, TEXT("DungeonTileRenderer: Generated %d ceiling tiles (HISM)"),
        CeilingHISM->GetInstanceCount());
}

void UDungeonTileRenderer::GenerateFloor(
    const FDungeonGrid& Grid, UHierarchicalInstancedStaticMeshComponent* FloorHISM) {
    if (!FloorHISM || !FloorMesh) {
        return;
    }

    FloorHISM->SetStaticMesh(FloorMesh);

    TArray<FTransform> FloorTransforms;
    FloorTransforms.Reserve(Grid.Width * Grid.Height / 2);

    for (int32 Y = 0; Y < Grid.Height; Y++) {
        for (int32 X = 0; X < Grid.Width; X++) {
            const ETileType Type = Grid.GetTile(X, Y).Type;

            // 바닥 타일 생성 (옵션에 따라 벽 아래에도 생성)
            bool bShouldGenerateFloor = (Type == ETileType::Floor || Type == ETileType::Corridor);
            if (bGenerateFloorUnderWalls && Type == ETileType::Wall) {
                bShouldGenerateFloor = true;
            }

            if (bShouldGenerateFloor) {
                FTransform Transform;
                Transform.SetLocation(FVector(X * TileSize, Y * TileSize, 0.0f) + FloorPivotOffset);
                Transform.SetRotation(FQuat::Identity);
                Transform.SetScale3D(FVector::OneVector);

                FloorTransforms.Add(Transform);
            }
        }
    }

    if (FloorTransforms.Num() > 0) {
        FloorHISM->AddInstances(FloorTransforms, false);
    }

    UE_LOG(LogTemp, Log, TEXT("DungeonTileRenderer: Generated %d floor tiles (HISM)"),
        FloorHISM->GetInstanceCount());
}

void UDungeonTileRenderer::SetupLOD(UHierarchicalInstancedStaticMeshComponent* HISM) {
    if (!HISM || !bUseLOD) {
        return;
    }

    // LOD 거리 설정
    if (LODDistances.Num() > 0) {
        // HISM은 자동 LOD/Culling을 지원
        // 스태틱 메시 자체에 LOD가 설정되어 있어야 함
        // 여기서는 렌더링 거리만 설정

        // 최대 드로우 거리 설정
        float MaxDrawDistance =
            LODDistances.Num() > 0 ? LODDistances.Last() : 20000.0f;
        HISM->SetCachedMaxDrawDistance(MaxDrawDistance);

        UE_LOG(LogTemp, Log,
            TEXT("DungeonTileRenderer: HISM LOD enabled with %d levels, max distance "
                "%.1f"),
            LODDistances.Num(), MaxDrawDistance);
    }
}

void UDungeonTileRenderer::ApplyDynamicMaterial(
    UHierarchicalInstancedStaticMeshComponent* HISM) {
    if (!HISM || !bUseDynamicMaterials) {
        return;
    }

    // 현재 머티리얼 가져오기
    UMaterialInterface* CurrentMaterial = HISM->GetMaterial(0);
    if (!CurrentMaterial) {
        return;
    }

    // 동적 머티리얼 인스턴스 생성
    UMaterialInstanceDynamic* DynMaterial =
        UMaterialInstanceDynamic::Create(CurrentMaterial, HISM);
    if (!DynMaterial) {
        return;
    }

    // 파라미터 설정
    DynMaterial->SetScalarParameterValue(FName("Wetness"), WetnessIntensity);
    DynMaterial->SetScalarParameterValue(FName("Moss"), MossIntensity);

    // 랜덤 variation (타일별)
    float RandomSeed = FMath::FRand();
    DynMaterial->SetScalarParameterValue(FName("RandomSeed"), RandomSeed);

    // HISM에 적용
    HISM->SetMaterial(0, DynMaterial);

    UE_LOG(LogTemp, Log,
        TEXT("DungeonTileRenderer: Applied dynamic material to HISM (Wetness=%.2f, "
            "Moss=%.2f)"),
        WetnessIntensity, MossIntensity);
}

void UDungeonTileRenderer::ApplyTheme(const UDungeonThemeAsset* Theme) {
    if (!Theme) return;

    TileSize = Theme->TileSize;
    WallHeight = Theme->WallHeight;
    WallPivotOffset = Theme->WallPivotOffset;
    FloorPivotOffset = Theme->FloorPivotOffset;
    CeilingPivotOffset = Theme->CeilingPivotOffset;

    bGenerateFloor = Theme->bGenerateFloor;
    bGenerateCeiling = Theme->bGenerateCeiling;
    bGenerateFloorUnderWalls = Theme->bGenerateFloorUnderWalls;

    bUseLOD = Theme->bUseLOD;
    LODDistances = Theme->LODDistances;

    bUseDynamicMaterials = Theme->bUseDynamicMaterials;
    WetnessIntensity = Theme->WetnessIntensity;
    MossIntensity = Theme->MossIntensity;

    CeilingMesh = Theme->CeilingMesh;
    FloorMesh = Theme->FloorMesh;

    // Convert Wall Mesh Table (int32 -> uint8)
    WallMeshTable.Empty();
    for (const auto& Pair : Theme->WallMeshTable) {
        if (Pair.Key >= 0 && Pair.Key <= 255) {
            WallMeshTable.Add((uint8)Pair.Key, Pair.Value);
        }
    }
    
    // Add Fallback Mesh to Index 0 if 0 is missing
    if (Theme->FallbackWallMesh && !WallMeshTable.Contains(0)) {
         WallMeshTable.Add(0, Theme->FallbackWallMesh);
    }
}
