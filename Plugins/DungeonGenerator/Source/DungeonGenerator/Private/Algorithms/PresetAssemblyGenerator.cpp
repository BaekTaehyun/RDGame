#include "Algorithms/PresetAssemblyGenerator.h"
#include "DungeonGenerator/Public/DungeonLevelGenerator.h"

void UPresetAssemblyGenerator::Generate(FDungeonGrid &Grid,
                                        FRandomStream &Stream) {
  if (!ModuleDatabase) {
    UE_LOG(LogTemp, Error,
           TEXT("PresetAssemblyGenerator: Missing Module Database!"));
    return;
  }

  UE_LOG(LogTemp, Log,
         TEXT("PresetAssemblyGenerator: Starting generation... MaxRooms=%d"),
         MaxRoomCount);

  // 1. 초기화
  Grid.Init(Grid.Width, Grid.Height, ETileType::Wall);
  PlacedModules.Empty();

  // 확장 대기열 (Open List)
  TArray<FOpenSocketInfo> OpenSockets;

  // 2. 루트(Start) 모듈 배치
  if (!PlaceStartModule(Grid, Stream, OpenSockets)) {
    UE_LOG(LogTemp, Error,
           TEXT("PresetAssemblyGenerator: Failed to place Start Module! Check "
                "if 'Start' type modules exist in DB."));
    return;
  }

  // 3. 확장 루프 (Growing Tree)
  int32 CurrentRoomCount = 1;
  int32 Attempts = 0;
  const int32 MaxAttempts = MaxRoomCount * 20; // 충분한 시도 횟수 부여

  while (OpenSockets.Num() > 0 && CurrentRoomCount < MaxRoomCount &&
         Attempts < MaxAttempts) {
    Attempts++;

    // 전략: 랜덤 선택 (Prim's Algorithm 스타일로 하면 중앙 집중되고,
    // 최신것(DFS)으로 하면 길게 뻗음) 여기서는 완전 랜덤 선택으로 다양한 모양
    // 유도
    int32 SocketIndex = Stream.RandRange(0, OpenSockets.Num() - 1);
    FOpenSocketInfo TargetSocket = OpenSockets[SocketIndex];

    // 시도 후에는 성공하든 실패하든 일단 리스트에서 제거 (실패면 막다른 길
    // 처리하거나 그냥 벽으로 둠) 단, 실패 시 다른 모듈을 시도해볼 수 있도록
    // 로직을 짤 수도 있지만, 단순화를 위해 Pop. 더 나은 품질을 위해선 'Retry'
    // 로직이 필요. 여기서는 TryPlaceNextModule이 내부적으로 여러 후보를 시도함.
    OpenSockets.RemoveAt(SocketIndex);

    TArray<FOpenSocketInfo> NewSockets;
    if (TryPlaceNextModule(Grid, Stream, TargetSocket, NewSockets)) {
      OpenSockets.Append(NewSockets);
      CurrentRoomCount++;
    } else {
      // 배치 실패!
      // TODO: 해당 소켓 위치를 'DeadEnd' (EndCap) 모듈로 막아야 함.
      // 일단은 그냥 뚫린 채로(Grid상으론 Wall 인접) 둠.
      // (V2에서 EndCap 로직 추가 가능)
    }
  }

  UE_LOG(
      LogTemp, Log,
      TEXT("PresetAssemblyGenerator: Complete. Rooms: %d, Placed Modules: %d"),
      CurrentRoomCount, PlacedModules.Num());
}

bool UPresetAssemblyGenerator::PlaceStartModule(
    FDungeonGrid &Grid, FRandomStream &Stream,
    TArray<FOpenSocketInfo> &OutOpenSockets) {
  // DB에서 Start 타입 찾기
  TArray<FModuleData> StartModules =
      ModuleDatabase->GetModulesByType(EDungeonPresetModuleType::Start);
  if (StartModules.Num() == 0)
    return false;

  // 랜덤 선택
  const FModuleData &SelectedModule =
      StartModules[Stream.RandRange(0, StartModules.Num() - 1)];

  // 그리드 중앙에 배치
  FIntPoint CenterPos(Grid.Width / 2 - SelectedModule.Size.X / 2,
                      Grid.Height / 2 - SelectedModule.Size.Y / 2);

  if (CanPlaceModule(Grid, SelectedModule, CenterPos)) {
    StampModuleToGrid(Grid, SelectedModule, CenterPos, OutOpenSockets);
    return true;
  }

  return false;
}

