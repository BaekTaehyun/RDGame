# Dungeon Generation Refactoring: PCG Separation

## 개요 (Overview)
기존 `DungeonTileRenderer`는 Mesh 기반 렌더링(HISM)과 PCG 기반 렌더링을 모두 처리하며 역할이 비대해졌습니다. 이를 분리하고, PCG 데이터 처리가 원활하도록 구조를 개선했습니다.

## 변경 사항 (Changes)

### 1. `UDungeonPCGRenderer` 생성
*   **역할**: PCG 그래프 생성 및 라이프사이클 관리 전담.
*   **파일**: `DungeonPCGRenderer.h`, `DungeonPCGRenderer.cpp`
*   **내용**: ThemeAsset에 정의된 Room, Wall, Door 등의 PCG Graph를 스폰하고 관리합니다.

### 2. `UDungeonRendererComponent` 데이터 중앙화
*   **역할**: 생성된 그리드 데이터(`FDungeonGrid`)의 중앙 저장소 역할.
*   **변경**:
    *   `CachedGrid` 멤버 변수 추가.
    *   `GetCachedGrid()` 접근자 추가.
    *   `GenerateDungeon` 시 `UDungeonTileRenderer`(레거시)와 `UDungeonPCGRenderer`(신규)를 모두 조율.

### 3. `UDungeonTileRenderer` 정리 (Cleanup)
*   **변경**: PCG 관련 로직 및 프로퍼티 제거. 순수하게 HISM/StaticMesh 기반 타일 생성에만 집중하도록 변경.

### 4. `PCGDungeonDataReader` 개선
*   **변경**: 기존에는 `TileRenderer` 깊숙한 곳의 그리드를 찾으려 했으나, 이제 `DungeonRendererComponent`의 `GetCachedGrid()`를 통해 안정적으로 그리드 데이터에 접근.

## 아키텍처 다이어그램 (Architecture)

```mermaid
graph TD
    A[DungeonRendererComponent] -->|Manages| B[DungeonTileRenderer (Legacy HISM)]
    A -->|Manages| C[DungeonPCGRenderer (New PCG)]
    A -->|Stores| D[CachedGrid (Data)]
    
    C -->|Spawns| E[PCGComponent]
    E -->|Reads Data via| F[PCGDungeonDataReader]
    F -->|Accesses| D
```

## 확인 필요 사항 (Verification)
1.  **컴파일**: 변경된 코드를 컴파일(Ctrl+Alt+F11) 하십시오.
2.  **재생성**: 에디터 내에서 던전 생성 버튼을 눌러 재생성 하십시오.
3.  **검증**: 기존 HISM 벽과 PCG 오브젝트들이 정상적으로 공존하거나 대체되는지 확인하십시오.
