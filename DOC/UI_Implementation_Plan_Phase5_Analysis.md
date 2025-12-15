# Lyra UI 추가 기능 이식 계획서

본 문서는 현재 RdGame에 아직 적용되지 않은 Lyra의 핵심 UI 기능들을 분석하고 이식 계획을 정리합니다.

---

## 1. 미적용 기능 분석

### 1.1 PrimaryGameLayout (CommonGame 플러그인)
**위치**: `Plugins/CommonGame/Source/Public/PrimaryGameLayout.h`

| 항목 | 설명 |
|------|------|
| **역할** | 플레이어 UI의 최상위 루트 위젯. 레이어 시스템을 통한 위젯 관리. |
| **핵심 기능** | - `RegisterLayer()`: GameplayTag로 레이어 등록<br>- `PushWidgetToLayerStack()`: 동기/비동기 위젯 Push<br>- `GetLayerWidget()`: 태그로 레이어 스택 조회<br>- `SetIsDormant()`: 휴면 상태 관리 |
| **현재 상태** | `URdHUDLayout`은 `URdActivatableWidget` 상속. Layer 시스템 미구현. |
| **이식 필요성** | **높음** - CommonUI 레이어 시스템의 핵심 |

### 1.2 LyraFrontendStateComponent
**위치**: `Source/LyraGame/UI/Frontend/LyraFrontendStateComponent.h`

| 항목 | 설명 |
|------|------|
| **역할** | 프론트엔드(메인 메뉴) 상태 및 흐름 관리 컴포넌트. GameState에 부착. |
| **핵심 기능** | - `ControlFlow` 기반 단계별 UI 흐름<br>- Press Start 화면 → 메인 메뉴 전환<br>- 사용자 초기화 대기<br>- 로딩 화면 표시 제어 |
| **의존성** | `ControlFlowManager`, `CommonUserSubsystem`, `CommonSessionSubsystem`, `LyraExperienceManagerComponent` |
| **현재 상태** | 미구현 |
| **이식 필요성** | **중간** - 메인 메뉴가 필요한 경우에만 |

### 1.3 CommonUIExtensions
**위치**: `Plugins/CommonGame/Source/Public/CommonUIExtensions.h`

| 항목 | 설명 |
|------|------|
| **역할** | UI 관련 유틸리티 함수 라이브러리 (BlueprintFunctionLibrary) |
| **핵심 기능** | - `PushContentToLayer_ForPlayer()`: 레이어에 위젯 Push<br>- `SuspendInputForPlayer()` / `ResumeInputForPlayer()`: 입력 일시 정지<br>- `GetOwningPlayerInputType()`: 입력 장치 타입 조회 |
| **현재 상태** | 플러그인에 이미 포함되어 있음 (사용 가능) |
| **이식 필요성** | **낮음** - 이미 사용 가능, 가이드만 필요 |

### 1.4 LyraInputConfig (입력 데이터 에셋)
**위치**: `Source/LyraGame/Input/LyraInputConfig.h`

| 항목 | 설명 |
|------|------|
| **역할** | GameplayTag ↔ InputAction 매핑 데이터 에셋 |
| **핵심 기능** | - `NativeInputActions`: 기본 입력 액션 목록<br>- `AbilityInputActions`: 어빌리티용 입력 액션 목록<br>- Tag로 InputAction 조회 |
| **현재 상태** | 이전에 `RdInputConfig` 또는 유사 클래스 존재 가능성 확인 필요 |
| **이식 필요성** | **높음** - GAS 및 입력 시스템 통합 시 필수 |

---

## 2. 이식 우선순위 및 계획

### Phase 5: PrimaryGameLayout 정식 도입
**목표**: `URdHUDLayout`을 `UPrimaryGameLayout` 상속으로 변경하여 레이어 시스템 활용

**작업 내용**:
1. `URdHUDLayout`의 부모를 `UPrimaryGameLayout`으로 변경
2. 블루프린트에서 `RegisterLayer()` 호출하여 레이어 등록
3. 레이어 GameplayTag 정의 (예: `UI.Layer.Game`, `UI.Layer.Menu`, `UI.Layer.Modal`)
4. 기존 코드 마이그레이션

**예상 소요**: 1~2시간

### Phase 6: UI 입력 설정 시스템
**목표**: GameplayTag 기반 입력 액션 매핑 시스템 구축

**작업 내용**:
1. `URdInputConfig` 데이터 에셋 클래스 생성
2. `FRdInputAction` 구조체 정의 (Tag ↔ InputAction)
3. `FindInputActionForTag()` 유틸리티 함수 구현
4. 블루프린트에서 데이터 에셋 생성 및 설정

**예상 소요**: 1시간

### Phase 7: 프론트엔드 상태 컴포넌트 (선택)
**목표**: 메인 메뉴 흐름 관리 시스템 구축

**작업 내용**:
1. `URdFrontendStateComponent` 생성 (GameStateComponent 상속)
2. ControlFlow 시스템 연동 (또는 단순화된 상태 머신)
3. Press Start / Main Menu 위젯 연결
4. 로딩 화면 인터페이스 구현

**의존성**: CommonUser 플러그인 정상 작동 필요  
**예상 소요**: 2~3시간

---

## 3. 즉시 사용 가능한 기능

### CommonUIExtensions 사용법
이미 `CommonGame` 플러그인에 포함되어 있어 바로 사용 가능합니다.

```cpp
#include "CommonUIExtensions.h"

// 레이어에 위젯 Push
UCommonUIExtensions::PushContentToLayer_ForPlayer(
    LocalPlayer,
    FGameplayTag::RequestGameplayTag("UI.Layer.Menu"),
    MyMenuWidgetClass
);

// 입력 일시 정지
FName Token = UCommonUIExtensions::SuspendInputForPlayer(PlayerController, "LoadingUI");

// 입력 재개
UCommonUIExtensions::ResumeInputForPlayer(PlayerController, Token);

// 입력 타입 체크
bool bUsingGamepad = UCommonUIExtensions::IsOwningPlayerUsingGamepad(MyWidget);
```

---

## 4. 권장 진행 순서

1. **[즉시]** `CommonUIExtensions` 활용 가이드 숙지
2. **[Phase 5]** `PrimaryGameLayout` 기반으로 `RdHUDLayout` 리팩토링
3. **[Phase 6]** 입력 설정 시스템 구축 (GAS 통합 시)
4. **[선택/Phase 7]** 메인 메뉴가 필요한 경우 FrontendState 구현

---

**다음 단계**: Phase 5 진행 여부 결정