bool UPresetAssemblyGenerator::TryPlaceNextModule(
    FDungeonGrid &Grid, FRandomStream &Stream,
    const FOpenSocketInfo &TargetSocket,
    TArray<FOpenSocketInfo> &OutOpenSockets) {
  // 1. 매칭 조건 설정
  // 반대 방향이어야 함 (North -> South)
  EModuleSocketDirection RequiredDir =
      GetOppositeDirection(TargetSocket.SocketData.Direction);
  FName RequiredTag =
      TargetSocket.SocketData.SocketTag; // 태그 매칭 (일단 동일 태그)

  // 2. 후보군 검색
  // 모든 모듈을 뒤져서 조건에 맞는 '소켓'을 가진 모듈 찾기
  struct FCandidate {
    const FModuleData *Module;
    int32 SocketIndex;
  };
  TArray<FCandidate> Candidates;

  for (const FModuleData &Module : ModuleDatabase->Modules) {
    // Start 모듈은 다시 나오지 않도록 제외 (선택 사항)
    if (Module.Type == EDungeonPresetModuleType::Start)
      continue;

    for (int32 i = 0; i < Module.Sockets.Num(); i++) {
      const FModuleSocket &Socket = Module.Sockets[i];

      // 방향 체크
      if (Socket.Direction == RequiredDir) {
        // 태그 체크 (둘 중 하나가 None이면 호환, 아니면 같아야 함)
        bool bTagMatch = (RequiredTag.IsNone() || Socket.SocketTag.IsNone() ||
                          RequiredTag == Socket.SocketTag);
        if (bTagMatch) {
          Candidates.Add({&Module, i});
        }
      }
    }
  }

  if (Candidates.Num() == 0)
    return false;

  // 3. 셔플 (랜덤성)
  // Fisher-Yates Shuffle
  for (int32 i = Candidates.Num() - 1; i > 0; i--) {
    int32 j = Stream.RandRange(0, i);
    Candidates.Swap(i, j);
  }

  // 4. 배치 시도
  for (const FCandidate &Cand : Candidates) {
    const FModuleData *Mod = Cand.Module;
    const FModuleSocket &CandSocket = Mod->Sockets[Cand.SocketIndex];

    // 위치 계산
    // 공식: TargetWorldPos + WalkVector - CandidateLocalPos = NewOrigin
    // 1) Target Socket에서 **한 칸 전진**하여 '새로운 모듈의 소켓 위치'를 구함
    FIntPoint ConnectionWorldPos =
        TargetSocket.WorldPosition +
        GetDirectionOffset(TargetSocket.SocketData.Direction);

    // 2) 그 위치가 새 모듈의 소켓(CandSocket) 위치가 되도록 원점 역산
    // ConnectionWorldPos = NewOrigin + CandSocket.LocalPosition
    // 따라서 NewOrigin = ConnectionWorldPos - CandSocket.LocalPosition
    FIntPoint NewOrigin = ConnectionWorldPos - CandSocket.LocalPosition;

    if (CanPlaceModule(Grid, *Mod, NewOrigin)) {
      // 배치 성공!
      StampModuleToGrid(Grid, *Mod, NewOrigin, OutOpenSockets);

      // 주의: 방금 연결된 소켓(Cand.SocketIndex)은 OpenSockets에 들어가면 안
      // 됨. StampModuleToGrid 내부에서 처리하거나, 반환된 후 제거해야 함. 현재
      // StampModuleToGrid 구현 상 모든 소켓을 반환하므로, 여기서 연결된 소켓을
      // 찾아 제거.

      // 연결된 소켓 찾아서 제거 (로컬 위치가 CandSocket의 로컬 위치와 같은 것)
      for (int32 k = 0; k < OutOpenSockets.Num(); k++) {
        if (OutOpenSockets[k].SocketData.LocalPosition ==
            CandSocket.LocalPosition) {
          OutOpenSockets.RemoveAt(k);
          break;
        }
      }

      // 로그
      // UE_LOG(LogTemp, Log, TEXT("Connected %s to %s"),
      // *Mod->ModuleID.ToString(),
      // *TargetSocket.SocketData.SocketTag.ToString());

      return true;
    }
  }

  return false;
}

