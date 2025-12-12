#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "DungeonChunkStreamer.generated.h"

/**
 * 던전 청크 스트리밍 매니저
 * 카메라 위치를 기반으로 청크를 동적으로 로드/언로드하여 성능 최적화
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonChunkStreamer : public UActorComponent
{
	GENERATED_BODY()

public:
	UDungeonChunkStreamer();

	// --- 스트리밍 설정 ---
	
	// 스트리밍 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	bool bEnableStreaming = false;

	// 카메라로부터의 청크 활성화 거리 (청크 단위, 예: 3 = 카메라 주변 7x7 청크)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming", meta = (ClampMin = "1", ClampMax = "10"))
	int32 StreamingDistance = 3;

	// 스트리밍 업데이트 주기 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float UpdateInterval = 0.5f;

	// 청크 크기 (타일 단위) - DungeonFullTestActor에서 설정
	UPROPERTY(BlueprintReadWrite, Category = "Streaming")
	int32 ChunkSize = 10;

	// 타일 크기 (언리얼 유닛) - DungeonFullTestActor에서 설정
	UPROPERTY(BlueprintReadWrite, Category = "Streaming")
	float TileSize = 100.0f;

	// --- 청크 데이터 ---
	
	// 청크별 HISM 맵 (DungeonFullTestActor에서 설정)
	// Note: TMap<FIntPoint, TArray<...>>는 UPROPERTY 지원 안됨
	TMap<FIntPoint, TArray<UHierarchicalInstancedStaticMeshComponent*>> ChunkHISMMap;

	// 청크별 머지된 메시 맵 (머징 활성화 시)
	UPROPERTY(Transient)
	TMap<FIntPoint, UDynamicMeshComponent*> ChunkMergedMeshMap;

	// 바닥/천장 HISM 청크 맵
	UPROPERTY(Transient)
	TMap<FIntPoint, UHierarchicalInstancedStaticMeshComponent*> ChunkFloorMap;

	UPROPERTY(Transient)
	TMap<FIntPoint, UHierarchicalInstancedStaticMeshComponent*> ChunkCeilingMap;

	// --- 런타임 ---
	
	// 현재 활성화된 청크들
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Streaming|Debug")
	TSet<FIntPoint> ActiveChunks;

	// 마지막 업데이트 시간
	float LastUpdateTime = 0.0f;

	// --- 함수 ---

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 카메라 위치를 기반으로 활성 청크 업데이트
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void UpdateActiveChunks(const FVector& CameraLocation);

	// 특정 청크의 가시성 설정
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void SetChunkVisible(FIntPoint ChunkCoord, bool bVisible);

	// 모든 청크 표시 (스트리밍 비활성화 시)
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void ShowAllChunks();

	// 청크 데이터 초기화
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void ClearChunkData();

protected:
	virtual void BeginPlay() override;

private:
	// 월드 위치를 청크 좌표로 변환
	FIntPoint WorldToChunkCoord(const FVector& WorldLocation) const;

	// 카메라 위치 가져오기
	FVector GetCameraLocation() const;
};
