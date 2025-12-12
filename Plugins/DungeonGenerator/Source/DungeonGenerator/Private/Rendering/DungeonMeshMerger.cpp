#include "Rendering/DungeonMeshMerger.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "UDynamicMesh.h"
#include "Components/DynamicMeshComponent.h"


UDynamicMeshComponent* UDungeonMeshMerger::MergeHISMsToDynamicMesh(
	AActor* Owner,
	const TArray<UHierarchicalInstancedStaticMeshComponent*>& HISMs,
	FName ComponentName)
{
    FMeshCache LocalCache;
    UDynamicMeshComponent* Result = MergeHISMsToDynamicMeshWithCache(Owner, HISMs, ComponentName, LocalCache);
    
    // Clean up Cache
    for (auto& Pair : LocalCache)
    {
        if (Pair.Value) Pair.Value->RemoveFromRoot();
    }
    return Result;
}

UDynamicMeshComponent* UDungeonMeshMerger::MergeHISMsToDynamicMeshWithCache(
	AActor* Owner,
	const TArray<UHierarchicalInstancedStaticMeshComponent*>& HISMs,
	FName ComponentName,
    FMeshCache& MeshCache)
{
	if (!Owner || HISMs.Num() == 0)
	{
		return nullptr;
	}

	// DynamicMeshComponent 생성
	if (ComponentName == NAME_None)
	{
		ComponentName = FName(TEXT("MergedDungeonMesh"));
	}
	
	UDynamicMeshComponent* MergedComponent = NewObject<UDynamicMeshComponent>(Owner, ComponentName);
	if (!MergedComponent)
	{
		return nullptr;
	}

	MergedComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	
	// DynamicMesh 가져오기
	UDynamicMesh* TargetMesh = MergedComponent->GetDynamicMesh();
	if (!TargetMesh)
	{
		return nullptr;
	}

	int32 TotalInstances = 0;

	// 각 HISM의 모든 인스턴스를 병합
	for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMs)
	{
		if (!HISM || !HISM->GetStaticMesh())
		{
			continue;
		}

		bool bSuccess = AppendHISMToDynamicMeshWithCache(HISM, TargetMesh, MeshCache);
		if (bSuccess)
		{
			TotalInstances += HISM->GetInstanceCount();
		}
	}

	// 컴포넌트 등록
	MergedComponent->RegisterComponent();
	
	// 메시 변경 알림 및 바운드 업데이트 강제
	MergedComponent->NotifyMeshUpdated();
	MergedComponent->UpdateBounds();
	MergedComponent->MarkRenderStateDirty();
	
	// 컬링 설정
	MergedComponent->bNeverDistanceCull = true;
	MergedComponent->SetBoundsScale(1.2f);
	
	// 콜리전 설정 (Complex as Simple)
	MergedComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MergedComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	MergedComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    
    // 비동기 쿠킹 대신 즉시 쿠킹 강제 (편집 중 버벅임보다 정확성 우선 시)
    // 런타임 생성 시에는 비동기가 좋지만, 여기서는 확실한 생성을 위해.
    MergedComponent->bDeferCollisionUpdates = false; 

    // Complex Collision을 Simple Collision으로 사용하도록 설정
    MergedComponent->SetComplexAsSimpleCollisionEnabled(true, true);

	return MergedComponent;
}

bool UDungeonMeshMerger::AppendHISMToDynamicMesh(
	UHierarchicalInstancedStaticMeshComponent* HISM,
	UDynamicMesh* OutMesh)
{
    FMeshCache LocalCache;
    bool bResult = AppendHISMToDynamicMeshWithCache(HISM, OutMesh, LocalCache);
    
    for (auto& Pair : LocalCache)
    {
        if (Pair.Value) Pair.Value->RemoveFromRoot();
    }
    return bResult;
}

bool UDungeonMeshMerger::AppendHISMToDynamicMeshWithCache(
	UHierarchicalInstancedStaticMeshComponent* HISM,
	UDynamicMesh* OutMesh,
    FMeshCache& MeshCache)
{
	if (!HISM || !OutMesh || !HISM->GetStaticMesh())
	{
		return false;
	}

	UStaticMesh* StaticMesh = HISM->GetStaticMesh();
	int32 InstanceCount = HISM->GetInstanceCount();

	if (InstanceCount == 0)
	{
		return true;
	}

	// 모든 인스턴스 트랜스폼 수집
	TArray<FTransform> Transforms;
	Transforms.Reserve(InstanceCount);

	for (int32 i = 0; i < InstanceCount; i++)
	{
		FTransform InstanceTransform;
		HISM->GetInstanceTransform(i, InstanceTransform, false);
		Transforms.Add(InstanceTransform);
	}

	// StaticMesh 인스턴스들을 DynamicMesh에 추가
	AppendStaticMeshInstancesWithCache(StaticMesh, Transforms, OutMesh, MeshCache);

	return true;
}