bool UPresetAssemblyGenerator::CanPlaceModule(const FDungeonGrid &Grid,
                                              const FModuleData &Module,
                                              FIntPoint Position) {
  // 범위 검사
  if (Position.X < 0 || Position.Y < 0 ||
      Position.X + Module.Size.X > Grid.Width ||
      Position.Y + Module.Size.Y > Grid.Height) {
    return false;
  }

  // 겹침 검사
  for (int32 Y = 0; Y < Module.Size.Y; Y++) {
    for (int32 X = 0; X < Module.Size.X; X++) {
      // 모듈 내부가 '꽉 찬' 공간이라고 가정 (직사각형)
      // 실제 데이터가 복잡하면 Layout 텍스처 등을 참조해야 함

      // Grid에서 Wall이 아니면 이미 점유된 것
      if (Grid.GetTile(Position.X + X, Position.Y + Y).Type !=
          ETileType::Wall) {
        return false;
      }
    }
  }
  return true;
}

void UPresetAssemblyGenerator::StampModuleToGrid(
    FDungeonGrid &Grid, const FModuleData &Module, FIntPoint Position,
    TArray<FOpenSocketInfo> &OutNewSockets) {
  // 1. 그리드 업데이트
  for (int32 Y = 0; Y < Module.Size.Y; Y++) {
    for (int32 X = 0; X < Module.Size.X; X++) {
      FDungeonTile &Tile = Grid.GetTile(Position.X + X, Position.Y + Y);
      Tile.Type = ETileType::Floor; // 일단 바닥으로
      // RoomID를 어떻게 저장할까? Module.ID는 FName인데 Tile.RoomID는 int.
      // 임시로 해시값이나 인덱스를 넣을 수 있음. 여기선 생략.
    }
  }

  // 2. 배치 정보 기록
  PlacedModules.Add({Module.ModuleID, Position, Module.Size});

  // 3. 소켓 정보 반환 (월드 좌표 계산)
  for (const FModuleSocket &Socket : Module.Sockets) {
    FOpenSocketInfo Info;
    Info.SocketData = Socket;
    Info.WorldPosition = Position + Socket.LocalPosition;
    OutNewSockets.Add(Info);
  }
}

EModuleSocketDirection
UPresetAssemblyGenerator::GetOppositeDirection(EModuleSocketDirection Dir) {
  switch (Dir) {
  case EModuleSocketDirection::North:
    return EModuleSocketDirection::South;
  case EModuleSocketDirection::East:
    return EModuleSocketDirection::West;
  case EModuleSocketDirection::South:
    return EModuleSocketDirection::North;
  case EModuleSocketDirection::West:
    return EModuleSocketDirection::East;
  default:
    return EModuleSocketDirection::North;
  }
}

FIntPoint
UPresetAssemblyGenerator::GetDirectionOffset(EModuleSocketDirection Dir) {
  switch (Dir) {
  case EModuleSocketDirection::North:
    return FIntPoint(0, 1);
  case EModuleSocketDirection::East:
    return FIntPoint(1, 0);
  case EModuleSocketDirection::South:
    return FIntPoint(0, -1);
  case EModuleSocketDirection::West:
    return FIntPoint(-1, 0);
  default:
    return FIntPoint(0, 0);
  }
}
