#pragma once

#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "DungeonMeshMerger.generated.h"

class UDynamicMesh;

/**
 * 던전 청크 메시 병합 유틸리티
 * GeometryScript를 사용하여 런타임에 HISM 인스턴스들을 단일 메시로 병합
 */
UCLASS(BlueprintType)
class DUNGEONGENERATOR_API UDungeonMeshMerger : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 여러 HISM 컴포넌트의 모든 인스턴스를 단일 DynamicMeshComponent로 병합
	 * @param Owner - 결과 컴포넌트를 생성할 Actor
	 * @param HISMs - 병합할 HISM 컴포넌트 배열
	 * @param ComponentName - 결과 컴포넌트 이름
	 * @return 병합된 메시를 담은 DynamicMeshComponent
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Mesh Merging")
	static UDynamicMeshComponent* MergeHISMsToDynamicMesh(
		AActor* Owner,
		const TArray<UHierarchicalInstancedStaticMeshComponent*>& HISMs,
		FName ComponentName = NAME_None);

	/**
	 * 단일 HISM의 모든 인스턴스를 DynamicMesh로 병합
	 * @param HISM - 병합할 HISM 컴포넌트
	 * @param OutMesh - 결과를 담을 DynamicMesh
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Mesh Merging")
	static bool AppendHISMToDynamicMesh(
		UHierarchicalInstancedStaticMeshComponent* HISM,
		UDynamicMesh* OutMesh);

	/**
	 * StaticMesh를 지정된 트랜스폼들로 DynamicMesh에 추가
	 * @param StaticMesh - 추가할 스태틱 메시
	 * @param Transforms - 인스턴스 트랜스폼 배열
	 * @param OutMesh - 결과를 담을 DynamicMesh
	 */
	static void AppendStaticMeshInstances(
		UStaticMesh* StaticMesh,
		const TArray<FTransform>& Transforms,
		UDynamicMesh* OutMesh);

	/**
	 * 청크별로 HISM들을 각각의 DynamicMeshComponent로 병합
	 * 컬링 효율을 유지하면서 드로우콜 감소
	 * @param Owner - 컴포넌트를 생성할 Actor
	 * @param ChunkHISMs - 청크 좌표별 HISM 배열 맵
	 * @param BaseName - 생성할 컴포넌트 기본 이름
	 * @return 청크 좌표별 머지된 DynamicMeshComponent 맵
	 */
	// Note: TMap<FIntPoint, TArray<...>>는 UFUNCTION 지원 안됨
	static TMap<FIntPoint, UDynamicMeshComponent*> MergeHISMsPerChunk(
		AActor* Owner,
		const TMap<FIntPoint, TArray<UHierarchicalInstancedStaticMeshComponent*>>& ChunkHISMs,
		const FString& BaseName);

    // --- Internal Optimized API (Uses Cache) ---
    // Key: UStaticMesh*, Value: UDynamicMesh*
    using FMeshCache = TMap<UStaticMesh*, UDynamicMesh*>;

    static UDynamicMeshComponent* MergeHISMsToDynamicMeshWithCache(
		AActor* Owner,
		const TArray<UHierarchicalInstancedStaticMeshComponent*>& HISMs,
		FName ComponentName,
        FMeshCache& MeshCache);

    static bool AppendHISMToDynamicMeshWithCache(
		UHierarchicalInstancedStaticMeshComponent* HISM,
		UDynamicMesh* OutMesh,
        FMeshCache& MeshCache);

    static void AppendStaticMeshInstancesWithCache(
		UStaticMesh* StaticMesh,
		const TArray<FTransform>& Transforms,
		UDynamicMesh* OutMesh,
        FMeshCache& MeshCache);
};
