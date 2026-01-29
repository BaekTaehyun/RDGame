# PCG Cleanup 리팩터링 로직

## 문제
- 레벨 로드 후 Clear 버튼이 동작하지 않음
- PCG가 생성한 ISM/StaticMesh 컴포넌트가 삭제되지 않음
- 코드 중복 발생

## 해결

### DungeonPCGRenderer::Cleanup(AActor* Owner)

```cpp
void UDungeonPCGRenderer::Cleanup(AActor* Owner)
{
    // 1. PCG 컴포넌트 정리
    TArray<UPCGComponent*> PCGComponents;
    Owner->GetComponents<UPCGComponent>(PCGComponents);
    for (auto* PCG : PCGComponents) {
        PCG->CleanupLocalImmediate(true, true);
        PCG->DestroyComponent();
    }
    
    // 2. ISM 컴포넌트 정리
    TArray<UInstancedStaticMeshComponent*> ISMComponents;
    Owner->GetComponents<UInstancedStaticMeshComponent>(ISMComponents);
    for (auto* ISM : ISMComponents) {
        ISM->DestroyComponent();
    }
    
    // 3. StaticMesh 컴포넌트 정리
    TArray<UStaticMeshComponent*> SMComponents;
    Owner->GetComponents<UStaticMeshComponent>(SMComponents);
    for (auto* SM : SMComponents) {
        SM->DestroyComponent();
    }
    
    // 4. PCGWorldActor 삭제 (에디터)
#if WITH_EDITOR
    if (UWorld* World = Owner->GetWorld()) {
        if (UPCGSubsystem* PCGSubsystem = UPCGSubsystem::GetInstance(World)) {
            PCGSubsystem->DestroyCurrentPCGWorldActor();
        }
    }
#endif
}
```

### DungeonRendererComponent::ClearDungeon()

```cpp
void UDungeonRendererComponent::ClearDungeon() {
    // Legacy HISM 정리
    for (auto* HISM : CreatedWallHISMs) {
        if (IsValid(HISM)) HISM->DestroyComponent();
    }
    CreatedWallHISMs.Empty();
    ChunkHISMMap.Empty();
    
    // Merged Mesh 정리
    for (auto& Pair : MergedChunkMeshes) {
        if (Pair.Value) Pair.Value->DestroyComponent();
    }
    MergedChunkMeshes.Empty();
    
    // PCG 정리 - PCGRenderer가 null이면 생성
    AActor* Owner = GetOwner();
    if (Owner) {
        if (!PCGRenderer) {
            PCGRenderer = NewObject<UDungeonPCGRenderer>(this);
        }
        PCGRenderer->Cleanup(Owner);
    }
}
```

## 핵심 포인트

| 항목 | 설명 |
|------|------|
| **PCGRenderer** | `UPROPERTY(Transient)` - 레벨 로드 후 null |
| **해결** | null이면 새로 생성 후 Cleanup 호출 |
| **통일된 방식** | Owner Actor에서 컴포넌트 직접 탐색/삭제 |
| **레벨 로드 대응** | PCG 컴포넌트 없어도 ISM/SM 삭제 가능 |

---

## ThroughWall 코너 연결 수정

### 문제
두 ThroughWall이 L자로 만나는 코너에서 틈이 발생

### 해결

```cpp
// 이전 - ThroughWall 인접 타일은 CornerWall 제외
if (bIsLCorner && !bThisTileIsThroughWall && !bAdjacentToThroughWall)

// 이후 - ThroughWall 인접해도 CornerWall 배치 허용
if (bIsLCorner && !bThisTileIsThroughWall)
```

이제 ThroughWall+ThroughWall 코너에 **CornerWall이 배치**되어 틈이 메워집니다.