void UDungeonMeshMerger::AppendStaticMeshInstances(
	UStaticMesh* StaticMesh,
	const TArray<FTransform>& Transforms,
	UDynamicMesh* OutMesh)
{
    FMeshCache LocalCache;
    AppendStaticMeshInstancesWithCache(StaticMesh, Transforms, OutMesh, LocalCache);
    
    for (auto& Pair : LocalCache)
    {
        if (Pair.Value) Pair.Value->RemoveFromRoot();
    }
}

void UDungeonMeshMerger::AppendStaticMeshInstancesWithCache(
	UStaticMesh* StaticMesh,
	const TArray<FTransform>& Transforms,
	UDynamicMesh* OutMesh,
    FMeshCache& MeshCache)
{
	if (!StaticMesh || !OutMesh || Transforms.Num() == 0)
	{
		return;
	}

    // Check Cache first
    UDynamicMesh* SourceMesh = nullptr;
    if (UDynamicMesh** CachedMesh = MeshCache.Find(StaticMesh))
    {
        SourceMesh = *CachedMesh;
    }
    else
    {
        // Safety: Ensure StaticMesh has render data
        if (!StaticMesh->GetRenderData())
        {
             UE_LOG(LogTemp, Warning, TEXT("DungeonMeshMerger: StaticMesh %s has no RenderData (CPU Access?)"), *StaticMesh->GetName());
             return;
        }

    	// StaticMesh를 DynamicMesh로 변환 (Slow Operation)
    	SourceMesh = NewObject<UDynamicMesh>();
        SourceMesh->AddToRoot(); // Cache에서 관리하므로 Rooting 유지
    	
    	FGeometryScriptCopyMeshFromAssetOptions CopyOptions;
    	FGeometryScriptMeshReadLOD ReadLOD;
    	ReadLOD.LODType = EGeometryScriptLODType::MaxAvailable;
    	
    	EGeometryScriptOutcomePins Outcome;
    	UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromStaticMesh(
    		StaticMesh,
    		SourceMesh,
    		CopyOptions,
    		ReadLOD,
    		Outcome);

    	if (Outcome != EGeometryScriptOutcomePins::Success)
    	{
    		UE_LOG(LogTemp, Warning, TEXT("DungeonMeshMerger: Failed to copy mesh from StaticMesh %s"), 
    			*StaticMesh->GetName());
            SourceMesh->RemoveFromRoot();
    		return;
    	}
        
        // Add to Cache
        MeshCache.Add(StaticMesh, SourceMesh);
    }
    
    if (!SourceMesh) return;

	// 각 트랜스폼으로 메시 추가 (Fast Operation)
	FGeometryScriptAppendMeshOptions AppendOptions;
	AppendOptions.CombineMode = EGeometryScriptCombineAttributesMode::EnableAllMatching;

	UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMeshTransformed(
		OutMesh,
		SourceMesh,
		Transforms,
		FTransform::Identity,
		false,
		false, 
		AppendOptions);
}

TMap<FIntPoint, UDynamicMeshComponent*> UDungeonMeshMerger::MergeHISMsPerChunk(
	AActor* Owner,
	const TMap<FIntPoint, TArray<UHierarchicalInstancedStaticMeshComponent*>>& ChunkHISMs,
	const FString& BaseName)
{
	TMap<FIntPoint, UDynamicMeshComponent*> Result;

	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("DungeonMeshMerger::MergeHISMsPerChunk - Owner is null"));
		return Result;
	}
    
    // 메시 캐시 생성 (함수 범위 내에서만 유효)
    FMeshCache MeshCache;

	for (const auto& Pair : ChunkHISMs)
	{
		const FIntPoint& ChunkCoord = Pair.Key;
		const TArray<UHierarchicalInstancedStaticMeshComponent*>& HISMs = Pair.Value;

		if (HISMs.Num() == 0)
		{
			continue;
		}

		// 청크별 고유 이름 생성
		FName ComponentName = FName(*FString::Printf(TEXT("%s_C%d_%d"), 
			*BaseName, ChunkCoord.X, ChunkCoord.Y));

		// 기존 함수 재사용하여 해당 청크의 HISM들을 병합
		UDynamicMeshComponent* MergedChunk = MergeHISMsToDynamicMeshWithCache(Owner, HISMs, ComponentName, MeshCache);

		if (MergedChunk)
		{
			Result.Add(ChunkCoord, MergedChunk);
		}
	}

    // 캐시 정리 (Rooting 해제)
    for (auto& Pair : MeshCache)
    {
        if (Pair.Value)
        {
            Pair.Value->RemoveFromRoot();
        }
    }
    MeshCache.Empty();

	UE_LOG(LogTemp, Log, TEXT("DungeonMeshMerger: Merged %d chunks for %s"), 
		Result.Num(), *BaseName);

	return Result;
}
