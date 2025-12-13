#include "Rendering/DungeonChunkStreamer.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UDungeonChunkStreamer::UDungeonChunkStreamer() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickInterval =
      0.1f; // 기본 tick은 빠르게, 실제 업데이트는 UpdateInterval 사용
}

void UDungeonChunkStreamer::BeginPlay() {
  Super::BeginPlay();

  // 시작 시 모든 청크 활성화
  if (bEnableStreaming) {
    // 첫 틱에서 카메라 위치 기반으로 업데이트
    LastUpdateTime = -UpdateInterval; // 즉시 첫 업데이트 트리거
  } else {
    ShowAllChunks();
  }
}

void UDungeonChunkStreamer::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!bEnableStreaming) {
    return;
  }

  // 업데이트 주기 확인
  float CurrentTime = GetWorld()->GetTimeSeconds();
  if (CurrentTime - LastUpdateTime < UpdateInterval) {
    return;
  }
  LastUpdateTime = CurrentTime;

  // 카메라 위치 기반 청크 업데이트
  FVector CameraLocation = GetCameraLocation();
  UpdateActiveChunks(CameraLocation);
}

void UDungeonChunkStreamer::UpdateActiveChunks(const FVector &CameraLocation) {
  // 카메라 위치를 청크 좌표로 변환
  FIntPoint CameraChunk = WorldToChunkCoord(CameraLocation);

  // 청크맵 상태 로그 (디버그용)
  // UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: Camera at chunk (%d, %d),
  // ChunkHISMMap has %d entries"), 	CameraChunk.X, CameraChunk.Y,
  // ChunkHISMMap.Num());

  // [FIXED] First Run Check: 첫 실행 시 모든 청크를 숨기고 카메라 주변만 활성화
  if (ActiveChunks.Num() == 0 &&
      (ChunkHISMMap.Num() > 0 || ChunkMergedMeshMap.Num() > 0)) {

    UE_LOG(LogTemp, Warning,
           TEXT("[DungeonChunkStreamer] First Run: Initializing streaming with "
                "%d chunks"),
           ChunkHISMMap.Num());

    // 1. 먼저 모든 청크를 숨김 (초기 상태 정리)
    for (const auto &Pair : ChunkHISMMap) {
      SetChunkVisible(Pair.Key, false);
    }
    for (const auto &Pair : ChunkMergedMeshMap) {
      SetChunkVisible(Pair.Key, false);
    }

    // 2. 카메라 주변 청크만 활성화
    for (int32 Y = -StreamingDistance; Y <= StreamingDistance; Y++) {
      for (int32 X = -StreamingDistance; X <= StreamingDistance; X++) {
        FIntPoint ChunkCoord = CameraChunk + FIntPoint(X, Y);
        SetChunkVisible(ChunkCoord, true);
        ActiveChunks.Add(ChunkCoord);
      }
    }

    UE_LOG(LogTemp, Warning,
           TEXT("[DungeonChunkStreamer] First Run complete: %d chunks active "
                "around (%d, %d)"),
           ActiveChunks.Num(), CameraChunk.X, CameraChunk.Y);

    return; // 첫 실행 완료, 일반 로직 건너뛰기
  }

  // 새롭게 활성화할 청크 계산
  TSet<FIntPoint> NewActiveChunks;
  for (int32 Y = -StreamingDistance; Y <= StreamingDistance; Y++) {
    for (int32 X = -StreamingDistance; X <= StreamingDistance; X++) {
      FIntPoint ChunkCoord = CameraChunk + FIntPoint(X, Y);
      NewActiveChunks.Add(ChunkCoord);
    }
  }

  // 비활성화할 청크 (이전에 활성이었으나 새 범위에 없는 것)
  for (const FIntPoint &Chunk : ActiveChunks) {
    if (!NewActiveChunks.Contains(Chunk)) {
      SetChunkVisible(Chunk, false);
    }
  }

  // 활성화할 청크 (새 범위에 있으나 이전에 비활성이었던 것)
  for (const FIntPoint &Chunk : NewActiveChunks) {
    if (!ActiveChunks.Contains(Chunk)) {
      SetChunkVisible(Chunk, true);
    }
  }

  ActiveChunks = NewActiveChunks;
}

