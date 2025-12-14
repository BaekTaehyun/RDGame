# RdGame UI 시스템 사용 가이드

본 문서는 Lyra 아키텍처를 기반으로 구현된 RdGame의 UI 시스템 사용 방법을 설명합니다.

---

## 1. 시스템 구성 요소

| 클래스 | 역할 |
|--------|------|
| `URdActivatableWidget` | 모든 활성화 가능한 위젯의 기반 클래스. 입력 모드(Game/Menu) 자동 전환. |
| `ARdHUD` | HUD 앵커 액터. GameFeature 확장 이벤트 트리거. |
| `URdGameUIPolicy` | UI 생성 정책. 메인 레이아웃 위젯 클래스 지정. |
| `URdUIManagerSubsystem` | 전역 UI 상태 관리. |
| `URdHUDLayout` | 메인 HUD 레이아웃 위젯. 메뉴 레이어 및 확장 포인트 포함. |

---

## 2. 초기 설정

### 2.1 GameMode 설정
게임모드 블루프린트 또는 C++에서 HUD 클래스를 지정합니다.

```cpp
// C++ GameMode 생성자 예시
ARdGameMode::ARdGameMode()
{
    HUDClass = ARdHUD::StaticClass();
}
```

또는 블루프린트 GameMode의 **Classes > HUD Class**를 `RdHUD`로 설정.

### 2.2 UI Policy 설정 (DefaultGame.ini)
`Config/DefaultGame.ini`에 다음이 설정되어 있어야 합니다:

```ini
[/Script/CommonGame.CommonGameSettings]
GameUIPolicyClass=/Script/RdGame.RdGameUIPolicy
```

### 2.3 레이아웃 위젯 생성 (에디터)
1.  **Content Browser** > 우클릭 > **User Interface** > **Widget Blueprint**
2.  **Parent Class**를 `RdHUDLayout`으로 선택
3.  이름: `WBP_RdHUDLayout`
4.  디자이너에서 원하는 HUD 요소 배치

### 2.4 레이아웃 클래스 지정
`URdGameUIPolicy`를 상속받는 블루프린트를 만들거나, `DefaultGame.ini`에서 직접 지정:

```ini
[/Script/RdGame.RdGameUIPolicy]
LayoutClass=/Game/UI/WBP_RdHUDLayout.WBP_RdHUDLayout_C
```

---

## 3. 새 위젯 생성하기

### 3.1 일반 UI 위젯 (HUD 요소)
1.  `URdActivatableWidget`을 상속받는 위젯 블루프린트 생성
2.  `InputConfig` 설정:
    - `Default`: 입력 모드 변경 없음
    - `Game`: 게임 입력 유지 (HUD)
    - `Menu`: UI가 입력 소비 (메뉴)
    - `GameAndMenu`: 게임과 UI 모두 입력 수신

### 3.2 메뉴 위젯 (모달/오버레이)
메뉴나 팝업은 `MenuLayer`에 Push 합니다:

```cpp
// C++ 예시
if (URdHUDLayout* Layout = GetHUDLayout())
{
    if (Layout->MenuLayer)
    {
        Layout->MenuLayer->AddWidget(MyMenuWidgetClass);
    }
}
```

블루프린트에서는 `WBP_RdHUDLayout`의 `MenuLayer` 변수에 접근하여 `Add Widget` 호출.

---

## 4. 입력 모드 제어

`URdActivatableWidget`의 `InputConfig` 속성을 통해 위젯 활성화 시 입력 처리 방식을 제어합니다.

| InputConfig | 게임 입력 | UI 입력 | 마우스 캡처 | 용도 |
|-------------|----------|--------|------------|------|
| `Default` | 유지 | 유지 | 유지 | 기본값 |
| `Game` | ✓ | ✗ | 영구 캡처 | HUD 요소 |
| `Menu` | ✗ | ✓ | 해제 | 메뉴, 인벤토리 |
| `GameAndMenu` | ✓ | ✓ | 영구 캡처 | 인게임 상호작용 UI |

---

## 5. UI 레이어 시스템 (CommonUI)

CommonUI의 레이어 시스템을 사용하면 위젯을 계층적으로 관리할 수 있습니다.

### 레이어 Push/Pop
```cpp
#include "CommonUIExtensions.h"

// 레이어에 위젯 추가
UCommonUIExtensions::PushContentToLayer_ForPlayer(
    PlayerController,
    FGameplayTag::RequestGameplayTag("UI.Layer.Menu"),
    MyWidgetClass
);
```

### 표준 레이어 태그
- `UI.Layer.Game`: 인게임 HUD
- `UI.Layer.GameMenu`: 게임 내 메뉴 (ESC 메뉴)
- `UI.Layer.Menu`: 전체 화면 메뉴
- `UI.Layer.Modal`: 팝업/대화상자

---

## 6. 확장 포인트 (Extension Points)

다른 시스템(인벤토리, 퀘스트 등)이 HUD에 동적으로 위젯을 주입할 수 있습니다.

### 6.1 HUD에 슬롯 정의
`WBP_RdHUDLayout` 디자이너에서:
1.  `UUIExtensionPointWidget` 배치
2.  **Extension Point Tag** 설정 (예: `HUD.Slot.Minimap`)

### 6.2 위젯 등록
```cpp
#include "UIExtensionSubsystem.h"

UUIExtensionSubsystem* ExtSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
if (ExtSubsystem)
{
    ExtSubsystem->RegisterExtensionAsWidgetForContext(
        FGameplayTag::RequestGameplayTag("HUD.Slot.Minimap"),
        PlayerController,
        MinimapWidgetClass,
        -1  // Priority
    );
}
```

---

## 7. 디버깅

### HUD 가시성 토글
콘솔 명령어:
```
ShowHUD
```

### UI Policy 확인
에디터 Output Log에서 `LogCommonGame` 카테고리 필터링.

---

## 8. 요약: 새 UI 추가 체크리스트

1.  [ ] `URdActivatableWidget` 상속 위젯 블루프린트 생성
2.  [ ] `InputConfig` 설정 (Game/Menu/GameAndMenu)
3.  [ ] HUD에 직접 배치 또는 MenuLayer에 Push
4.  [ ] (선택) Extension Point를 통한 동적 등록
5.  [ ] 테스트: 입력 모드, 가시성, 레이어 순서 확인
