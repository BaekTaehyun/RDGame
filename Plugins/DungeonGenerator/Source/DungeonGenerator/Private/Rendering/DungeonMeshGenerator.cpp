#include "Rendering/DungeonMeshGenerator.h"

UDungeonMeshGenerator::UDungeonMeshGenerator() {
  TileSize = 100.0f;
  WallHeight = 300.0f;
  CurveSmoothing = 0.5f;
  bGenerateCeiling = true;
  bGenerateFloor = true;
}

void UDungeonMeshGenerator::GenerateCaveWalls(
    const FDungeonGrid &Grid, UProceduralMeshComponent *WallMeshComponent,
    UProceduralMeshComponent *FloorMeshComponent,
    UProceduralMeshComponent *CeilingMeshComponent) {
  if (!WallMeshComponent) {
    UE_LOG(LogTemp, Error,
           TEXT("DungeonMeshGenerator: WallMeshComponent is null"));
    return;
  }

  // 기존 메시 클리어
  WallMeshComponent->ClearAllMeshSections();

  TArray<FVector> Vertices;
  TArray<int32> Triangles;
  TArray<FVector2D> UVs;
  TArray<FVector> Normals;

  // Marching Squares 적용
  for (int32 Y = 0; Y < Grid.Height - 1; Y++) {
    for (int32 X = 0; X < Grid.Width - 1; X++) {
      uint8 Config = GetMarchingSquareConfig(X, Y, Grid);
      if (Config > 0 && Config < 15) // 0과 15는 완전 비어있거나 꽉 찬 케이스
      {
        GenerateMarchingSquareGeometry(Vertices, Triangles, UVs, Normals, X, Y,
                                       Config);
      }
    }
  }

  // 노멀 스무딩
  ApplySmoothNormals(Normals, Vertices, Triangles);

  // 메시 생성
  if (Vertices.Num() > 0) {
    TArray<FColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    WallMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs,
                                         VertexColors, Tangents, true);

    if (CaveWallMaterial) {
      WallMeshComponent->SetMaterial(0, CaveWallMaterial);
    }

    UE_LOG(LogTemp, Log,
           TEXT("DungeonMeshGenerator: Generated cave walls with %d vertices"),
           Vertices.Num());
  }

  // 바닥 생성
  if (bGenerateFloor && FloorMeshComponent) {
    GenerateFloorMesh(Grid, FloorMeshComponent);
  }

  // 천장 생성
  if (bGenerateCeiling && CeilingMeshComponent) {
    GenerateCeilingMesh(Grid, CeilingMeshComponent);
  }
}

uint8 UDungeonMeshGenerator::GetMarchingSquareConfig(
    int32 X, int32 Y, const FDungeonGrid &Grid) const {
  uint8 Config = 0;

  // 4개 코너 검사
  if (IsWall(X, Y, Grid))
    Config |= 1; // BottomLeft  (bit 0)
  if (IsWall(X + 1, Y, Grid))
    Config |= 2; // BottomRight (bit 1)
  if (IsWall(X + 1, Y + 1, Grid))
    Config |= 4; // TopRight    (bit 2)
  if (IsWall(X, Y + 1, Grid))
    Config |= 8; // TopLeft     (bit 3)

  return Config; // 0~15
}