void UDungeonChunkStreamer::SetChunkVisible(FIntPoint ChunkCoord,
                                            bool bVisible) {
  int32 AffectedHISMCount = 0;

  // 벽 HISM들
  if (TArray<UHierarchicalInstancedStaticMeshComponent *> *HISMs =
          ChunkHISMMap.Find(ChunkCoord)) {
    for (UHierarchicalInstancedStaticMeshComponent *HISM : *HISMs) {
      if (HISM && IsValid(HISM)) {
        HISM->SetVisibility(bVisible, true);
        HISM->SetHiddenInGame(!bVisible, true); // 추가: HiddenInGame도 설정
        // 콜리전도 함께 제어 (성능 최적화)
        HISM->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryAndPhysics
                                           : ECollisionEnabled::NoCollision);
        AffectedHISMCount++;
      }
    }

    // UE_LOG(LogTemp, Warning, TEXT("SetChunkVisible: Chunk (%d,%d) -> %s,
    // affected %d HISMs out of %d"), 	ChunkCoord.X, ChunkCoord.Y, 	bVisible
    // ?
    // TEXT("SHOW") : TEXT("HIDE"), 	AffectedHISMCount, HISMs->Num());
  } else {
    // UE_LOG(LogTemp, Warning, TEXT("SetChunkVisible: Chunk (%d,%d) NOT FOUND
    // in ChunkHISMMap!"), 	ChunkCoord.X, ChunkCoord.Y);
  }

  // 머지된 메시
  if (UDynamicMeshComponent **MergedMesh =
          ChunkMergedMeshMap.Find(ChunkCoord)) {
    if (*MergedMesh) {
      (*MergedMesh)->SetVisibility(bVisible, true);
      (*MergedMesh)
          ->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryAndPhysics
                                         : ECollisionEnabled::NoCollision);
    }
  }

  // 바닥
  if (UHierarchicalInstancedStaticMeshComponent **Floor =
          ChunkFloorMap.Find(ChunkCoord)) {
    if (*Floor) {
      (*Floor)->SetVisibility(bVisible, true);
    }
  }

  // 천장
  if (UHierarchicalInstancedStaticMeshComponent **Ceiling =
          ChunkCeilingMap.Find(ChunkCoord)) {
    if (*Ceiling) {
      (*Ceiling)->SetVisibility(bVisible, true);
    }
  }

  if (bVisible) {
    // UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: Chunk (%d, %d)
    // activated"), ChunkCoord.X, ChunkCoord.Y);
  } else {
    // UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: Chunk (%d, %d)
    // deactivated"), ChunkCoord.X, ChunkCoord.Y);
  }
}

void UDungeonChunkStreamer::ShowAllChunks() {
  for (auto &Pair : ChunkHISMMap) {
    SetChunkVisible(Pair.Key, true);
    ActiveChunks.Add(Pair.Key);
  }

  for (auto &Pair : ChunkMergedMeshMap) {
    if (!ActiveChunks.Contains(Pair.Key)) {
      SetChunkVisible(Pair.Key, true);
      ActiveChunks.Add(Pair.Key);
    }
  }

  UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: All %d chunks shown"),
         ActiveChunks.Num());
}

void UDungeonChunkStreamer::ClearChunkData() {
  ChunkHISMMap.Empty();
  ChunkMergedMeshMap.Empty();
  ChunkFloorMap.Empty();
  ChunkCeilingMap.Empty();
  ActiveChunks.Empty();
}

FIntPoint
UDungeonChunkStreamer::WorldToChunkCoord(const FVector &WorldLocation) const {
  // 액터의 로컬 오프셋 고려
  FVector LocalLocation = WorldLocation;
  if (AActor *Owner = GetOwner()) {
    LocalLocation = WorldLocation - Owner->GetActorLocation();
  }

  float ChunkWorldSize = TileSize * ChunkSize;
  return FIntPoint(FMath::FloorToInt(LocalLocation.X / ChunkWorldSize),
                   FMath::FloorToInt(LocalLocation.Y / ChunkWorldSize));
}

FVector UDungeonChunkStreamer::GetCameraLocation() const {
  if (UWorld *World = GetWorld()) {
    if (APlayerController *PC = World->GetFirstPlayerController()) {
      if (APlayerCameraManager *CameraManager = PC->PlayerCameraManager) {
        return CameraManager->GetCameraLocation();
      }
    }
  }

  // 폴백: 첫 번째 플레이어 위치
  if (UWorld *World = GetWorld()) {
    if (APlayerController *PC = World->GetFirstPlayerController()) {
      if (APawn *Pawn = PC->GetPawn()) {
        return Pawn->GetActorLocation();
      }
    }
  }

  return FVector::ZeroVector;
}
