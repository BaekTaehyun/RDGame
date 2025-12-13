# 고도화된 네트워크 동기화 기법 TODO

## 1. Client-Side Prediction & Reconciliation (클라이언트 예측 및 보정)
**상태:** `SenderStrategy`에 예측(Prediction)은 언리얼 `CharacterMovementComponent`를 통해 기본적으로 적용되어 있으나, 서버 보정(Reconciliation) 로직은 미구현 상태.

### 구현 계획
1.  **History Buffer:** 클라이언트는 보낸 패킷(`Input` + `Timestamp` + `ResultLoc`)을 `SavedMoves` 리스트에 저장.
2.  **Server Ack:** 서버로부터 "너 이 시간에 여기 있었음"이라는 Ack 패킷 수신.
3.  **Correction:** 
    *   Ack된 위치와 내가 저장해둔 그 시간의 위치(`SavedMove`)를 비교.
    *   오차가 허용 범위(예: 5cm) 이내면 `SavedMoves`에서 해당 시간 이전 데이터를 삭제.
    *   오차가 크면 **강제 위치 수정(Snap)** 후, 그 시점부터 현재까지의 `SavedMoves`를 **재시뮬레이션(Replay)**하여 현재 위치를 재계산.

---

## 2. Entity Interpolation with State Buffer (상태 버퍼 보간)
**상태:** `ReceiverStrategy`에서 현재 위치와 목표 위치 간의 단순 `VInterpTo`만 구현됨. 핑이 튀면 움직임이 끊길 수 있음.

### 구현 계획
1.  **State Buffer:** 수신된 패킷(`Location`, `Rotation`, `Timestamp`)을 큐(Queue/Buffer)에 저장.
2.  **Render Time:** 현재 시간보다 일정 시간(예: `InterpolationDelay` = 100ms) 과거를 렌더링 시간(`RenderTime`)으로 설정.
3.  **Interpolation:**
    *   버퍼에서 `RenderTime` 앞뒤의 두 스냅샷(`Prev`, `Next`)을 찾음.
    *   `Alpha = (RenderTime - Prev.Time) / (Next.Time - Prev.Time)` 계산.
    *   두 스냅샷 사이를 정확하게 보간(`Lerp`/`Slerp`)하여 부드러워짐.
    *   패킷이 늦게 와도 버퍼에 여유가 있다면 끊김 없음.

---

## 3. Dead Reckoning (추측 항법)
**상태:** 현재는 패킷이 올 때까지 이전 목표점에 멈추거나 단순 보간만 수행. 패킷 손실 시 멈칫거림 발생.

### 구현 계획
1.  **Velocity 활용:** `Pkt_MoveUpdate`에 포함된 `Velocity`를 활용.
2.  **Extrapolation:**
    *   다음 패킷이 도착하지 않았을 때, `P = P_last + V_last * DeltaTime` 공식으로 미래 위치를 예측하여 계속 이동시킴.
    *   최대 예측 시간(예: 500ms)을 두어 무한정 날아가는 것 방지.
    *   새 패킷 도착 시 예측 위치와 실제 위치 간의 오차를 부드럽게 보정(`Decay`).

---

## 4. Server-Side Lag Compensation (랙 보상 히트 판정)
**상태:** 미구현. 움직이는 적을 맞출 때, 클라이언트는 맞췄다고 생각하지만 서버는 이미 적이 이동한 뒤라 안 맞는 문제 발생 가능.

### 구현 계획
1.  **Hit History:** 서버는 모든 캐릭터의 과거 위치(지난 1초 분량)를 히스토리 버퍼에 저장.
2.  **Time Rewind:** 
    *   공격 패킷에 담긴 클라이언트의 `AttackTimestamp`를 확인.
    *   서버는 해당 타임스탬프 시점(RTT/2 고려)으로 세상(적 캐릭터들)의 위치를 되돌림.
    *   그 상태에서 충돌 판정(Raycast) 수행.
    *   판정 후 다시 원상 복구.

---

## 5. Clock Synchronization (시간 동기화)
**상태:** 현재 `FPlatformTime::Seconds()`를 쓰거나 로컬 시간 의존. 클라/서버 간 절대 시간 기준이 다름.

### 구현 계획
1.  **Clock Sync Packet:** 
    *   클라 -> 서버: `Ping(T1)` 전송.
    *   서버 -> 클라: `Pong(T1, T2_ServerTime)` 전송.
    *   클라: 도착 시간(`T3`) 체크. `RTT = T3 - T1`.
    *   `ServerTimeOffset = T2 - T1 - (RTT / 2)`.
2.  **Usage:** 모든 타임스탬프 처리 시 `GetLocalTime() + ServerTimeOffset`을 사용하여 서버 시간 기준으로 통일.

---

## 6. Adaptive Interpolation Delay (적응형 보간 지연)
**상태:** 고정된 값 사용 시, 네트워크가 좋으면 딜레이가 느껴지고 나쁘면 끊김.

### 구현 계획
1.  **Jitter Measurement:** 패킷 도착 간격의 편차(Jitter)를 실시간 측정.
2.  **Dynamic Adjustment:**
    *   Jitter가 적으면 `InterpolationDelay`를 줄여 반응성 향상.
    *   Jitter가 크면 `InterpolationDelay`를 늘려 끊김 방지(Smoothness 우선).
