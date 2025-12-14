# UI 시스템 적용 계획 - 2단계 (Phase 2)

이 문서는 RDGame 프로젝트에 Lyra 스타일의 UI 시스템을 도입하기 위한 2단계(기반 클래스 및 종속성 구성)의 상세 구현 계획입니다.

## 1. 개요 (Overview)
- **목표**: UI 시스템의 뼈대가 되는 기반 클래스(`ActivatableWidget`, `HUD`)를 구현하고, 이를 지원하기 위한 모듈 종속성을 설정합니다.
- **예상 소요 시간**: 30분 ~ 1시간

## 2. 사전 작업: 종속성 설정 (Dependencies)
현재 `RDGame.Build.cs`에 누락된 CommonUI 관련 모듈을 추가해야 합니다.

### [MODIFY] RdGame.Build.cs
- **추가할 모듈**:
  - `CommonUI`
  - `CommonInput`
  - `CommonGame`
  - `UIExtension`
  - `GameFeatures` (확장성 대비)
  
```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    // ... 기존 모듈 ...
    "CommonUI",
    "CommonInput",
    "CommonGame",
    "UIExtension",
    "GameFeatures"
});
```

> [!IMPORTANT]
> 이 작업을 수행하기 전에 에디터를 종료하고, 작업 후에는 `.uproject` 파일을 우클릭하여 "Generate Visual Studio project files"를 실행하거나 리빌드해야 할 수 있습니다.

## 3. 구현 상세 (Implementation Details)

### 3.1 기반 위젯 클래스: `URdActivatableWidget`
Lyra의 `ULyraActivatableWidget`을 참조하여 프로젝트 전용 기반 클래스를 생성합니다. 이 클래스는 입력 모드(Game/Menu) 전환 정책을 관리합니다.

- **파일 위치**: 
  - `Source/RdGame/UI/RdActivatableWidget.h`
  - `Source/RdGame/UI/RdActivatableWidget.cpp`
- **상속**: `UCommonActivatableWidget`
- **주요 기능**:
  - `InputConfig`: 위젯 활성화 시 적용할 입력 모드 설정 (Game, GameMenu, Menu)
  - `GetDesiredInputConfig()`: 오버라이드하여 설정된 InputConfig 반환

### 3.2 HUD 액터: `ARdHUD`
UI 시스템의 초기화를 담당하는 앵커 액터입니다.

- **파일 위치**:
  - `Source/RdGame/UI/RdHUD.h`
  - `Source/RdGame/UI/RdHUD.cpp`
- **상속**: `AHUD`, `IGameFrameworkInitStateInterface`
- **주요 기능**:
  - **초기화 인터페이스 구현**: `IGameFrameworkInitStateInterface`를 통해 초기화 상태(State) 관리.
  - **Component Receiver**: `GameFrameworkComponentManager`에 자신을 리시버로 등록하여 다른 구성 요소(Extension)들이 HUD에 접근할 수 있도록 함.
  - **BeginPlay**: 초기화 완료 시 `UI.System.Ready` 또는 `Actor.Ready` 와 같은 이벤트 브로드캐스팅.

## 4. 검증 계획 (Verification)
1.  **컴파일 테스트**: 모든 모듈이 정상적으로 링크되고 컴파일되는지 확인.
2.  **클래스 노출 확인**: 언리얼 에디터에서 `WBP_TestWidget` 등을 생성할 때 `RdActivatableWidget`을 부모 클래스로 선택할 수 있는지 확인.
3.  **HUD 설정**: `GameMode`에서 HUD Class를 `ARdHUD`로 변경 후 게임 실행 시 로그에 초기화 메시지가 뜨는지 확인.

---
**다음 단계**: 3단계 - UI 매니저 및 정책 구현 (UI Manager & Policy)
