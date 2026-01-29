# LyraHUDLayout HandleEscapeAction 분석 보고서

## 📋 개요

`HandleEscapeAction()` 함수는 플레이어가 ESC 키를 눌렀을 때 Escape 메뉴(일시정지 메뉴)를 표시하는 기능입니다.

```cpp
void ULyraHUDLayout::HandleEscapeAction()
{
    if (ensure(!EscapeMenuClass.IsNull()))
    {
        UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(
            GetOwningLocalPlayer(), 
            TAG_UI_LAYER_MENU, 
            EscapeMenuClass
        );
    }
}
```

---

## 🔍 필수 의존성 분석

### 1. **코드 의존성** (현재 RdGame에 없음 ❌)

| 파일 | 상태 | 설명 |
|-----|------|------|
| `LyraActivatableWidget.h/cpp` | ❌ 없음 | LyraHUDLayout의 부모 클래스 |
| `LyraHUDLayout.h/cpp` | ❌ 없음 | ESC 메뉴 로직이 있는 HUD 레이아웃 클래스 |
| `LyraControllerDisconnectedScreen.h/cpp` | ❌ 없음 | 컨트롤러 연결 끊김 화면 (선택사항) |

### 2. **코드 의존성** (현재 RdGame에 있음 ✅)

| 파일 | 상태 | 설명 |
|-----|------|------|
| `CommonUIExtensions.h` | ✅ 있음 | `PushStreamedContentToLayer_ForPlayer` 함수 제공 |
| `CommonActivatableWidget.h` | ✅ 있음 | CommonUI 플러그인의 활성화 가능 위젯 |

### 3. **GameplayTag 의존성**

| 태그 | 상태 | 용도 |
|-----|------|------|
| `UI.Layer.Menu` | ✅ 있음 | ESC 메뉴가 표시될 UI 레이어 |
| `UI.Action.Escape` | ❌ 없음 | ESC 액션 바인딩 태그 |
| `Platform.Trait.Input.PrimarlyController` | ❌ 없음 | 컨트롤러 전용 플랫폼 태그 (선택사항) |

---

## 🎮 리소스 의존성

### 필수 Blueprint/Asset

| 리소스 | 타입 | 설명 |
|--------|------|------|
| `EscapeMenuClass` | `TSoftClassPtr<UCommonActivatableWidget>` | ESC 메뉴 위젯 블루프린트 |
| `W_LyraMainMenu` (예시) | Widget Blueprint | Lyra의 기본 ESC 메뉴 |

### Lyra에서의 설정 방식

`LyraHUDLayout`은 **Widget Blueprint**로 확장되어 사용됩니다:
- `WBP_HUDLayout_ShooterGame` (ShooterCore GameFeature)
- 이 블루프린트에서 `EscapeMenuClass` 프로퍼티에 메뉴 위젯 할당

---

## 🔧 RegisterUIActionBinding 설명

```cpp
// NativeOnInitialized에서 호출
RegisterUIActionBinding(
    FBindUIActionArgs(
        FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE),  // UI.Action.Escape 태그
        false,  // bPersistBinding: false = 위젯이 활성 상태일 때만 작동
        FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction)
    )
);
```

**작동 원리:**
1. `UI.Action.Escape` 태그가 CommonUI 입력 설정에 등록되어 있어야 함
2. ESC 키가 눌리면 CommonUI가 이 태그를 브로드캐스트
3. 바인딩된 델리게이트 (`HandleEscapeAction`) 실행

---

## 📁 포팅이 필요한 파일 목록

```
Lyra_/Source/LyraGame/UI/
├── LyraActivatableWidget.h       → Source/RdGame/UI/Foundation/RdActivatableWidget.h
├── LyraActivatableWidget.cpp     → Source/RdGame/UI/Foundation/RdActivatableWidget.cpp
├── LyraHUDLayout.h               → Source/RdGame/UI/RdHUDLayout.h
├── LyraHUDLayout.cpp             → Source/RdGame/UI/RdHUDLayout.cpp
└── Foundation/
    └── LyraControllerDisconnectedScreen.h/cpp  (선택사항)
```

---

## ⚙️ 필요한 설정 작업

### 1. GameplayTag 등록

`Config/DefaultGameplayTags.ini` 또는 C++ 코드에서:
```cpp
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE, "UI.Action.Escape");
```

### 2. CommonUI 입력 설정

`CommonUIInputSettings`에서 `UI.Action.Escape` 태그와 ESC 키 연결 필요

### 3. ESC 메뉴 위젯 생성

`UCommonActivatableWidget`을 상속받는 ESC 메뉴 Widget Blueprint 생성:
- 게임 일시정지 옵션
- 설정 버튼
- 게임 종료 버튼

### 4. GameFeature 또는 Experience에서 HUD 추가

`GameFeatureAction_AddWidget`을 통해 `RdHUDLayout`을 HUD에 추가

---

## 📊 구현 우선순위

| 우선순위 | 작업 | 난이도 |
|---------|------|--------|
| 1 | `RdActivatableWidget` 포팅 | 쉬움 |
| 2 | `RdHUDLayout` 포팅 | 중간 |
| 3 | `UI.Action.Escape` 태그 & 입력 설정 | 쉬움 |
| 4 | ESC 메뉴 WBP 생성 | 중간 |
| 5 | GameFeature에서 HUD 등록 | 쉬움 |

---

## 🔗 관련 파일 경로

### Lyra 원본
- `Lyra_/Source/LyraGame/UI/LyraHUDLayout.h`
- `Lyra_/Source/LyraGame/UI/LyraActivatableWidget.h`
- `Lyra_/Plugins/CommonGame/Source/Public/CommonUIExtensions.h`

### RdGame (기존)
- `Source/RdGame/UI/Frontend/RdFrontendStateComponent.cpp` (TAG_UI_LAYER_MENU 정의됨)
- `Plugins/CommonGame/` (CommonUIExtensions 있음)

---

*Generated: 2025-12-16*
