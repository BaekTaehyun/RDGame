# RdGame UI 시스템 분석 문서

## 개요
이 문서는 Lyra 프로젝트의 UI 시스템을 분석하고 RdGame 프로젝트에 포팅하기 위한 가이드입니다.

---

## 📁 폴더 구조

| 폴더 | 역할 |
|------|------|
| **Basic** | 기본 커스텀 위젯 (MaterialProgressBar) |
| **Common** | 공통 재사용 위젯 (TabList, ListView, WidgetFactory) |
| **Foundation** | 핵심 기반 (Button, ConfirmDialog, LoadingScreen) |
| **Frontend** | 프론트엔드 상태 관리 + 로딩스크린 + 세션 처리 |
| **Subsystem** | UIManagerSubsystem + UIMessaging |
| **IndicatorSystem** | 3D→2D 인디케이터 (체력바, 마커 등) |
| **PerformanceStats** | FPS/Ping 표시 위젯 |

---

## 상세 분석

### 1. Basic - 기본 위젯
- `RdMaterialProgressBar`: 머티리얼 기반 프로그레스바 (세그먼트, 그래프, 애니메이션)

### 2. Common - 공통 위젯
- `RdTabListWidgetBase`: 탭 리스트 관리
- `RdListView`: 커스텀 리스트뷰
- `RdWidgetFactory`: 위젯 동적 생성 팩토리

### 3. Foundation - 기초 클래스
- `RdButtonBase`: 버튼 베이스 클래스
- `RdConfirmationScreen`: 확인 다이얼로그
- `RdLoadingScreenSubsystem`: 로딩 스크린 관리

### 4. Frontend - 프론트엔드
- `RdFrontendStateComponent`: 로비 상태 머신 (ControlFlow 기반)
- `RdFrontendPerfSettingsAction`: 메뉴용 성능 설정

### 5. Subsystem - 서브시스템
- `RdUIManagerSubsystem`: UI 정책 관리
- `RdUIMessaging`: 다이얼로그 표시

### 6. IndicatorSystem - 인디케이터
- `RdIndicatorDescriptor`: 인디케이터 설정
- `SRdActorCanvas`: 렌더링 캔버스
- `RdIndicatorManagerComponent`: 액터별 관리

### 7. PerformanceStats - 성능 통계
- `RdPerfStatWidgetBase`: 통계 위젯
- `SRdLatencyGraph`: 지연 그래프

---

## 의존성

| 모듈 | 용도 |
|------|------|
| `CommonUI` | Activatable Widget, Input Routing |
| `CommonGame` | UIPolicy, UIManagerSubsystem |
| `CommonUser` | User 세션 관리 |
| `GameFeatures` | GameFeatureAction |
| `Slate/SlateCore` | SWidget 렌더링 |
