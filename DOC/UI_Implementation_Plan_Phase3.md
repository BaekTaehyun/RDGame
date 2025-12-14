# UI 시스템 적용 계획 - 3단계 (Phase 3)

이 문서는 RDGame 프로젝트에 Lyra 스타일의 UI 시스템을 도입하기 위한 3단계(UI 매니저 및 정책)의 상세 구현 계획입니다.

## 1. 개요 (Overview)
- **목표**: UI 생성 및 관리 정책을 정의하는 `Policy` 클래스와 전역 UI 상태를 관리하는 `Subsystem`을 구현하여 체계적인 UI 관리를 가능하게 합니다.
- **예상 소요 시간**: 30분 ~ 1시간

## 2. 구현 상세 (Implementation Details)

### 2.1 UI 정책: `URdGameUIPolicy`
`CommonGame` 플러그인의 `UGameUIPolicy`를 상속받아 프로젝트(RdGame)의 구체적인 UI 정책을 정의합니다. 
주로 `PrimaryGameLayout` (메인 HUD 위젯) 클래스를 지정하는 역할을 합니다.

- **파일 위치**: 
  - `Source/RdGame/UI/RdGameUIPolicy.h`
  - `Source/RdGame/UI/RdGameUIPolicy.cpp`
- **상속**: `UGameUIPolicy`
- **주요 기능**:
  - `GetLayoutWidgetClass()`: 사용할 기본 레이아웃 위젯 클래스 반환 (블루프린트에서 설정 가능하도록 `UPROPERTY`로 노출)

### 2.2 UI 매니저 서브시스템: `URdUIManagerSubsystem`
게임플레이와 UI 시스템 간의 가교 역할을 합니다. `CommonGame`의 `UGameUIManagerSubsystem`을 상속받습니다.

- **파일 위치**:
  - `Source/RdGame/UI/RdUIManagerSubsystem.h`
  - `Source/RdGame/UI/RdUIManagerSubsystem.cpp`
- **상속**: `UGameUIManagerSubsystem`
- **주요 기능**:
  - `Tick()`: 매 프레임 실행되며, 현재 UI 상태(루트 레이아웃의 가시성 등)를 관리합니다.
  - HUD 가시성 동기화: `APlayerController`나 `AHUD`의 `bShowHUD` 값에 따라 전체 UI 레이아웃을 숨기거나 보이게 합니다.

## 3. 설정 변경 (Configuration)

### [MODIFY] DefaultGame.ini
구현한 정책 클래스를 게임에서 사용하도록 설정해야 합니다.

```ini
[/Script/CommonGame.CommonGameSettings]
GameUIPolicyClass=/Script/RdGame.RdGameUIPolicy
```

## 4. 검증 계획 (Verification)
1.  **컴파일 테스트**: 클래스 생성 후 컴파일 성공 확인.
2.  **설정 확인**: `DefaultGame.ini` 변경 후 에디터 실행 시 로그에서 Policy 초기화 메시지 확인 (필요시 로그 추가).
3.  **기능 확인**: (다음 4단계에서 HUD 위젯 생성 후) 게임 내에서 `ShowHUD` 콘솔 명령어로 UI가 숨겨지는지 확인.

---
**다음 단계**: 4단계 - HUD 레이아웃 및 확장 시스템 (Layout & Extensions)
