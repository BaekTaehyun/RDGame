# C++ TCP Server Implementation Plan

이 문서는 `SimpleMMO_Server.md` 가이드를 바탕으로 **이동/전투 동기화 연구용 TCP 서버**를 구축하기 위한 계획입니다.

## 🎯 목표
- **TCP 브로드캐스트 서버 구축**: 클라이언트 간 패킷 중계.
- **프로토콜 정의**: 로그인, 이동, 전투 패킷 구조체화.
- **간편한 빌드**: CMake를 이용한 빌드 시스템 구성.

## 📋 진행 단계

### 1단계: 프로토콜 정의 (Protocol Definition)
- **파일**: `Protocol.h`
- **내용**: 
  - `PacketHeader`, `PacketType` 정의
  - 로그인 (`Login`), 이동 (`MoveUpdate`), 전투 (`Attack`) 구조체 정의
  - 1바이트 패킹 (`#pragma pack`) 적용

### 2단계: 서버 핵심 로직 구현 (Server Core)
- **파일**: `main.cpp`
- **내용**:
  - `WinSock2` 초기화 및 리슨 소켓 설정
  - `std::thread`를 이용한 1 클라이언트 1 스레드 모델 (프로토타이핑용)
  - `ClientHandler`: 헤더/바디 파싱 로직 및 브로드캐스팅 (`BroadcastPacket`)

### 3단계: 빌드 시스템 구성 (Build System)
- **파일**: `CMakeLists.txt`
- **내용**: 
  - Windows 환경 (`ws2_32` 라이브러리 링크) 설정
  - 표준 C++17 설정

### 4단계: 검증 (Verification)
- 빌드 후 서버 실행 테스트 (`Listening on port 9000...` 확인)
- (추후 클라이언트 연결 테스트 가능)

## 📂 파일 구조
```
NetServer/
├── DOC/
│   ├── SimpleMMO_Server.md   (가이드)
│   └── implementation_plan.md (본 문서)
├── Protocol.h               (프로토콜)
├── main.cpp                 (메인 서버 로직)
└── CMakeLists.txt           (빌드 설정)
```
