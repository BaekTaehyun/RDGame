#include "Utils/DungeonPCGHelper.h"
#include "Data/PCGPointData.h"
#include "PCGContext.h"
#include "Metadata/PCGMetadata.h"

UPCGPointData* UDungeonPCGHelper::GenerateDungeonPoints(const FDungeonGrid& Grid, ETileType TargetType, float TileSize)
{
	UPCGPointData* PointData = NewObject<UPCGPointData>();

	TArray<FPCGPoint>& Points = PointData->GetMutablePoints();

	int32 EstimatedCount = Grid.Width * Grid.Height / 4;
	Points.Reserve(EstimatedCount);

	FVector HalfSize(TileSize * 0.5f, TileSize * 0.5f, 100.0f);

	// Track visited for simple greedy merging (2x2)
	TArray<bool> Visited;
	Visited.SetNumZeroed(Grid.Width * Grid.Height);
	auto IsVisited = [&](int32 X, int32 Y) { return Visited[Y * Grid.Width + X]; };
	auto MarkVisited = [&](int32 X, int32 Y) { Visited[Y * Grid.Width + X] = true; };

	for (int32 Y = 0; Y < Grid.Height; ++Y)
	{
		for (int32 X = 0; X < Grid.Width; ++X)
		{
			if (IsVisited(X, Y)) continue;

			const FDungeonTile& Tile = Grid.GetTile(X, Y);
			if (Tile.Type != TargetType) continue;

			// Check for 2x2 Block (Only if not near edge)
			bool bIsLarge = false;
			if (X + 1 < Grid.Width && Y + 1 < Grid.Height)
			{
				const FDungeonTile& T1 = Grid.GetTile(X + 1, Y);
				const FDungeonTile& T2 = Grid.GetTile(X, Y + 1);
				const FDungeonTile& T3 = Grid.GetTile(X + 1, Y + 1);

				if (!IsVisited(X + 1, Y) && !IsVisited(X, Y + 1) && !IsVisited(X + 1, Y + 1) &&
					T1.Type == TargetType && T2.Type == TargetType && T3.Type == TargetType)
				{
					// Found 2x2 Block!
					bIsLarge = true;
					MarkVisited(X, Y);
					MarkVisited(X + 1, Y);
					MarkVisited(X, Y + 1);
					MarkVisited(X + 1, Y + 1);
				}
			}

			if (bIsLarge)
			{
				// Spawn Large Point @ Center of 2x2
				FPCGPoint& Point = Points.Emplace_GetRef();
				
				float WorldX = (X + 0.5f) * TileSize;
				float WorldY = (Y + 0.5f) * TileSize;
				
				Point.Transform.SetLocation(FVector(WorldX, WorldY, 0.0f));
				Point.Seed = FMath::Rand(); 
				Point.Density = 2.0f; // Use Density > 1 to mark "Large"
				Point.BoundsMin = -HalfSize * 2.0f;
				Point.BoundsMax = HalfSize * 2.0f;
				Point.Steepness = 1.0f;
			}
			else
			{
				// Normal Single Point
				MarkVisited(X, Y);
				
				FPCGPoint& Point = Points.Emplace_GetRef();
				
				float WorldX = X * TileSize;
				float WorldY = Y * TileSize;
				
				Point.Transform.SetLocation(FVector(WorldX, WorldY, 0.0f));
				Point.Seed = FMath::Rand(); 
				Point.Density = 1.0f; // Normal = 1.0
				Point.BoundsMin = -HalfSize;
				Point.BoundsMax = HalfSize;
				Point.Steepness = 1.0f;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DungeonPCG: Generated %d points for type %d"), Points.Num(), (int32)TargetType);
	return PointData;
}

void UDungeonPCGHelper::FillPCGComponent(UPCGComponent* Component, const FDungeonGrid& Grid)
{
	if (!Component) return;
	// Placeholder - data is passed via actor properties
}
