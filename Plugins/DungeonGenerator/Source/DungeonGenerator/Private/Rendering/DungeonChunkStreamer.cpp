#include "Rendering/DungeonChunkStreamer.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"

UDungeonChunkStreamer::UDungeonChunkStreamer()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 기본 tick은 빠르게, 실제 업데이트는 UpdateInterval 사용
}

void UDungeonChunkStreamer::BeginPlay()
{
	Super::BeginPlay();

	// 시작 시 모든 청크 활성화
	if (bEnableStreaming)
	{
		// 첫 틱에서 카메라 위치 기반으로 업데이트
		LastUpdateTime = -UpdateInterval; // 즉시 첫 업데이트 트리거
	}
	else
	{
		ShowAllChunks();
	}
}

void UDungeonChunkStreamer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableStreaming)
	{
		return;
	}

	// 업데이트 주기 확인
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastUpdateTime < UpdateInterval)
	{
		return;
	}
	LastUpdateTime = CurrentTime;

	// 카메라 위치 기반 청크 업데이트
	FVector CameraLocation = GetCameraLocation();
	UpdateActiveChunks(CameraLocation);
}

void UDungeonChunkStreamer::UpdateActiveChunks(const FVector& CameraLocation)
{
	// 카메라 위치를 청크 좌표로 변환
	FIntPoint CameraChunk = WorldToChunkCoord(CameraLocation);

	// 청크맵 상태 로그 (디버그용)
	UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: Camera at chunk (%d, %d), ChunkHISMMap has %d entries"), 
		CameraChunk.X, CameraChunk.Y, ChunkHISMMap.Num());

    // [New Logic] First Run Check: If ActiveChunks is empty but we have map data, 
    // it implies all chunks were set to Visible by RebuildChunkMaps. 
    // We must populate ActiveChunks with EVERYTHING so the difference logic below will turn off the distant ones.
    if (ActiveChunks.Num() == 0 && (ChunkHISMMap.Num() > 0 || ChunkMergedMeshMap.Num() > 0)) {
        TSet<FIntPoint> AllKeys;
        ChunkHISMMap.GetKeys(AllKeys);
        for(const auto& Pair : ChunkMergedMeshMap) { AllKeys.Add(Pair.Key); }
        for(const auto& Pair : ChunkFloorMap) { AllKeys.Add(Pair.Key); }
        
        ActiveChunks = AllKeys;
        UE_LOG(LogTemp, Warning, TEXT("[DungeonChunkStreamer] First Run detected. Initialized ActiveChunks with %d existing chunks."), ActiveChunks.Num());
    }

	// 새롭게 활성화할 청크 계산
	TSet<FIntPoint> NewActiveChunks;
	for (int32 Y = -StreamingDistance; Y <= StreamingDistance; Y++)
	{
		for (int32 X = -StreamingDistance; X <= StreamingDistance; X++)
		{
			FIntPoint ChunkCoord = CameraChunk + FIntPoint(X, Y);
			NewActiveChunks.Add(ChunkCoord);
		}
	}

	// 비활성화할 청크 (이전에 활성이었으나 새 범위에 없는 것)
	for (const FIntPoint& Chunk : ActiveChunks)
	{
		if (!NewActiveChunks.Contains(Chunk))
		{
			SetChunkVisible(Chunk, false);
		}
	}

	// 활성화할 청크 (새 범위에 있으나 이전에 비활성이었던 것)
	for (const FIntPoint& Chunk : NewActiveChunks)
	{
		if (!ActiveChunks.Contains(Chunk))
		{
			SetChunkVisible(Chunk, true);
		}
	}

	ActiveChunks = NewActiveChunks;
}

void UDungeonChunkStreamer::SetChunkVisible(FIntPoint ChunkCoord, bool bVisible)
{
	int32 AffectedHISMCount = 0;
	
	// 벽 HISM들
	if (TArray<UHierarchicalInstancedStaticMeshComponent*>* HISMs = ChunkHISMMap.Find(ChunkCoord))
	{
		for (UHierarchicalInstancedStaticMeshComponent* HISM : *HISMs)
		{
			if (HISM && IsValid(HISM))
			{
				HISM->SetVisibility(bVisible, true);
				HISM->SetHiddenInGame(!bVisible, true);  // 추가: HiddenInGame도 설정
				// 콜리전도 함께 제어 (성능 최적화)
				HISM->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
				AffectedHISMCount++;
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("SetChunkVisible: Chunk (%d,%d) -> %s, affected %d HISMs out of %d"), 
			ChunkCoord.X, ChunkCoord.Y, 
			bVisible ? TEXT("SHOW") : TEXT("HIDE"),
			AffectedHISMCount, HISMs->Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SetChunkVisible: Chunk (%d,%d) NOT FOUND in ChunkHISMMap!"), 
			ChunkCoord.X, ChunkCoord.Y);
	}

	// 머지된 메시
	if (UDynamicMeshComponent** MergedMesh = ChunkMergedMeshMap.Find(ChunkCoord))
	{
		if (*MergedMesh)
		{
			(*MergedMesh)->SetVisibility(bVisible, true);
			(*MergedMesh)->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}

	// 바닥
	if (UHierarchicalInstancedStaticMeshComponent** Floor = ChunkFloorMap.Find(ChunkCoord))
	{
		if (*Floor)
		{
			(*Floor)->SetVisibility(bVisible, true);
		}
	}

	// 천장
	if (UHierarchicalInstancedStaticMeshComponent** Ceiling = ChunkCeilingMap.Find(ChunkCoord))
	{
		if (*Ceiling)
		{
			(*Ceiling)->SetVisibility(bVisible, true);
		}
	}

	if (bVisible)
	{
		UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: Chunk (%d, %d) activated"), ChunkCoord.X, ChunkCoord.Y);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: Chunk (%d, %d) deactivated"), ChunkCoord.X, ChunkCoord.Y);
	}
}

void UDungeonChunkStreamer::ShowAllChunks()
{
	for (auto& Pair : ChunkHISMMap)
	{
		SetChunkVisible(Pair.Key, true);
		ActiveChunks.Add(Pair.Key);
	}

	for (auto& Pair : ChunkMergedMeshMap)
	{
		if (!ActiveChunks.Contains(Pair.Key))
		{
			SetChunkVisible(Pair.Key, true);
			ActiveChunks.Add(Pair.Key);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DungeonChunkStreamer: All %d chunks shown"), ActiveChunks.Num());
}

void UDungeonChunkStreamer::ClearChunkData()
{
	ChunkHISMMap.Empty();
	ChunkMergedMeshMap.Empty();
	ChunkFloorMap.Empty();
	ChunkCeilingMap.Empty();
	ActiveChunks.Empty();
}

FIntPoint UDungeonChunkStreamer::WorldToChunkCoord(const FVector& WorldLocation) const
{
	// 액터의 로컬 오프셋 고려
	FVector LocalLocation = WorldLocation;
	if (AActor* Owner = GetOwner())
	{
		LocalLocation = WorldLocation - Owner->GetActorLocation();
	}

	float ChunkWorldSize = TileSize * ChunkSize;
	return FIntPoint(
		FMath::FloorToInt(LocalLocation.X / ChunkWorldSize),
		FMath::FloorToInt(LocalLocation.Y / ChunkWorldSize)
	);
}

FVector UDungeonChunkStreamer::GetCameraLocation() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
			{
				return CameraManager->GetCameraLocation();
			}
		}
	}

	// 폴백: 첫 번째 플레이어 위치
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				return Pawn->GetActorLocation();
			}
		}
	}

	return FVector::ZeroVector;
}
