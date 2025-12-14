# UI 시스템 적용 계획 - 4단계 (Phase 4)

이 문서는 RDGame 프로젝트에 Lyra 스타일의 UI 시스템을 도입하기 위한 4단계(HUD 레이아웃 및 확장 시스템)의 상세 구현 계획입니다.

## 1. 개요 (Overview)
- **목표**: 실제 게임 화면에 표시될 메인 HUD 레이아웃을 생성하고, UI 확장 포인트를 설정하여 다른 시스템들이 동적으로 위젯을 주입할 수 있게 합니다.
- **예상 소요 시간**: 30분 ~ 1시간

## 2. 구현 상세 (Implementation Details)

### 2.1 메인 HUD 레이아웃 위젯: `URdHUDLayout`
`URdActivatableWidget`을 상속받아 게임의 메인 HUD 컨테이너 역할을 하는 위젯 클래스를 생성합니다.

- **파일 위치**: 
  - `Source/RdGame/UI/RdHUDLayout.h`
  - `Source/RdGame/UI/RdHUDLayout.cpp`
- **상속**: `URdActivatableWidget`
- **주요 기능**:
  - ESC 메뉴 처리
  - 입력 장치 변경 감지 (컨트롤러/키보드마우스)
  - UI Extension Point 위젯들을 포함할 컨테이너

### 2.2 블루프린트 위젯(WBP) 생성 가이드
C++ 클래스 구현 후, 블루프린트에서 실제 레이아웃을 구성합니다.

- **위젯 블루프린트**: `WBP_RdHUDLayout`
  - `URdHUDLayout` 상속
  - `UCommonActivatableWidgetStack` 배치 (메뉴 레이어용)
  - `UUIExtensionPointWidget` 배치 (동적 위젯 슬롯)

## 3. 검증 계획 (Verification)
1.  **컴파일 테스트**: C++ 클래스 컴파일 확인.
2.  **블루프린트 생성**: 에디터에서 `WBP_RdHUDLayout` 생성 가능 여부 확인.
3.  **게임 실행 테스트**: GameMode의 HUD Class를 `ARdHUD`로, UI Policy의 Layout을 `WBP_RdHUDLayout`으로 설정 후 게임 실행, HUD 표시 확인.

---
**완료 후**: 커밋 및 다음 기능 개발 진행
