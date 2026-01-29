# 던전 랜드스케이프 & PCG 통합 구현 계획 (v5 - 최종안)

## 목표
DungeonGenerator를 기반으로 **'오픈 필드형 던전'**을 제작하는 에디터 워크플로우를 구축합니다.
1.  **지형 (Landscape)**: 던전 영역 전체에 "굴곡 있는 자연스러운 지형"을 에디터 타임에 생성합니다. 벽을 융기시키는 것이 아니라, 전반적으로 자연스러운 높낮이(Noise)를 부여합니다.
2.  **장애물 (PCG Walls)**: 이동 불가능한 영역(Wall)은 PCG 그래프가 나무, 바위 등을 스폰하여 막습니다.
3.  **바닥 디테일 (PCG Floors)**: 이동 가능한 영역(Floor)은 랜드스케이프가 그대로 보이거나 자잘한 풀/돌을 PCG로 스폰합니다.

## 아키텍처

### 1. [Editor Tool] 랜드스케이프 생성기
-   **역할**: 에디터에서 버튼 클릭 시 `ALandscape` 액터를 생성합니다.
-   **하이트맵 생성 로직**:
    -   던전 그리드 크기에 맞춰 해상도 결정.
    -   **Perlin Noise**를 적용하여 전체적으로 완만한 굴곡(Undulation) 생성.
    -   *옵션*: 던전의 'Wall' 영역이라고 해서 고도를 강제로 높이지 않습니다 (사용자 피드백 반영). 단, 원한다면 약간의 융기(Bump) 정도는 노이즈에 섞을 수 있도록 파라미터화 합니다.

### 2. [Plugin] DungeonPCG 브릿지
-   **역할**: 던전의 논리적 구조(Grid)를 PCG 데이터로 변환하여, 지형 위에 에셋을 배치합니다.
-   **데이터 변환 (`UDungeonPCGHelper`)**:
    -   `FDungeonGrid` -> `UPCGPointData` 변환.
    -   각 포인트에 `TileType` ("Wall", "Floor") 어트리뷰트 부여.
    -   Z축 위치는 랜드스케이프 알맞게 투영(Project)되거나 레이캐스트로 보정.

### 3. [PCG Graph] 로직 구성
-   **입력**: `DungeonPoints` (from Helper).
-   **분기 (Filter)**:
    -   `Type == Wall`: **Blocker Spawner** (큰 바위, 빽빽한 나무, 절벽 메시 등). -> *실질적인 벽 역할*
    -   `Type == Floor`: **Detail Spawner** (자갈, 꽃, 잔디). -> *플레이어 이동 가능*

## 단계별 구현

### 1단계: DungeonPCG 플러그인 (데이터 브릿지)
-   `GenerateDungeonPoints(Grid)` 함수 구현.
-   PCG 컴포넌트에 데이터 주입 기능.

### 2단계: 에디터 랜드스케이프 툴
-   `GenerateLandscape(Grid)` 구현.
-   C++ `ALandscapeProxy::Import`를 사용하여 노이즈 하이트맵 적용.
-   생성된 랜드스케이프에 `DungeonPoints`를 처리할 `PCGComponent` 자동 부착 및 실행.

### 3단계: 통합 및 검증
-   에디터에서 "Generate Dungeon Landscape" 실행.
-   결과: 굴곡진 땅(Landscape) 위에, 미로 형태의 숲(PCG Wall)이 생성됨.
-   사용자가 지형 스컬핑 모드로 바닥을 더 다듬거나, PCG 파라미터를 조절하여 숲의 밀도 수정 가능.
