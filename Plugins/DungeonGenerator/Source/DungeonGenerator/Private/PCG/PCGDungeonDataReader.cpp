#include "PCG/PCGDungeonDataReader.h"
#include "PCGContext.h"
#include "PCGComponent.h"
#include "DungeonLevelGenerator.h" // ADungeonLevelGenerator
#include "Components/DungeonRendererComponent.h"
#include "Rendering/DungeonTileRenderer.h"
#include "Data/DungeonThemeAsset.h"
#include "Helpers/PCGHelpers.h"
#include "Data/PCGPointData.h"
#include "LandscapeProxy.h"
#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGDungeonDataReader)

#define LOCTEXT_NAMESPACE "PCGDungeonDataReaderElement"

UPCGDungeonDataReaderSettings::UPCGDungeonDataReaderSettings()
{
}

#if WITH_EDITOR
FText UPCGDungeonDataReaderSettings::GetNodeTooltipText() const
{
	return LOCTEXT("PCGDungeonDataReaderTooltip", "Reads the Dungeon Generator's Grid and creates points for specific tile types.");
}

FLinearColor UPCGDungeonDataReaderSettings::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.4f, 0.8f); // Blue-ish
}

void UPCGDungeonDataReaderSettings::ApplyDeprecation(UPCGNode* InNode)
{
	Super::ApplyDeprecation(InNode);
}
#endif

FString UPCGDungeonDataReaderSettings::GetAdditionalTitleInformation() const
{
	switch (TargetTileType)
	{
	case EPCGDungeonTileFilter::Floor: return TEXT("Floor");
	case EPCGDungeonTileFilter::Wall: return TEXT("Wall");
	case EPCGDungeonTileFilter::Corridor: return TEXT("Corridor");
	case EPCGDungeonTileFilter::Door: return TEXT("Door");
	case EPCGDungeonTileFilter::Walkable: return TEXT("Walkable");
	default: return TEXT("All");
	}
}

TArray<FPCGPinProperties> UPCGDungeonDataReaderSettings::InputPinProperties() const
{
	// This is a Source node, so no inputs required.
	return TArray<FPCGPinProperties>();
}



TArray<FPCGPinProperties> UPCGDungeonDataReaderSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);
	return PinProperties;
}

FPCGElementPtr UPCGDungeonDataReaderSettings::CreateElement() const
{
	return MakeShared<FPCGDungeonDataReaderElement>();
}

// Helper to check if a tile type is walkable (Floor, Corridor, Stair, Door)
static bool IsWalkableTile(ETileType Type)
{
	return Type == ETileType::Floor || Type == ETileType::Corridor || 
	       Type == ETileType::Stair || Type == ETileType::Door;
}

bool FPCGDungeonDataReaderElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGDungeonDataReaderElement::Execute);
	
	UE_LOG(LogTemp, Warning, TEXT("PCGDungeonDataReader::ExecuteInternal called (Element Execution Start)"));

	check(Context); 
	const UPCGDungeonDataReaderSettings* Settings = Context->GetInputSettings<UPCGDungeonDataReaderSettings>();
	check(Settings);

	// 1. Find Dungeon Generator Actor (any actor with DungeonRendererComponent)
	// First try the target actor from context
	AActor* TargetActor = Context->GetTargetActor(nullptr);
	UDungeonRendererComponent* RendererComp = nullptr;
	
	if (TargetActor)
	{
		RendererComp = TargetActor->FindComponentByClass<UDungeonRendererComponent>();
	}
	
	// If not found on target actor, try to find via the PCGSubsystem's cached reference
	// NOTE: We cannot use TActorIterator here because PCG nodes run on worker threads!
	// Instead, we use a static cached reference that was set when the dungeon was generated.
	if (!RendererComp)
	{
		// Try to get from static cache (set by DungeonWorldBuilder during generation)
		RendererComp = UDungeonRendererComponent::GetLastActiveRenderer();
		if (RendererComp)
		{
			TargetActor = RendererComp->GetOwner();
			UE_LOG(LogTemp, Log, TEXT("PCGDungeonDataReader: Found DungeonRendererComponent via static cache on %s"), 
				TargetActor ? *TargetActor->GetName() : TEXT("(null)"));
		}
	}
	
	if (!RendererComp)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("MissingRenderer", "No DungeonRendererComponent found. Generate Dungeon first."));
		return true;
	}
	// Access Centrally Cached Grid from Component
	const FDungeonGrid* GridPtr = RendererComp->GetCachedGrid();
	if (!GridPtr)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("NoGridData", "Dungeon Grid data is not available. Generate Dungeon first."));
		return true;
	}

	const FDungeonGrid& Grid = *GridPtr;

	UE_LOG(LogTemp, Warning, TEXT("PCGReader: Reading Grid. Size=%dx%d, TargetActor=%s"), 
		Grid.Width, Grid.Height, *TargetActor->GetName());

	if (Grid.Width == 0 || Grid.Height == 0)
	{
		// Grid not valid yet
		return true;
	}

	// 3. Create Output Data
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
	UPCGPointData* PointData = NewObject<UPCGPointData>();
	PointData->InitializeFromData(nullptr); // Empty init
	Outputs.Emplace_GetRef().Data = PointData;

	TArray<FPCGPoint>& Points = PointData->GetMutablePoints();
	
	// 4. Iterate Grid and Create Points
	// Get TileSize from Theme (consistent with Landscape generation)
	const UDungeonThemeAsset* Theme = RendererComp->GetCachedTheme();
	double TileSize = Theme ? Theme->TileSize : 100.0;
	
	// NOTE: Cannot use TActorIterator here because PCG nodes run on worker threads!
	// Use cached Landscape world size from DungeonRendererComponent instead.
	FVector ActorLocation = FVector::ZeroVector;
	
	// Check for cached Landscape world size (set by DungeonLandscapeTool) - STATIC variable
	FVector2D CachedLandscapeSize = UDungeonRendererComponent::GetCachedLandscapeWorldSize();
	if (CachedLandscapeSize.X > 0.0 && Grid.Width > 0)
	{
		// Calculate actual TileSize from cached Landscape world size
		TileSize = CachedLandscapeSize.X / Grid.Width;
		UE_LOG(LogTemp, Log, TEXT("PCGDungeonDataReader: Using cached Landscape size. EffectiveTileSize=%.2f (LandscapeSize=%.1f / GridWidth=%d)"),
			TileSize, CachedLandscapeSize.X, Grid.Width);
	}
	
	if (!Theme)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("MissingTheme", "Theme not cached."));
	}
	
	// Debug: Count tile types
	int32 CountNone = 0, CountFloor = 0, CountWall = 0, CountCorridor = 0, CountDoor = 0;
	for (int32 i = 0; i < Grid.Width * Grid.Height; i++)
	{
		ETileType Type = Grid.Tiles[i].Type;
		switch (Type)
		{
			case ETileType::None: CountNone++; break;
			case ETileType::Floor: CountFloor++; break;
			case ETileType::Wall: CountWall++; break;
			case ETileType::Corridor: CountCorridor++; break;
			case ETileType::Door: CountDoor++; break;
			default: break;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("PCGDungeonDataReader: Grid Stats - None=%d, Floor=%d, Wall=%d, Corridor=%d, Door=%d"),
		CountNone, CountFloor, CountWall, CountCorridor, CountDoor);
	
	UE_LOG(LogTemp, Log, TEXT("PCGDungeonDataReader: ActorLocation=%s, TileSize=%.1f, GridSize=%dx%d"), 
		*ActorLocation.ToString(), TileSize, Grid.Width, Grid.Height);
	
	for (int32 Y = 0; Y < Grid.Height; Y++)
	{
		for (int32 X = 0; X < Grid.Width; X++)
		{
			const FDungeonTile& Tile = Grid.GetTile(X, Y);
			bool bMatch = false;
			
			// DEBUG: Log Wall tiles near Grid(26, 17) to find missing EdgeWalls
			if (Tile.Type == ETileType::Wall && X >= 24 && X <= 28 && Y >= 15 && Y <= 19)
			{
				auto CheckWalkable = [&Grid](int32 tx, int32 ty) -> bool {
					if (!Grid.IsValid(tx, ty)) return false;
					ETileType t = Grid.GetTile(tx, ty).Type;
					return t == ETileType::Floor || t == ETileType::Corridor || t == ETileType::Door;
				};
				
				bool bN = CheckWalkable(X, Y+1);
				bool bS = CheckWalkable(X, Y-1);
				bool bE = CheckWalkable(X+1, Y);
				bool bW = CheckWalkable(X-1, Y);
				int32 WalkableCount = (bN?1:0) + (bS?1:0) + (bE?1:0) + (bW?1:0);
				
				UE_LOG(LogTemp, Warning, TEXT("DEBUG Wall(%d,%d): N=%d S=%d E=%d W=%d WalkableCount=%d Filter=%d"),
					X, Y, bN, bS, bE, bW, WalkableCount, (int32)Settings->TargetTileType);
			}

			switch (Settings->TargetTileType)
			{
			case EPCGDungeonTileFilter::All: bMatch = (Tile.Type != ETileType::None); break;
			case EPCGDungeonTileFilter::Floor: bMatch = (Tile.Type == ETileType::Floor); break;
			case EPCGDungeonTileFilter::Wall: bMatch = (Tile.Type == ETileType::Wall); break;
			case EPCGDungeonTileFilter::EdgeWall:
				// Wall tiles with single-direction OR 3-direction walkable adjacency
				// 1-direction: standard edge wall
				// 3-direction: fills gap not covered by ThroughWall (e.g., N+E+W -> EdgeWall covers N)
				if (Tile.Type == ETileType::Wall)
				{
					auto CheckWalkable = [&Grid](int32 tx, int32 ty) -> bool {
						if (!Grid.IsValid(tx, ty)) return false;
						ETileType t = Grid.GetTile(tx, ty).Type;
						return t == ETileType::Floor || t == ETileType::Corridor || t == ETileType::Door;
					};
					
					bool bWalkableWest = CheckWalkable(X-1, Y);
					bool bWalkableEast = CheckWalkable(X+1, Y);
					bool bWalkableSouth = CheckWalkable(X, Y-1);
					bool bWalkableNorth = CheckWalkable(X, Y+1);
					
					int32 WalkableCount = (bWalkableWest ? 1 : 0) + (bWalkableEast ? 1 : 0) + 
					                       (bWalkableSouth ? 1 : 0) + (bWalkableNorth ? 1 : 0);
					
					// ThroughWall case: 2 walkable on opposite sides (N+S or E+W)
					bool bIsThroughWall = (WalkableCount == 2) && 
					                       ((bWalkableNorth && bWalkableSouth) || (bWalkableEast && bWalkableWest));
					
					// EdgeWall matches:
					// 1. Single direction (standard edge)
					// 2. Two opposite directions (ThroughWall → will spawn 2 EdgeWalls)
					// 3. Three directions (ThroughWall + EdgeWall combo)
					if (WalkableCount == 1 || bIsThroughWall || WalkableCount == 3)
					{
						bMatch = true;
					}
				}
				break;
			case EPCGDungeonTileFilter::CornerWall:
				// Wall tiles with L-shaped walkable adjacency (corners only, not straight-through)
				// Also exclude corners adjacent to ThroughWall tiles to prevent overlap
				if (Tile.Type == ETileType::Wall)
				{
					auto CheckWalkable = [&Grid](int32 tx, int32 ty) -> bool {
						if (!Grid.IsValid(tx, ty)) return false;
						ETileType t = Grid.GetTile(tx, ty).Type;
						return t == ETileType::Floor || t == ETileType::Corridor || t == ETileType::Door;
					};
					
					// Check if an adjacent Wall tile is a ThroughWall (has N+S or E+W walkable)
					auto IsThroughWallTile = [&Grid, &CheckWalkable](int32 tx, int32 ty) -> bool {
						if (!Grid.IsValid(tx, ty)) return false;
						if (Grid.GetTile(tx, ty).Type != ETileType::Wall) return false;
						
						bool bN = Grid.IsValid(tx, ty+1) && (Grid.GetTile(tx, ty+1).Type == ETileType::Floor || 
						          Grid.GetTile(tx, ty+1).Type == ETileType::Corridor || Grid.GetTile(tx, ty+1).Type == ETileType::Door);
						bool bS = Grid.IsValid(tx, ty-1) && (Grid.GetTile(tx, ty-1).Type == ETileType::Floor || 
						          Grid.GetTile(tx, ty-1).Type == ETileType::Corridor || Grid.GetTile(tx, ty-1).Type == ETileType::Door);
						bool bE = Grid.IsValid(tx+1, ty) && (Grid.GetTile(tx+1, ty).Type == ETileType::Floor || 
						          Grid.GetTile(tx+1, ty).Type == ETileType::Corridor || Grid.GetTile(tx+1, ty).Type == ETileType::Door);
						bool bW = Grid.IsValid(tx-1, ty) && (Grid.GetTile(tx-1, ty).Type == ETileType::Floor || 
						          Grid.GetTile(tx-1, ty).Type == ETileType::Corridor || Grid.GetTile(tx-1, ty).Type == ETileType::Door);
						
						return (bN && bS) || (bE && bW);
					};
					
					bool bWalkableWest = CheckWalkable(X-1, Y);
					bool bWalkableEast = CheckWalkable(X+1, Y);
					bool bWalkableSouth = CheckWalkable(X, Y-1);
					bool bWalkableNorth = CheckWalkable(X, Y+1);
					
					// Corner = L-shaped (diagonal pairs only)
					// Exclude straight-through cases (N+S or E+W)
					bool bIsLCorner = (bWalkableNorth && bWalkableEast) ||
					                  (bWalkableNorth && bWalkableWest) ||
					                  (bWalkableSouth && bWalkableEast) ||
					                  (bWalkableSouth && bWalkableWest);
					
					// Exclude if THIS tile matches ThroughWall condition (N+S or E+W)
					// This prevents same tile from being both CornerWall and ThroughWall
					bool bThisTileIsThroughWall = (bWalkableNorth && bWalkableSouth) || (bWalkableEast && bWalkableWest);
					
					// Check if any adjacent Wall tile is a ThroughWall
					// NOTE: We now allow CornerWall adjacent to ThroughWall to fill the gaps
					// where two ThroughWalls meet at corners
					
					if (bIsLCorner && !bThisTileIsThroughWall)
					{
						bMatch = true;
					}
				}
				break;
			case EPCGDungeonTileFilter::ThroughWall:
				// DEPRECATED: ThroughWall is now handled by EdgeWall (spawns 2 EdgeWalls instead)
				// This filter no longer matches any tiles
				bMatch = false;
				break;
			case EPCGDungeonTileFilter::Corridor: bMatch = (Tile.Type == ETileType::Corridor); break;
			case EPCGDungeonTileFilter::Door: bMatch = (Tile.Type == ETileType::Door); break;
			case EPCGDungeonTileFilter::Walkable: 
				bMatch = (Tile.Type == ETileType::Floor || 
				          Tile.Type == ETileType::Corridor || 
				          Tile.Type == ETileType::Door || 
				          Tile.Type == ETileType::Stair); 
				break;
			}

			if (bMatch)
			{
				FTransform Transform;
				// Calculate point location to match Landscape texture coordinates exactly
				// Landscape World Size = Grid.Width * TileSize (from DungeonLandscapeTool)
				// Each grid tile maps to TileSize world units
				// Point should be at center of tile: (X + 0.5) * TileSize
				FVector PointLocation = ActorLocation + FVector((X + 0.5) * TileSize, (Y + 0.5) * TileSize, 0.0);
				
				// Get pivot offset from Theme (will be applied after rotation is calculated)
				FVector PivotOffset = FVector::ZeroVector;
				if (Theme)
				{
					if (Settings->TargetTileType == EPCGDungeonTileFilter::Wall || 
					    Settings->TargetTileType == EPCGDungeonTileFilter::EdgeWall)
					{
						PivotOffset = Theme->WallPivotOffset;
					}
					else if (Settings->TargetTileType == EPCGDungeonTileFilter::CornerWall)
					{
						PivotOffset = Theme->CornerWallPivotOffset;
					}
					else if (Settings->TargetTileType == EPCGDungeonTileFilter::ThroughWall)
					{
						PivotOffset = Theme->ThroughWallPivotOffset;
						// Debug: Log offset value
						if (Points.Num() < 3)
						{
							UE_LOG(LogTemp, Warning, TEXT("ThroughWall: PivotOffset=%s"), *PivotOffset.ToString());
						}
					}
					else if (Settings->TargetTileType == EPCGDungeonTileFilter::Floor ||
					         Settings->TargetTileType == EPCGDungeonTileFilter::Corridor)
					{
						PivotOffset = Theme->FloorPivotOffset;
					}
				}
				
				// Debug log for first few doors
				if (Settings->TargetTileType == EPCGDungeonTileFilter::Door && Points.Num() < 5)
				{
					UE_LOG(LogTemp, Warning, TEXT("PCGDungeonDataReader [Door]: GridPos=(%d,%d), TileSize=%.1f, ActorLoc=%s, PointLoc=%s"),
						X, Y, TileSize, *ActorLocation.ToString(), *PointLocation.ToString());
				}
				
				// Handle Door Rotation based on adjacent walkable tiles
				if (Settings->TargetTileType == EPCGDungeonTileFilter::Door)
				{
					// Check which direction the door connects (where are the walkable tiles?)
					bool bWalkableNorth = (Y + 1 < Grid.Height) && IsWalkableTile(Grid.GetTile(X, Y + 1).Type);
					bool bWalkableSouth = (Y - 1 >= 0) && IsWalkableTile(Grid.GetTile(X, Y - 1).Type);
					bool bWalkableEast = (X + 1 < Grid.Width) && IsWalkableTile(Grid.GetTile(X + 1, Y).Type);
					bool bWalkableWest = (X - 1 >= 0) && IsWalkableTile(Grid.GetTile(X - 1, Y).Type);
					
					// Door connects North-South (walkable tiles above/below) -> Door should face North (0 rotation)
					// Door connects East-West (walkable tiles left/right) -> Door should face East (90 rotation)
					if (bWalkableEast || bWalkableWest)
					{
						// Door spans East-West, passage goes through along X axis
						// Rotate 90 degrees so door frame aligns with Y axis
						Transform.SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(90.0f)));
					}
					// else: Door spans North-South (default 0 rotation)
					
					// Debug log rotation
					if (Points.Num() < 5)
					{
						UE_LOG(LogTemp, Log, TEXT("Door(%d,%d): N=%d S=%d E=%d W=%d -> Rot=%s"),
							X, Y, bWalkableNorth, bWalkableSouth, bWalkableEast, bWalkableWest,
							(bWalkableEast || bWalkableWest) ? TEXT("90") : TEXT("0"));
					}
				}
				
				// Handle EdgeWall Rotation - face toward walkable tile (1-way) or non-walkable tile (3-way)
				if (Settings->TargetTileType == EPCGDungeonTileFilter::EdgeWall)
				{
					// Check which direction has walkable tiles
					bool bWalkableNorth = (Y + 1 < Grid.Height) && IsWalkableTile(Grid.GetTile(X, Y + 1).Type);
					bool bWalkableSouth = (Y - 1 >= 0) && IsWalkableTile(Grid.GetTile(X, Y - 1).Type);
					bool bWalkableEast = (X + 1 < Grid.Width) && IsWalkableTile(Grid.GetTile(X + 1, Y).Type);
					bool bWalkableWest = (X - 1 >= 0) && IsWalkableTile(Grid.GetTile(X - 1, Y).Type);
					
					int32 WalkableCount = (bWalkableNorth ? 1 : 0) + (bWalkableSouth ? 1 : 0) + 
					                       (bWalkableEast ? 1 : 0) + (bWalkableWest ? 1 : 0);
					
					// Default: wall mesh faces +X direction
					float RotationDegrees = 0.0f;
					
					if (WalkableCount == 1)
					{
						// Standard case: face toward the single walkable tile
						if (bWalkableEast) RotationDegrees = 0.0f;       // +X direction
						else if (bWalkableWest) RotationDegrees = 180.0f;  // -X direction
						else if (bWalkableNorth) RotationDegrees = 90.0f;  // +Y direction
						else if (bWalkableSouth) RotationDegrees = 270.0f; // -Y direction
					}
					else if (WalkableCount == 2 && ((bWalkableNorth && bWalkableSouth) || (bWalkableEast && bWalkableWest)))
					{
						// ThroughWall case: spawn 2 EdgeWalls (0° and 180°)
						// Plus additional EdgeWalls for non-walkable perpendicular directions (end caps)
						if (bWalkableNorth && bWalkableSouth)
						{
							RotationDegrees = 90.0f;  // First wall faces North
							
							// Create second point facing South (180° opposite)
							FTransform SecondTransform;
							FQuat SecondRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(270.0f));
							SecondTransform.SetRotation(SecondRotation);
							FVector SecondLocation = PointLocation + SecondRotation.RotateVector(PivotOffset);
							SecondTransform.SetLocation(SecondLocation);
							
							FPCGPoint SecondPoint;
							SecondPoint.Transform = SecondTransform;
							Points.Add(SecondPoint);
						
						// Check if neighboring tiles are also ThroughWall (N+S walkable)
							auto IsNeighborThroughWallNS = [&Grid](int32 tx, int32 ty) -> bool {
								if (!Grid.IsValid(tx, ty)) return false;
								if (Grid.GetTile(tx, ty).Type != ETileType::Wall) return false;
								auto IsWalkable = [](ETileType t) { return t == ETileType::Floor || t == ETileType::Corridor || t == ETileType::Door; };
								bool nN = Grid.IsValid(tx, ty + 1) && IsWalkable(Grid.GetTile(tx, ty + 1).Type);
								bool nS = Grid.IsValid(tx, ty - 1) && IsWalkable(Grid.GetTile(tx, ty - 1).Type);
								return nN && nS;
							};
							
							// Helper to check if neighbor is NOT None (any solid tile - Wall, Floor, etc.)
							// End cap is NOT needed when neighbor is open space (None)
							// End cap IS needed when neighbor is any other type
							auto IsNotNoneTile = [&Grid](int32 tx, int32 ty) -> bool {
								if (!Grid.IsValid(tx, ty)) return false;
								return Grid.GetTile(tx, ty).Type != ETileType::None;
							};
							
							// Add end cap for East ONLY if:
							// - E is non-walkable (Wall or None)
							// - E neighbor is NOT ThroughWall
							// - E neighbor is NOT None (any solid type)
							if (!bWalkableEast && !IsNeighborThroughWallNS(X + 1, Y) && IsNotNoneTile(X + 1, Y))
							{
								FTransform EastTransform;
								FQuat EastRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(0.0f));
								EastTransform.SetRotation(EastRotation);
								FVector EastLocation = PointLocation + EastRotation.RotateVector(PivotOffset);
								EastTransform.SetLocation(EastLocation);
								
								FPCGPoint EastPoint;
								EastPoint.Transform = EastTransform;
								Points.Add(EastPoint);
							}
							
							// Add end cap for West ONLY if neighbor is not None
							if (!bWalkableWest && !IsNeighborThroughWallNS(X - 1, Y) && IsNotNoneTile(X - 1, Y))
							{
								FTransform WestTransform;
								FQuat WestRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(180.0f));
								WestTransform.SetRotation(WestRotation);
								FVector WestLocation = PointLocation + WestRotation.RotateVector(PivotOffset);
								WestTransform.SetLocation(WestLocation);
								
								FPCGPoint WestPoint;
								WestPoint.Transform = WestTransform;
								Points.Add(WestPoint);
							}
						}
						else // bWalkableEast && bWalkableWest
						{
							RotationDegrees = 0.0f;   // First wall faces East
							
							// Create second point facing West (180° opposite)
							FTransform SecondTransform;
							FQuat SecondRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(180.0f));
							SecondTransform.SetRotation(SecondRotation);
							FVector SecondLocation = PointLocation + SecondRotation.RotateVector(PivotOffset);
							SecondTransform.SetLocation(SecondLocation);
							
							FPCGPoint SecondPoint;
							SecondPoint.Transform = SecondTransform;
							Points.Add(SecondPoint);
							
							// Check if neighboring tiles are also ThroughWall (E+W walkable)
							auto IsNeighborThroughWallEW = [&Grid](int32 tx, int32 ty) -> bool {
								if (!Grid.IsValid(tx, ty)) return false;
								if (Grid.GetTile(tx, ty).Type != ETileType::Wall) return false;
								auto IsWalkable = [](ETileType t) { return t == ETileType::Floor || t == ETileType::Corridor || t == ETileType::Door; };
								bool nE = Grid.IsValid(tx + 1, ty) && IsWalkable(Grid.GetTile(tx + 1, ty).Type);
								bool nW = Grid.IsValid(tx - 1, ty) && IsWalkable(Grid.GetTile(tx - 1, ty).Type);
								return nE && nW;
							};
							
							// Helper to check if neighbor is a Wall tile
							// Helper to check if neighbor is a walkable tile (Floor/Corridor/Door)
							// End cap is needed when ThroughWall ends at a room/corridor, not at open space (None)
							auto IsWalkableTileAt = [&Grid](int32 tx, int32 ty) -> bool {
								if (!Grid.IsValid(tx, ty)) return false;
								ETileType t = Grid.GetTile(tx, ty).Type;
								return t == ETileType::Floor || t == ETileType::Corridor || t == ETileType::Door;
							};
							
							// Add end cap for North ONLY if neighbor is walkable (room/corridor)
							if (!bWalkableNorth && !IsNeighborThroughWallEW(X, Y + 1) && IsWalkableTileAt(X, Y + 1))
							{
								FTransform NorthTransform;
								FQuat NorthRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(90.0f));
								NorthTransform.SetRotation(NorthRotation);
								FVector NorthLocation = PointLocation + NorthRotation.RotateVector(PivotOffset);
								NorthTransform.SetLocation(NorthLocation);
								
								FPCGPoint NorthPoint;
								NorthPoint.Transform = NorthTransform;
								Points.Add(NorthPoint);
							}
							
							// Add end cap for South ONLY if neighbor is walkable (room/corridor)
							if (!bWalkableSouth && !IsNeighborThroughWallEW(X, Y - 1) && IsWalkableTileAt(X, Y - 1))
							{
								FTransform SouthTransform;
								FQuat SouthRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(270.0f));
								SouthTransform.SetRotation(SouthRotation);
								FVector SouthLocation = PointLocation + SouthRotation.RotateVector(PivotOffset);
								SouthTransform.SetLocation(SouthLocation);
								
								FPCGPoint SouthPoint;
								SouthPoint.Transform = SouthTransform;
								Points.Add(SouthPoint);
							}
						}
					}
					else if (WalkableCount == 3)
					{
						// 3-way case: generate EdgeWall for EACH walkable direction (3 walls total)
						// Wall faces are placed toward walkable areas
						
						// Helper to add a wall at specific rotation
						auto AddWallAtRotation = [&](float Degrees) {
							FTransform ExtraTransform;
							FQuat ExtraRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees));
							ExtraTransform.SetRotation(ExtraRotation);
							FVector ExtraLocation = PointLocation + ExtraRotation.RotateVector(PivotOffset);
							ExtraTransform.SetLocation(ExtraLocation);
							
							FPCGPoint ExtraPoint;
							ExtraPoint.Transform = ExtraTransform;
							Points.Add(ExtraPoint);
						};
						
						// First wall direction (will use RotationDegrees for the main point)
						bool bFirstWallSet = false;
						
						if (bWalkableNorth)
						{
							if (!bFirstWallSet) { RotationDegrees = 90.0f; bFirstWallSet = true; }
							else AddWallAtRotation(90.0f);
						}
						if (bWalkableSouth)
						{
							if (!bFirstWallSet) { RotationDegrees = 270.0f; bFirstWallSet = true; }
							else AddWallAtRotation(270.0f);
						}
						if (bWalkableEast)
						{
							if (!bFirstWallSet) { RotationDegrees = 0.0f; bFirstWallSet = true; }
							else AddWallAtRotation(0.0f);
						}
						if (bWalkableWest)
						{
							if (!bFirstWallSet) { RotationDegrees = 180.0f; bFirstWallSet = true; }
							else AddWallAtRotation(180.0f);
						}
						
						UE_LOG(LogTemp, Warning, TEXT("WC3 at (%d,%d): N=%d S=%d E=%d W=%d - Added 3 walls"), 
							X, Y, bWalkableNorth, bWalkableSouth, bWalkableEast, bWalkableWest);
					}
					
					FQuat Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(RotationDegrees));
					Transform.SetRotation(Rotation);
					// Apply pivot offset in local space (rotated)
					PointLocation += Rotation.RotateVector(PivotOffset);
				}
				// Handle CornerWall Rotation - based on corner type (two walkable directions)
				else if (Settings->TargetTileType == EPCGDungeonTileFilter::CornerWall)
				{
					bool bWalkableNorth = (Y + 1 < Grid.Height) && IsWalkableTile(Grid.GetTile(X, Y + 1).Type);
					bool bWalkableSouth = (Y - 1 >= 0) && IsWalkableTile(Grid.GetTile(X, Y - 1).Type);
					bool bWalkableEast = (X + 1 < Grid.Width) && IsWalkableTile(Grid.GetTile(X + 1, Y).Type);
					bool bWalkableWest = (X - 1 >= 0) && IsWalkableTile(Grid.GetTile(X - 1, Y).Type);
					
				// L-mesh default: faces +X/+Y (NE direction)
					// Corner rotation based on which two directions are walkable
					float RotationDegrees = 0.0f;
					
					if (bWalkableSouth && bWalkableWest)
					{
						// SW corner -> 180 degrees
						RotationDegrees = 180.0f;
					}
					else if (bWalkableNorth && bWalkableWest)
					{
						// NW corner -> 90 degrees
						RotationDegrees = 90.0f;
					}
					else if (bWalkableNorth && bWalkableEast)
					{
						// NE corner -> 0 degrees
						RotationDegrees = 0.0f;
					}
					else if (bWalkableSouth && bWalkableEast)
					{
						// SE corner -> 270 degrees
						RotationDegrees = 270.0f;
					}
					
					FQuat Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(RotationDegrees));
					Transform.SetRotation(Rotation);
					PointLocation += Rotation.RotateVector(PivotOffset);
				}
				// Handle ThroughWall Rotation - based on N+S or E+W direction
				else if (Settings->TargetTileType == EPCGDungeonTileFilter::ThroughWall)
				{
					bool bWalkableNorth = (Y + 1 < Grid.Height) && IsWalkableTile(Grid.GetTile(X, Y + 1).Type);
					bool bWalkableSouth = (Y - 1 >= 0) && IsWalkableTile(Grid.GetTile(X, Y - 1).Type);
					bool bWalkableEast = (X + 1 < Grid.Width) && IsWalkableTile(Grid.GetTile(X + 1, Y).Type);
					bool bWalkableWest = (X - 1 >= 0) && IsWalkableTile(Grid.GetTile(X - 1, Y).Type);
					
					// ThroughWall rotation: 0 for E+W, 90 for N+S
					float RotationDegrees = 0.0f;
					if (bWalkableNorth && bWalkableSouth)
					{
						RotationDegrees = 90.0f;  // Wall runs E-W, faces N/S
					}
					else if (bWalkableEast && bWalkableWest)
					{
						RotationDegrees = 0.0f;   // Wall runs N-S, faces E/W
					}
					
					FQuat Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(RotationDegrees));
					Transform.SetRotation(Rotation);
					PointLocation += Rotation.RotateVector(PivotOffset);
				}
				else
				{
					// No rotation - apply pivot offset directly
					PointLocation += PivotOffset;
				}

				Transform.SetLocation(PointLocation);
				FPCGPoint& Point = Points.Emplace_GetRef(Transform, 1.0f, 0);
				Point.BoundsMin = FVector(-TileSize * 0.5f);
				Point.BoundsMax = FVector(TileSize * 0.5f);
				Point.SetExtents(FVector(TileSize * 0.5f));
			}
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