void UDungeonMeshGenerator::GenerateMarchingSquareGeometry(
    TArray<FVector> &Vertices, TArray<int32> &Triangles, TArray<FVector2D> &UVs,
    TArray<FVector> &Normals, int32 X, int32 Y, uint8 Config) {
  // 기준 위치
  FVector BasePos(X * TileSize, Y * TileSize, 0.0f);

  // 4개 코너 정점
  FVector BL = BasePos;                                  // BottomLeft
  FVector BR = BasePos + FVector(TileSize, 0, 0);        // BottomRight
  FVector TR = BasePos + FVector(TileSize, TileSize, 0); // TopRight
  FVector TL = BasePos + FVector(0, TileSize, 0);        // TopLeft

  // 엣지 중간점 (보간 가능)
  FVector Bot = GetInterpolatedVertex(X, Y, false, CurveSmoothing);
  FVector Right = GetInterpolatedVertex(X + 1, Y, true, CurveSmoothing);
  FVector Top = GetInterpolatedVertex(X, Y + 1, false, CurveSmoothing);
  FVector Left = GetInterpolatedVertex(X, Y, true, CurveSmoothing);

  int32 BaseIndex = Vertices.Num();

  // Marching Squares 16가지 케이스
  // 간소화: 대표적인 케이스만 구현
  switch (Config) {
  case 1: // ◣ (BL만)
    Vertices.Append({BL, Bot, Left});
    break;

  case 2: // ◢ (BR만)
    Vertices.Append({BR, Right, Bot});
    break;

  case 3: // ▬ (하단)
    Vertices.Append({BL, BR, Right, Left});
    break;

  case 4: // ◥ (TR만)
    Vertices.Append({TR, Top, Right});
    break;

  case 6: // ▐ (우측)
    Vertices.Append({BR, TR, Top, Bot});
    break;

  case 8: // ◤ (TL만)
    Vertices.Append({TL, Left, Top});
    break;

  case 9: // ▌ (좌측)
    Vertices.Append({BL, Bot, Top, TL});
    break;

  case 12: // ▀ (상단)
    Vertices.Append({TL, TR, Right, Left});
    break;

  default:
    // 기타 케이스는 기본 쿼드
    Vertices.Append({BL, BR, TR, TL});
    break;
  }

  // 트라이앵글 추가 (quad = 2 triangles)
  int32 VertCount = Vertices.Num() - BaseIndex;
  if (VertCount == 3) {
    Triangles.Append({BaseIndex, BaseIndex + 1, BaseIndex + 2});
  } else if (VertCount == 4) {
    Triangles.Append({BaseIndex, BaseIndex + 1, BaseIndex + 2});
    Triangles.Append({BaseIndex, BaseIndex + 2, BaseIndex + 3});
  }

  // UV 및 노멀 추가
  for (int32 i = 0; i < VertCount; i++) {
    UVs.Add(FVector2D(0, 0)); // 간소화
    Normals.Add(FVector::UpVector);
  }

  // 벽 높이 추가 (수직 extrude)
  int32 TopBaseIndex = Vertices.Num();
  for (int32 i = 0; i < VertCount; i++) {
    FVector TopVert = Vertices[BaseIndex + i];
    TopVert.Z += WallHeight;
    Vertices.Add(TopVert);
    UVs.Add(FVector2D(0, 1));
    Normals.Add(FVector::UpVector);
  }

  // 측면 트라이앵글 추가
  for (int32 i = 0; i < VertCount; i++) {
    int32 Next = (i + 1) % VertCount;
    Triangles.Append({BaseIndex + i, TopBaseIndex + i, TopBaseIndex + Next,
                      BaseIndex + i, TopBaseIndex + Next, BaseIndex + Next});
  }
}

void UDungeonMeshGenerator::ApplySmoothNormals(TArray<FVector> &Normals,
                                               const TArray<FVector> &Vertices,
                                               const TArray<int32> &Triangles) {
  // 간소화된 노멀 스무딩
  if (Normals.Num() != Vertices.Num()) {
    return;
  }

  TArray<FVector> SmoothNormals;
  SmoothNormals.SetNumZeroed(Vertices.Num());

  // 트라이앵글 노멀 누적
  for (int32 i = 0; i < Triangles.Num(); i += 3) {
    FVector V0 = Vertices[Triangles[i]];
    FVector V1 = Vertices[Triangles[i + 1]];
    FVector V2 = Vertices[Triangles[i + 2]];

    FVector Normal = FVector::CrossProduct(V1 - V0, V2 - V0).GetSafeNormal();

    SmoothNormals[Triangles[i]] += Normal;
    SmoothNormals[Triangles[i + 1]] += Normal;
    SmoothNormals[Triangles[i + 2]] += Normal;
  }

  // 평균화
  for (int32 i = 0; i < SmoothNormals.Num(); i++) {
    Normals[i] = SmoothNormals[i].GetSafeNormal();
  }
}

