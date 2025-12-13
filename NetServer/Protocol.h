#pragma once
#include <cstdint>

// 패킷 타입 정의
enum class PacketType : uint16_t {
  NONE = 0,
  C2S_LOGIN_REQ = 1,      // 클라 -> 서버: 로그인 요청
  S2C_LOGIN_RES = 2,      // 서버 -> 클라: 로그인 결과 (내 ID 할당)
  C2S_MOVE_UPDATE = 3,    // 클라 -> 서버: 나 이동해요 (좌표, 속도)
  S2C_MOVE_BROADCAST = 4, // 서버 -> 클라: 쟤 이동한대 (브로드캐스팅)
  C2S_ATTACK = 5,         // 클라 -> 서버: 공격!
  S2C_ATTACK_BROADCAST = 6,
  S2C_USER_ENTER = 7, // 서버 -> 클라: 유저 입장 (내 정보 포함, 타인 정보 포함)
  S2C_USER_LEAVE = 8  // 서버 -> 클라: 유저 퇴장
};

#pragma pack(push, 1) // 바이트 정렬 (네트워크 전송용)

// 모든 패킷의 공통 헤더
struct PacketHeader {
  uint16_t size; // 패킷 전체 크기 (헤더 포함)
  uint16_t type; // PacketType
};

// [로그인] 요청: 간단히 유저 이름만 전송
struct Pkt_LoginReq : public PacketHeader {
  char username[32];
};

// [로그인] 응답: 서버가 부여한 SessionID 전송
struct Pkt_LoginRes : public PacketHeader {
  uint32_t mySessionId;
  bool success;
};

// [이동] 데드 레코닝을 위한 데이터 구조
struct Pkt_MoveUpdate : public PacketHeader {
  uint32_t sessionId; // 누가? (서버가 브로드캐스팅 할 때 채움)
  float x, y, z;      // 현재 위치 P_current
  float vx, vy, vz;   // 현재 속도 Velocity
  float pitch;        // 회전 (Pitch) +
  float yaw;          // 회전 (Yaw)
  float roll;         // 회전 (Roll) +
  uint64_t timestamp; // 보낸 시간 (랙 보상용)
};

// [전투] 공격 패킷
struct Pkt_Attack : public PacketHeader {
  uint32_t sessionId; // 누가 공격했나
  uint64_t timestamp; // 언제?
                      // 필요하다면 타겟 ID나 스킬 ID 등 추가
};

// [유저 관리] 입장 알림
struct Pkt_UserEnter : public PacketHeader {
  uint32_t sessionId;
  float x, y, z;
  float yaw;
};

// [유저 관리] 퇴장 알림
struct Pkt_UserLeave : public PacketHeader {
  uint32_t sessionId;
};

#pragma pack(pop)
