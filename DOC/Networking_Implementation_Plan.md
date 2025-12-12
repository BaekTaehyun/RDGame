# 네트워크 구현 계획 (GsNetworking & UI)

## 목표
`GsNetworking` 플러그인과 `DOC/Protocol.h`에 정의된 프로토콜을 사용하여 서버 통신 기능을 구현하고, 접속을 위한 로그인 UI를 제작합니다.

## 구현 단계

### 1. 프로토콜 정의 (Protocol Definition)
*   **파일 복사**: `DOC/Protocol.h` 파일을 `Source/RdGame/Network/Protocol.h`로 복사합니다.
*   **목적**: 게임(클라이언트) 모듈에서 패킷 구조체를 직접 사용할 수 있도록 합니다.

### 2. 로그인 UI 구현 (User Interface)
*   **새 클래스**: `ULoginWidget` (부모: `UUserWidget`)
*   **위치**:
    *   `Source/RdGame/UI/LoginWidget.h`
    *   `Source/RdGame/UI/LoginWidget.cpp`
*   **주요 기능**:
    *   **UI 요소**: IP 주소 입력창, 포트 입력창, 사용자 이름 입력창, 연결 버튼, 상태 텍스트.
    *   **로직**:
        1.  연결 버튼 클릭 시 `GsNetworkSubsystem`을 가져옵니다.
        2.  `Connect(IP, Port)` 함수를 호출합니다.
        3.  연결 성공 시 `C2S_LOGIN_REQ` 패킷(사용자 이름 포함)을 전송합니다.
        4.  `S2C_LOGIN_RES` 패킷 핸들러를 등록하여 응답을 대기합니다.
        5.  응답 수신 시 결과에 따라 상태 텍스트를 업데이트하거나 레벨을 전환합니다.

### 3. 네트워크 기능 연동 (Network Integration)
*   `GsNetworkSubsystem`의 `Connect`, `Send`, `RegisterHandler` API를 활용합니다.
*   `Protocol.h`의 `PacketHeader`, `Pkt_LoginReq`, `Pkt_LoginRes` 구조체를 사용하여 데이터를 직렬화합니다.

## 검증 계획
*   **컴파일 테스트**: 새로운 클래스와 구조체가 정상적으로 빌드되는지 확인합니다.
*   **기능 테스트**:
    *   에디터에서 위젯을 띄우고 "접속" 버튼을 눌렀을 때 크래시 없이 로직이 실행되는지 확인합니다.
    *   서버(더미 또는 실제)가 있다고 가정하고 패킷 전송 로직이 호출되는지 로그로 확인합니다.