void UDungeonMeshGenerator::GenerateFloorMesh(
    const FDungeonGrid &Grid, UProceduralMeshComponent *FloorMeshComponent) {
  if (!FloorMeshComponent) {
    return;
  }

  FloorMeshComponent->ClearAllMeshSections();

  TArray<FVector> Vertices;
  TArray<int32> Triangles;
  TArray<FVector2D> UVs;

  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
        int32 BaseIndex = Vertices.Num();

        Vertices.Append({FVector(X * TileSize, Y * TileSize, 0),
                         FVector((X + 1) * TileSize, Y * TileSize, 0),
                         FVector((X + 1) * TileSize, (Y + 1) * TileSize, 0),
                         FVector(X * TileSize, (Y + 1) * TileSize, 0)});

        Triangles.Append({BaseIndex, BaseIndex + 1, BaseIndex + 2, BaseIndex,
                          BaseIndex + 2, BaseIndex + 3});

        UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1),
                    FVector2D(0, 1)});
      }
    }
  }

  if (Vertices.Num() > 0) {
    TArray<FVector> Normals;
    Normals.Init(FVector::UpVector, Vertices.Num());

    FloorMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs,
                                          TArray<FColor>(),
                                          TArray<FProcMeshTangent>(), true);

    if (CaveFloorMaterial) {
      FloorMeshComponent->SetMaterial(0, CaveFloorMaterial);
    }
  }
}

void UDungeonMeshGenerator::GenerateCeilingMesh(
    const FDungeonGrid &Grid, UProceduralMeshComponent *CeilingMeshComponent) {
  // 바닥과 동일하되 Z 위치만 다름
  if (!CeilingMeshComponent) {
    return;
  }

  CeilingMeshComponent->ClearAllMeshSections();

  TArray<FVector> Vertices;
  TArray<int32> Triangles;
  TArray<FVector2D> UVs;

  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
        int32 BaseIndex = Vertices.Num();

        Vertices.Append(
            {FVector(X * TileSize, Y * TileSize, WallHeight),
             FVector((X + 1) * TileSize, Y * TileSize, WallHeight),
             FVector((X + 1) * TileSize, (Y + 1) * TileSize, WallHeight),
             FVector(X * TileSize, (Y + 1) * TileSize, WallHeight)});

        // 천장은 아래에서 보이도록 winding order 반대
        Triangles.Append({BaseIndex, BaseIndex + 2, BaseIndex + 1, BaseIndex,
                          BaseIndex + 3, BaseIndex + 2});

        UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1),
                    FVector2D(0, 1)});
      }
    }
  }

  if (Vertices.Num() > 0) {
    TArray<FVector> Normals;
    Normals.Init(FVector::DownVector, Vertices.Num());

    CeilingMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals,
                                            UVs, TArray<FColor>(),
                                            TArray<FProcMeshTangent>(), true);

    if (CaveCeilingMaterial) {
      CeilingMeshComponent->SetMaterial(0, CaveCeilingMaterial);
    }
  }
}

bool UDungeonMeshGenerator::IsWall(int32 X, int32 Y,
                                   const FDungeonGrid &Grid) const {
  if (!Grid.IsValid(X, Y)) {
    return true;
  }

  return Grid.GetTile(X, Y).Type == ETileType::Wall;
}

FVector UDungeonMeshGenerator::GetInterpolatedVertex(int32 X, int32 Y,
                                                     bool bVertical,
                                                     float T) const {
  if (bVertical) {
    return FVector(X * TileSize, (Y + T) * TileSize, 0);
  } else {
    return FVector((X + T) * TileSize, Y * TileSize, 0);
  }
}
