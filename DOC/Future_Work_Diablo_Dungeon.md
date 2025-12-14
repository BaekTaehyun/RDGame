# Future Roadmap: Diablo-Style Dungeon Integration

이 문서는 **"프리셋 조립(Preset Assembly) 알고리즘"**을 통해 생성된 논리적 데이터를 실제 게임 월드에 시각적으로 완벽하게 구현하기 위해 **향후 수행해야 할 과제**를 정리합니다.

현재 단계(Phase 1)에서는 알고리즘이 그리드 데이터(`FDungeonGrid`)상의 타일 정보(`Floor`)만 채우고 있으며, 기존 렌더러는 이를 단순한 바닥 타일로만 표시합니다. 진정한 디아블로 스타일을 위해서는 **Phase 2 (Visual Integration)** 작업이 필요합니다.

---

## 1. 렌더러 확장 (Renderer Extension)

### 🚨 현재 상황
기존 `UDungeonRendererComponent`는 `FDungeonGrid`의 타일 타입(Wall, Floor)만 보고 **HISM(Instanced Mesh)**을 사용하여 큐브 형태의 타일을 배치합니다. 아티스트가 만든 정교한 '방 모듈(Level Instance)'은 사용되지 않습니다.

### ✅ 구현 과제: 모듈 액터 스폰 (Module Spawning)
알고리즘이 생성한 `PlacedModules` 리스트를 기반으로 실제 에셋을 배치하는 로직을 추가해야 합니다.

1.  **`Data/DungeonPresetData.h` 업데이트**:
    *   `FModuleData` 구조체에 있는 `AvailableModules` (SoftClassPtr)를 로드하는 로직 필요.
2.  **`DungeonRendererComponent.cpp` 확장**:
    *   `GenerateDungeon` 함수 마지막에 `SpawnPresetModules()` 호출 추가.
    *   `SpawnPresetModules()`는 `PlacedModules` 배열을 순회하며 `GetWorld()->SpawnActor` 또는 `LevelStreamingDynamic::LoadLevelInstance`를 실행.

> **Note**: 성능 최적화를 위해 단순 Actor보다는 **Level Instance** 사용을 권장합니다.

---

## 2. 스트리밍 시스템 업그레이드 (Streaming Upgrade)

### 🚨 현재 상황
기존 `UDungeonChunkStreamer`는 **HISM 컴포넌트**의 인스턴스 가시성(Visibility)만 제어합니다. 새로 스폰될 모듈 액터들은 이 시스템의 관리 대상이 아닙니다.

### ✅ 구현 과제: 액터 기반 스트리밍 (Actor-Based Streaming)
스트리머가 HISM뿐만 아니라 일반 Actor들도 관리할 수 있도록 확장해야 합니다.

1.  **`DungeonChunkStreamer.h` 수정**:
    *   `TMap<FIntPoint, AActor*> StreamedActorsMap` 추가. (청크 좌표 -> 스폰된 모듈 액터 매핑)
2.  **`UpdateActiveChunks()` 로직 수정**:
    *   현재: `HISM->SetCustomDataValue(...)` 또는 `SetVisibility` 사용.
    *   추가: `StreamedActorsMap`에서 해당 청크의 액터를 찾아 `SetActorHiddenInGame(bool)` 호출.
3.  **비동기 로딩 (Async Loading)**:
    *   모듈이 매우 많을 경우, 스트리밍 시점에 비동기로 로드/언로드하는 고도화 작업 고려 (World Partition과 유사한 로직).

---

## 3. 벽 자동 생성 (Auto-Walling / Gap Filling)

### 🚨 현재 상황
프리셋 알고리즘은 모듈 내부를 `Floor`로 채웁니다. 모듈과 모듈 사이의 빈 공간은 `Wall(None)` 상태입니다. 하지만 시각적으로는 "모듈의 외벽"이 필요할 수 있습니다.

### ✅ 구현 과제
아티스트가 방 에셋을 만들 때 외벽을 포함해서 만들면 해결되지만(가장 쉬운 방법), 만약 "내부 공간"만 모델링하고 외벽은 자동으로 채우고 싶다면 다음 로직이 필요합니다.

1.  **Border Analysis**: 그리드 상에서 `Floor`와 `Empty`가 만나는 경계선을 탐지.
2.  **Wall Injection**: 해당 경계선 좌표에 `Wall` 타입의 HISM이나 별도의 벽 모듈 자동 배치.

> **권장**: 일단 아티스트가 모듈 에셋에 외벽까지 포함해서 제작하는 것("Closed Box" 방식)이 프로그래밍 복잡도를 낮추는 가장 효율적인 방법입니다.

---

## 4. 요약 및 우선순위 (Priority)

| 우선순위 | 작업 항목 | 설명 | 난이도 |
| :--- | :--- | :--- | :--- |
| **High** | **Module Spawning** | 실제 방 에셋(Actor)을 월드에 배치하는 기능 | ⭐⭐⭐ |
| **High** | **Actor Streaming** | 배치된 방들을 플레이어 거리에 따라 숨기/보이기 | ⭐⭐ |
| Medium | Collision Handling | 스폰된 방들의 충돌 처리가 기존 시스템과 간섭 없는지 확인 | ⭐ |
| Low | Auto-Walling | 빈 공간을 자동으로 벽으로 채우기 (아티스트 의존도 낮춤) | ⭐⭐⭐⭐ |

**결론**: 다음 개발 스프린트에서는 **"Module Spawning"**부터 구현하여 생성된 던전이 시각적으로 보이도록 하는 것이 최우선입니다.
