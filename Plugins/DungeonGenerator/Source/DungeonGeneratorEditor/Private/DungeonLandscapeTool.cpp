#include "DungeonLandscapeTool.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "LandscapeInfo.h"
#include "LandscapeComponent.h"
#include "LandscapeEditorUtils.h"
#include "LandscapeEdit.h"
#include "LandscapeLayerInfoObject.h"
#include "DungeonGrid.h"
#include "Engine/World.h"
#include "Math/MathFwd.h"
#include "Data/DungeonThemeAsset.h"
#include "Components/DungeonRendererComponent.h"
#include "DungeonWorldBuilder.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"

ALandscape* UDungeonLandscapeTool::GenerateLandscape(ADungeonWorldBuilder* DungeonActor, bool bUpdateExisting)
{
	// Forward to internal implementation with null grid (will fallback to cache/config)
	return GenerateLandscapeWithGrid(DungeonActor, bUpdateExisting, nullptr);
}

ALandscape* UDungeonLandscapeTool::GenerateLandscapeWithGrid(ADungeonWorldBuilder* DungeonActor, bool bUpdateExisting, const FDungeonGrid* InGrid)
{
	UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Request received."));

	if (!DungeonActor) {
		UE_LOG(LogTemp, Error, TEXT("DungeonLandscapeTool: DungeonActor is null!"));
		return nullptr;
	}

	UWorld* World = DungeonActor->GetWorld();
	if (!World) {
		UE_LOG(LogTemp, Error, TEXT("DungeonLandscapeTool: World is null!"));
		return nullptr;
	}

	if (!DungeonActor->DungeonRenderer) {
		UE_LOG(LogTemp, Error, TEXT("DungeonLandscapeTool: Renderer Component is missing!"));
		return nullptr;
	}

	// 0. Resolve Grid Data
	const FDungeonGrid* GridToUse = InGrid;
	
	// If not provided, try cache
	if (!GridToUse && DungeonActor->DungeonRenderer) {
		GridToUse = DungeonActor->DungeonRenderer->GetCachedGrid();
	}
	
	// If still invalid, we can proceed with just dimensions (Flat Landscape) OR Fail.
	// Earlier we used GeneratorConfig for dimensions, which is safe for dimensions.
	// But GenerateHeightmap relies on Grid content for flattening.
	
	int32 GridW = DungeonActor->GeneratorConfig.Width;
	int32 GridH = DungeonActor->GeneratorConfig.Height;
	
	if (GridToUse && GridToUse->Width > 0) {
		GridW = GridToUse->Width;
		GridH = GridToUse->Height;
	}
	
	if (GridW <= 0 || GridH <= 0) {
		UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Invalid Grid Dimensions."));
		return nullptr;
	}

	// 1. Calculate Landscape Resolution
	int32 Resolution = 0;
	int32 ComponentSize = 63;
	int32 SectionsPerComponent = 1;
	
	// Build raised terrain settings from theme
	FRaisedTerrainSettings RaisedSettings;
	FTerracedTerrainSettings TerracedSettings;
	if (DungeonActor->DungeonTheme)
	{
		RaisedSettings.bEnabled = DungeonActor->DungeonTheme->bEnableRaisedTerrain;
		RaisedSettings.Height = DungeonActor->DungeonTheme->WallTerrainHeight;
		RaisedSettings.HeightNoise = DungeonActor->DungeonTheme->WallHeightNoise;
		RaisedSettings.EdgeSteepness = DungeonActor->DungeonTheme->WallEdgeSteepness;
		
		// Path noise settings
		RaisedSettings.PathDepressionDepth = DungeonActor->DungeonTheme->PathDepressionDepth;
		RaisedSettings.PathNoiseAmplitude1 = DungeonActor->DungeonTheme->PathNoiseAmplitude1;
		RaisedSettings.PathNoiseAmplitude2 = DungeonActor->DungeonTheme->PathNoiseAmplitude2;
		RaisedSettings.PathNoiseAmplitude3 = DungeonActor->DungeonTheme->PathNoiseAmplitude3;
		RaisedSettings.PathDomainWarp = DungeonActor->DungeonTheme->PathDomainWarp;
		
		// Terraced terrain settings
		TerracedSettings.bEnabled = DungeonActor->DungeonTheme->bEnableTerracedTerrain;
		TerracedSettings.MaxHeightVariation = DungeonActor->DungeonTheme->MaxRoomHeightVariation;
		TerracedSettings.Seed = DungeonActor->DungeonTheme->TerrainSeed;
	}
	
	// PASS GRID POINTER, RAISED TERRAIN SETTINGS, AND TERRACED SETTINGS
	TArray<uint16> HeightData = GenerateHeightmap(GridW, GridH, Resolution, ComponentSize, SectionsPerComponent, GridToUse, &RaisedSettings, &TerracedSettings);

	if (HeightData.Num() == 0) {
		UE_LOG(LogTemp, Error, TEXT("DungeonLandscapeTool: Failed to generate heightmap data."));
		return nullptr;
	}

	// 2. Calculate Landscape position (Force World Origin)
	// User Request: "Default generation coordinates to 0,0,0"
	// FVector Location = DungeonActor->GetActorLocation(); 
	FVector Location = FVector::ZeroVector;
	
	// 3. Calculate component count
	int32 ComponentsX = (Resolution - 1) / ComponentSize;
	int32 ComponentsY = (Resolution - 1) / ComponentSize;
	
	// Spawn with proper template
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName("DungeonLandscape");
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALandscape* Landscape = World->SpawnActor<ALandscape>(Location, FRotator::ZeroRotator, SpawnParams);
	
	if (Landscape)
	{
		Landscape->SetActorLabel(TEXT("DungeonLandscape"));
		// CRITICAL: PCG Graph looks for this tag to find the landscape!
		Landscape->Tags.Add(FName("DungeonGeneratedLandscape"));

		UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Spawning Landscape Actor..."));

		if (!Landscape->GetLandscapeGuid().IsValid()) {
			Landscape->SetLandscapeGuid(FGuid::NewGuid());
		}

		Landscape->ComponentSizeQuads = ComponentSize;
		Landscape->SubsectionSizeQuads = ComponentSize;


		// 4. Assign Material from Theme (Restored per user request)
		if (DungeonActor->DungeonTheme && DungeonActor->DungeonTheme->LandscapeMaterial)
		{
			Landscape->LandscapeMaterial = DungeonActor->DungeonTheme->LandscapeMaterial;
			UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Assigned Material '%s' from Theme."), *DungeonActor->DungeonTheme->LandscapeMaterial->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: No Landscape Material found in Theme (or Theme is null). Landscape will be gray."));
		}

#if WITH_EDITOR
		// Calculate Scale
		// DungeonWorldBuilder typically gets TileSize from Theme
		float TileSize = 100.0f; 
		if (DungeonActor->DungeonTheme) {
			TileSize = DungeonActor->DungeonTheme->TileSize;
		} else {
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: No Theme assigned. Using default TileSize 100.0f"));
		}

		// Calculate scale so that Landscape exactly matches Dungeon world size
		// Heightmap valid data spans Grid * 4 quads (Resolution may be larger due to ComponentSize padding)
		// Target world size = Grid * TileSize
		// We want Grid*4 heightmap pixels to map to Grid*TileSize world units
		// Scale = TileSize / 4 (each quad = TileSize/4 world units)
		float LandscapeScaleXY = TileSize / 4.0f;  // 100 / 4 = 25
		
		FVector LandscapeScale(LandscapeScaleXY, LandscapeScaleXY, 100.0f);
		Landscape->SetActorScale3D(LandscapeScale);

#if WITH_EDITOR
		// Force Disable Edit Layers (UE5 feature that blocks simple painting)
		// Try using the Toggle function found in docs/community
#pragma warning(push)
#pragma warning(disable: 4996) // Suppress depreciation warning for CanHaveLayersContent
		if (Landscape->CanHaveLayersContent())
		{
			Landscape->ToggleCanHaveLayersContent();
			UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Disabled Edit Layers via ToggleCanHaveLayersContent()."));
		}
#pragma warning(pop)
#endif
		

		
		int32 MinX = 0;
		int32 MinY = 0;
		int32 MaxX = Resolution - 1;
		int32 MaxY = Resolution - 1;
		
		// Create Layer Blend Settings from Theme
		FLayerBlendSettings BlendSettings;
		if (DungeonActor->DungeonTheme && DungeonActor->DungeonTheme->bEnableRaisedTerrain)
		{
			BlendSettings.DirtStartDistance = DungeonActor->DungeonTheme->DirtStartDistance;
			BlendSettings.StoneStartDistance = DungeonActor->DungeonTheme->StoneStartDistance;
			BlendSettings.BlendRadius = DungeonActor->DungeonTheme->LayerBlendRadius;
			BlendSettings.EdgeBlendWidth = DungeonActor->DungeonTheme->EdgeBlendWidth;
		}
		
		// Generate Weightmap data alongside HeightData
		TArray<uint8> WeightmapData = GenerateWeightmap(Resolution, GridToUse, &BlendSettings);
		
		// Prepare Layer Infos for Import
		TMap<FGuid, TArray<FLandscapeImportLayerInfo>> ImportLayerInfosMap;
		TArray<FLandscapeImportLayerInfo> MaterialLayers;
		
		// Path Layer (Stone/layer3) - uses generated pattern
		// We need to create LayerInfo objects if they don't exist
		// For simplicity, we'll load existing ones from theme or create temporary
		ULandscapeLayerInfoObject* PathLayerInfo = nullptr;
		ULandscapeLayerInfoObject* BaseLayerInfo = nullptr;
		
		// Try to get layer info from Theme
		if (DungeonActor->DungeonTheme)
		{
			PathLayerInfo = DungeonActor->DungeonTheme->PathLayerInfo;
			BaseLayerInfo = DungeonActor->DungeonTheme->BaseLayerInfo;
		}
		
		// Fallback: Create temporary LayerInfo if not provided
		if (!PathLayerInfo)
		{
			PathLayerInfo = NewObject<ULandscapeLayerInfoObject>(GetTransientPackage(), FName(TEXT("PathLayerInfo_Temp")));
			PathLayerInfo->SetLayerName(FName(TEXT("layer3")), false);
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Created temporary PathLayerInfo. Assign in Theme for persistent use."));
		}
		if (!BaseLayerInfo)
		{
			BaseLayerInfo = NewObject<ULandscapeLayerInfoObject>(GetTransientPackage(), FName(TEXT("BaseLayerInfo_Temp")));
			BaseLayerInfo->SetLayerName(FName(TEXT("layer1")), false);
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Created temporary BaseLayerInfo. Assign in Theme for persistent use."));
		}
		
		// 1. Path Layer (Primary)
		FLandscapeImportLayerInfo PathImportInfo;
		PathImportInfo.LayerName = PathLayerInfo->GetLayerName();
		PathImportInfo.LayerInfo = PathLayerInfo;
		PathImportInfo.LayerData = WeightmapData;
		MaterialLayers.Add(PathImportInfo);
		
		// 2. Wall Layer (Dirt on slope/transition - only if raised terrain enabled)
		TArray<uint8> WallWeightmapData;
		bool bHasWallLayer = false;
		ULandscapeLayerInfoObject* WallLayerInfo = nullptr;
		
		// Debug log for WallLayerInfo conditions
		UE_LOG(LogTemp, Warning, TEXT("[GenerateLandscape WallLayer] Theme=%s, bEnableRaisedTerrain=%s"),
			DungeonActor->DungeonTheme ? TEXT("Valid") : TEXT("NULL"),
			(DungeonActor->DungeonTheme && DungeonActor->DungeonTheme->bEnableRaisedTerrain) ? TEXT("True") : TEXT("False"));

		if (DungeonActor->DungeonTheme && DungeonActor->DungeonTheme->bEnableRaisedTerrain)
		{
			WallLayerInfo = DungeonActor->DungeonTheme->WallLayerInfo;
			if (WallLayerInfo)
			{
				WallWeightmapData = GenerateWallWeightmap(Resolution, GridToUse, &BlendSettings);
				bHasWallLayer = true;
				
				// Debug: Check if WallWeightmap has any non-zero values
				int32 NonZeroCount = 0;
				int32 MaxValue = 0;
				for (int32 i = 0; i < WallWeightmapData.Num(); i++)
				{
					if (WallWeightmapData[i] > 0) NonZeroCount++;
					if (WallWeightmapData[i] > MaxValue) MaxValue = WallWeightmapData[i];
				}
				UE_LOG(LogTemp, Warning, TEXT("[WallWeightmap Debug] Total=%d, NonZero=%d, MaxValue=%d"), 
					WallWeightmapData.Num(), NonZeroCount, MaxValue);
				
				FLandscapeImportLayerInfo WallImportInfo;
				WallImportInfo.LayerName = WallLayerInfo->GetLayerName();
				WallImportInfo.LayerInfo = WallLayerInfo;
				WallImportInfo.LayerData = WallWeightmapData;
				MaterialLayers.Add(WallImportInfo);
				
				UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Added Wall Layer '%s' for slope/transition areas"), *WallLayerInfo->GetLayerName().ToString());
			}
		}
		
		// 3. Base Layer (Inverse of Path + Wall)
		TArray<uint8> BaseWeightmapData;
		BaseWeightmapData.SetNum(WeightmapData.Num());
		for (int32 i = 0; i < WeightmapData.Num(); i++)
		{
			int32 Used = WeightmapData[i];
			if (bHasWallLayer)
			{
				Used += WallWeightmapData[i];
			}
			BaseWeightmapData[i] = static_cast<uint8>(FMath::Max(0, 255 - Used));
		}
		
		FLandscapeImportLayerInfo BaseImportInfo;
		BaseImportInfo.LayerName = BaseLayerInfo->GetLayerName();
		BaseImportInfo.LayerInfo = BaseLayerInfo;
		BaseImportInfo.LayerData = BaseWeightmapData;
		MaterialLayers.Add(BaseImportInfo);
		
		ImportLayerInfosMap.Add(FGuid(), MaterialLayers);

		TMap<FGuid, TArray<uint16>> HeightDataPerLayer;
		HeightDataPerLayer.Add(FGuid(), HeightData);
		
		TArray<FLandscapeLayer> ImportLayers; 

		UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Calling Import with Height + Weightmap data..."));
		Landscape->Import(
			Landscape->GetLandscapeGuid(),
			MinX, MinY, MaxX, MaxY,
			SectionsPerComponent, ComponentSize,
			HeightDataPerLayer,
			nullptr, // No filename
			ImportLayerInfosMap,
			ELandscapeImportAlphamapType::Additive,
			ImportLayers
		);
		

		// Critical for PCG Surface Sampler
		Landscape->SetActorEnableCollision(true);
		for (ULandscapeComponent* Comp : Landscape->LandscapeComponents)
		{
			if (Comp)
			{
				Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Comp->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			}
		}
		
#endif
		
		// 5. PCG is handled by DungeonPCGRenderer, not here.
		// Just ensuring Landscape exists is enough for the PCG graph to sample it.
		
		// Store reference in DungeonActor for cleanup
		// First destroy old Landscape if exists
		if (DungeonActor->SpawnedLandscape.IsValid())
		{
			DungeonActor->SpawnedLandscape->Destroy();
			UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Previous Landscape destroyed."));
		}
		DungeonActor->SpawnedLandscape = Landscape;
		UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: SpawnedLandscape stored."));
		
		// Cache Landscape world size for PCG coordinate calculation
		UDungeonRendererComponent* RendererComp = DungeonActor->FindComponentByClass<UDungeonRendererComponent>();
		if (RendererComp)
		{
			// Use Grid * TileSize (excludes Resolution padding)
			// The actual dungeon data only covers Grid*4 pixels, not full Resolution
			double LandscapeWorldSizeX = GridW * TileSize;
			double LandscapeWorldSizeY = GridH * TileSize;
			RendererComp->SetCachedLandscapeWorldSize(FVector2D(LandscapeWorldSizeX, LandscapeWorldSizeY));
			UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Cached LandscapeWorldSize=%.1fx%.1f (Grid*TileSize)"), LandscapeWorldSizeX, LandscapeWorldSizeY);
		}
		
		// CRITICAL: Save level so that GetLandscapeData works in PCG
		// Without saving, the Landscape data is not serialized and PCG can't read it
		if (FEditorFileUtils::SaveCurrentLevel())
		{
			UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Level saved for Landscape data serialization."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Failed to save level. PCG GetLandscapeData may not work."));
		}
		
		return Landscape;
	}

	return nullptr;
}

TArray<uint16> UDungeonLandscapeTool::GenerateHeightmap(int32 Width, int32 Height, int32& OutResolution, int32& OutComponentSize, int32& OutSectionsPerComponent, const FDungeonGrid* Grid, const FRaisedTerrainSettings* RaisedSettings, const FTerracedTerrainSettings* TerracedSettings)
{
	OutComponentSize = 63;
	OutSectionsPerComponent = 1;
	
	int32 W = Width * 4;
	int32 H = Height * 4;
	
	int32 CompX = FMath::CeilToInt((float)W / OutComponentSize);
	int32 CompY = FMath::CeilToInt((float)H / OutComponentSize);
	
	OutResolution = FMath::Max(CompX, CompY) * OutComponentSize + 1;
	
	TArray<uint16> Data;
	Data.SetNum(OutResolution * OutResolution);
	
	const uint16 BaseHeight = 32768; 

	if (!Grid || Grid->Width == 0 || Grid->Height == 0) {
		for (int32 i = 0; i < Data.Num(); i++) Data[i] = BaseHeight;
		return Data; 
	}

	// Terraced terrain: Analyze room clusters and assign heights
	FRoomClusterData ClusterData;
	const bool bTerracedEnabled = TerracedSettings && TerracedSettings->bEnabled;
	if (bTerracedEnabled)
	{
		ClusterData = AnalyzeRoomClusters(Grid, TerracedSettings->Seed, TerracedSettings->MaxHeightVariation);
		UE_LOG(LogTemp, Log, TEXT("[Terraced] Found %d room clusters"), ClusterData.ClusterHeights.Num());
	}

	// Noise parameters for terrain variation
	const float NoiseScale = 0.03f;  // Lower = larger hills
	const float NoiseAmplitude = 300.0f; // Height variation
	const float WalkableDepressionDepth = RaisedSettings ? RaisedSettings->PathDepressionDepth : 150.0f;
	const int32 BlendRadius = 8; // Pixels to blend at edge (in heightmap space)
	
	// Path noise settings from theme
	const float PathNoiseAmp1 = RaisedSettings ? RaisedSettings->PathNoiseAmplitude1 : 25.0f;
	const float PathNoiseAmp2 = RaisedSettings ? RaisedSettings->PathNoiseAmplitude2 : 10.0f;
	const float PathNoiseAmp3 = RaisedSettings ? RaisedSettings->PathNoiseAmplitude3 : 5.0f;
	const float PathDomainWarp = RaisedSettings ? RaisedSettings->PathDomainWarp : 30.0f;

	// First pass: Generate base noise terrain for everything
	for (int32 Y = 0; Y < OutResolution; Y++)
	{
		for (int32 X = 0; X < OutResolution; X++)
		{
			float NoiseValue = FMath::PerlinNoise2D(FVector2D(X * NoiseScale, Y * NoiseScale));
			NoiseValue = (NoiseValue + 1.0f) * 0.5f; // Normalize to [0, 1]
			int32 HeightOffset = FMath::RoundToInt(NoiseValue * NoiseAmplitude);
			Data[Y * OutResolution + X] = static_cast<uint16>(BaseHeight + HeightOffset);
		}
	}

	// Second pass: Calculate distance to walkable and blend
	TArray<float> DistanceField;
	DistanceField.SetNum(OutResolution * OutResolution);
	
	// Initialize: 0 for walkable, large number for non-walkable
	for (int32 Y = 0; Y < OutResolution; Y++)
	{
		for (int32 X = 0; X < OutResolution; X++)
		{
			int32 DungeonX = X / 4;
			int32 DungeonY = Y / 4;
			bool bIsWalkable = false;
			if (Grid->IsValid(DungeonX, DungeonY))
			{
				ETileType Type = Grid->GetTile(DungeonX, DungeonY).Type;
				if (Type == ETileType::Floor || Type == ETileType::Corridor || 
				    Type == ETileType::Door || Type == ETileType::Stair)
				{
					bIsWalkable = true;
				}
			}
			DistanceField[Y * OutResolution + X] = bIsWalkable ? 0.0f : (float)BlendRadius + 1.0f;
		}
	}

	// Simple distance propagation (approximation)
	for (int32 Pass = 0; Pass < BlendRadius; Pass++)
	{
		for (int32 Y = 1; Y < OutResolution - 1; Y++)
		{
			for (int32 X = 1; X < OutResolution - 1; X++)
			{
				float MinDist = DistanceField[Y * OutResolution + X];
				MinDist = FMath::Min(MinDist, DistanceField[(Y-1) * OutResolution + X] + 1.0f);
				MinDist = FMath::Min(MinDist, DistanceField[(Y+1) * OutResolution + X] + 1.0f);
				MinDist = FMath::Min(MinDist, DistanceField[Y * OutResolution + (X-1)] + 1.0f);
				MinDist = FMath::Min(MinDist, DistanceField[Y * OutResolution + (X+1)] + 1.0f);
				DistanceField[Y * OutResolution + X] = MinDist;
			}
		}
	}

	// Third pass: Apply depression AND raised terrain based on distance
	// Get raised terrain settings (use defaults if not provided)
	const bool bRaisedEnabled = RaisedSettings ? RaisedSettings->bEnabled : false;
	const float RaisedHeight = RaisedSettings ? RaisedSettings->Height : 100.0f;
	const float RaisedNoise = RaisedSettings ? RaisedSettings->HeightNoise : 50.0f;
	const float RaisedNoiseScale = 0.08f; // Perlin noise frequency for raised areas
	
	for (int32 Y = 0; Y < OutResolution; Y++)
	{
		for (int32 X = 0; X < OutResolution; X++)
		{
			float Dist = DistanceField[Y * OutResolution + X];
			float BlendFactor = FMath::Clamp(Dist / (float)BlendRadius, 0.0f, 1.0f);
			// BlendFactor: 0 = center of walkable, 1 = fully non-walkable (wall)
			
			int32 CurrentHeight = Data[Y * OutResolution + X];
			
			// Terraced terrain: Apply room-specific height offset
			if (bTerracedEnabled && BlendFactor < 1.0f)
			{
				int32 TileX = X / 4;
				int32 TileY = Y / 4;
				float TerraceHeight = GetInterpolatedTerraceHeight(TileX, TileY, Grid, ClusterData);
				
				// Add FBM noise with domain warping for natural variation
				// Domain warping: distort coordinates for organic look
				float WarpX = X + FMath::PerlinNoise2D(FVector2D(X * 0.005f, Y * 0.005f)) * 50.0f;
				float WarpY = Y + FMath::PerlinNoise2D(FVector2D(Y * 0.005f, X * 0.005f)) * 50.0f;
				
				// FBM (Fractal Brownian Motion) - multiple octaves of noise
				float DetailNoise = 0.0f;
				DetailNoise += FMath::PerlinNoise2D(FVector2D(WarpX * 0.02f, WarpY * 0.02f)) * 50.0f;  // Large undulation
				DetailNoise += FMath::PerlinNoise2D(FVector2D(WarpX * 0.05f, WarpY * 0.05f)) * 20.0f;  // Medium detail
				DetailNoise += FMath::PerlinNoise2D(FVector2D(WarpX * 0.1f, WarpY * 0.1f)) * 8.0f;     // Fine detail
				
				// Reduce noise in walkable center, increase towards edges
				float NoiseStrength = FMath::Lerp(0.3f, 1.0f, BlendFactor);
				TerraceHeight += DetailNoise * NoiseStrength;
				
				CurrentHeight += FMath::RoundToInt(TerraceHeight);
			}
			
			if (BlendFactor < 1.0f)
			{
				// Near walkable: Lower walkable areas smoothly
				float Depression = WalkableDepressionDepth * (1.0f - BlendFactor);
				
				// Add subtle ground noise for natural variation in path areas
				// Use theme parameters for noise amplitudes
				float PathWarpX = X + FMath::PerlinNoise2D(FVector2D(X * 0.008f, Y * 0.008f)) * PathDomainWarp;
				float PathWarpY = Y + FMath::PerlinNoise2D(FVector2D(Y * 0.008f, X * 0.008f)) * PathDomainWarp;
				
				// FBM for ground detail (using theme amplitudes)
				float PathNoise = 0.0f;
				PathNoise += FMath::PerlinNoise2D(FVector2D(PathWarpX * 0.03f, PathWarpY * 0.03f)) * PathNoiseAmp1;  // Gentle rolling
				PathNoise += FMath::PerlinNoise2D(FVector2D(PathWarpX * 0.08f, PathWarpY * 0.08f)) * PathNoiseAmp2;  // Medium bumps
				PathNoise += FMath::PerlinNoise2D(FVector2D(PathWarpX * 0.15f, PathWarpY * 0.15f)) * PathNoiseAmp3;  // Fine gravel texture
				
				// Path center is smoother, edges can have more variation
				float PathNoiseStrength = FMath::Lerp(0.5f, 1.0f, BlendFactor);
				PathNoise *= PathNoiseStrength;
				
				CurrentHeight = FMath::Max(0, CurrentHeight - FMath::RoundToInt(Depression) + FMath::RoundToInt(PathNoise));
			}
			
			// Raised terrain for wall areas
			if (bRaisedEnabled && BlendFactor >= 0.9f)
			{
				// Wall area: add extra height with noise variation
				float WallNoise = FMath::PerlinNoise2D(FVector2D(X * RaisedNoiseScale, Y * RaisedNoiseScale));
				WallNoise = (WallNoise + 1.0f) * 0.5f; // Normalize to [0, 1]
				float ExtraHeight = RaisedHeight + (WallNoise - 0.5f) * RaisedNoise * 2.0f;
				CurrentHeight += FMath::RoundToInt(ExtraHeight);
			}
			else if (bRaisedEnabled && BlendFactor > 0.0f)
			{
				// Transition zone: steep cliff (based on edge steepness)
				// Higher steepness = sharper drop
				float CliffBlend = FMath::Pow(BlendFactor, 0.5f); // Sharp transition
				float PartialHeight = RaisedHeight * CliffBlend;
				CurrentHeight += FMath::RoundToInt(PartialHeight);
			}
			
			Data[Y * OutResolution + X] = static_cast<uint16>(FMath::Clamp(CurrentHeight, 0, 65535));
		}
	}
	
	return Data;
}

TArray<uint8> UDungeonLandscapeTool::GenerateWeightmap(int32 Resolution, const FDungeonGrid* Grid, const FLayerBlendSettings* BlendSettings)
{
	TArray<uint8> Data;
	Data.SetNum(Resolution * Resolution);
	
	// Default: Layer 1 (Grass) = 0
	FMemory::Memset(Data.GetData(), 0, Data.Num());
	
	if (!Grid || Grid->Width == 0 || Grid->Height == 0)
	{
		return Data;
	}
	
	// Get settings from BlendSettings or use defaults
	float StoneStartDistance = BlendSettings ? BlendSettings->StoneStartDistance : 6.0f;
	float BlendRadius = BlendSettings ? BlendSettings->BlendRadius : 12.0f;
	const int32 StoneBlendRadius = static_cast<int32>(BlendRadius);
	
	UE_LOG(LogTemp, Log, TEXT("[GenerateWeightmap] StoneStart=%.1f, BlendRadius=%.1f"), StoneStartDistance, BlendRadius);
	
	// Calculate distance from wall into walkable area
	TArray<float> DistFromWall;
	DistFromWall.SetNum(Resolution * Resolution);
	
	// Initialize: 0 for wall, large for walkable
	for (int32 Y = 0; Y < Resolution; Y++)
	{
		for (int32 X = 0; X < Resolution; X++)
		{
			int32 DungeonX = X / 4;
			int32 DungeonY = Y / 4;
			DungeonX = FMath::Clamp(DungeonX, 0, Grid->Width - 1);
			DungeonY = FMath::Clamp(DungeonY, 0, Grid->Height - 1);
			
			bool bIsWalkable = false;
			if (Grid->IsValid(DungeonX, DungeonY))
			{
				ETileType Type = Grid->GetTile(DungeonX, DungeonY).Type;
				if (Type == ETileType::Floor || Type == ETileType::Corridor || 
				    Type == ETileType::Door || Type == ETileType::Stair)
				{
					bIsWalkable = true;
				}
			}
			DistFromWall[Y * Resolution + X] = bIsWalkable ? (float)StoneBlendRadius * 2.0f : 0.0f;
		}
	}
	
	// Propagate distance from wall into walkable
	for (int32 Pass = 0; Pass < StoneBlendRadius * 2; Pass++)
	{
		for (int32 Y = 1; Y < Resolution - 1; Y++)
		{
			for (int32 X = 1; X < Resolution - 1; X++)
			{
				int32 Idx = Y * Resolution + X;
				float MinDist = DistFromWall[Idx];
				MinDist = FMath::Min(MinDist, DistFromWall[(Y-1) * Resolution + X] + 1.0f);
				MinDist = FMath::Min(MinDist, DistFromWall[(Y+1) * Resolution + X] + 1.0f);
				MinDist = FMath::Min(MinDist, DistFromWall[Y * Resolution + (X-1)] + 1.0f);
				MinDist = FMath::Min(MinDist, DistFromWall[Y * Resolution + (X+1)] + 1.0f);
				DistFromWall[Idx] = MinDist;
			}
		}
	}
	
	// Apply stone layer based on distance: only path CENTER
	// CRITICAL: Stone must complement Dirt so that Stone + Dirt = 1.0 in blend zone
	float StoneThreshold = StoneStartDistance; // Use configured value directly
	const float BlendWidth = BlendSettings ? BlendSettings->EdgeBlendWidth : 2.0f;
	int32 PaintedPixels = 0;
	
	// Noise settings (must match GenerateWallWeightmap!)
	const float NoiseScale = 0.15f;
	const float NoiseAmplitude = 3.0f;
	
	for (int32 Y = 0; Y < Resolution; Y++)
	{
		for (int32 X = 0; X < Resolution; X++)
		{
			int32 Idx = Y * Resolution + X;
			float Dist = DistFromWall[Idx];
			
			// Apply same noise as Dirt layer for boundary alignment
			float NoiseValue = FMath::Sin(X * NoiseScale * 2.1f) * FMath::Cos(Y * NoiseScale * 1.7f) +
			                   FMath::Sin((X + Y) * NoiseScale * 0.9f) * 0.5f;
			NoiseValue *= NoiseAmplitude;
			float LocalStoneStart = StoneThreshold + NoiseValue;
			
			// Stone strength calculation - must match Dirt layer's fade-out
			float StoneStrength = 0.0f;
			
			if (Dist >= LocalStoneStart)
			{
				// Full stone (past threshold)
				StoneStrength = 1.0f;
			}
			else if (Dist >= LocalStoneStart - BlendWidth)
			{
				// Blend zone: Stone increases as Dirt decreases
				// At LocalStoneStart - BlendWidth: Stone = 0, Dirt = 1
				// At LocalStoneStart: Stone = 1, Dirt = 0
				float BlendRatio = (Dist - (LocalStoneStart - BlendWidth)) / BlendWidth;
				StoneStrength = BlendRatio; // Inverse of Dirt's (1 - BlendRatio)
			}
			// Else: Stone = 0 (Dirt zone or Grass zone)
			
			Data[Idx] = static_cast<uint8>(FMath::Clamp(StoneStrength, 0.0f, 1.0f) * 255.0f);
			if (Data[Idx] > 0) PaintedPixels++;
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Generated Weightmap - Stone in center, Painted %d pixels."), PaintedPixels);
	
	return Data;
}

TArray<uint8> UDungeonLandscapeTool::GenerateWallWeightmap(int32 Resolution, const FDungeonGrid* Grid, const FLayerBlendSettings* BlendSettings)
{
	TArray<uint8> Data;
	Data.SetNum(Resolution * Resolution);
	
	// Default: 0 (no wall/dirt)
	FMemory::Memset(Data.GetData(), 0, Data.Num());
	
	if (!Grid || Grid->Width == 0 || Grid->Height == 0)
	{
		return Data;
	}
	
	// Get settings from BlendSettings or use defaults
	float DirtStartDistance = BlendSettings ? BlendSettings->DirtStartDistance : 1.0f;
	float StoneStartDistance = BlendSettings ? BlendSettings->StoneStartDistance : 6.0f;
	float BlendRadius = BlendSettings ? BlendSettings->BlendRadius : 12.0f;
	const int32 DirtBlendRadius = static_cast<int32>(BlendRadius);
	
	UE_LOG(LogTemp, Log, TEXT("[GenerateWallWeightmap] DirtStart=%.1f, StoneStart=%.1f, BlendRadius=%.1f"), 
		DirtStartDistance, StoneStartDistance, BlendRadius);
	
	// SIGNED DISTANCE FIELD: 
	// - Negative = inside wall (further from boundary)
	// - Zero = at boundary
	// - Positive = inside walkable (further from boundary)
	
	TArray<float> SignedDist;
	SignedDist.SetNum(Resolution * Resolution);
	
	TArray<bool> IsWalkable;
	IsWalkable.SetNum(Resolution * Resolution);
	
	// First pass: Mark walkable areas and initialize
	for (int32 Y = 0; Y < Resolution; Y++)
	{
		for (int32 X = 0; X < Resolution; X++)
		{
			int32 DungeonX = X / 4;
			int32 DungeonY = Y / 4;
			bool bIsWalkable = false;
			if (Grid->IsValid(DungeonX, DungeonY))
			{
				ETileType Type = Grid->GetTile(DungeonX, DungeonY).Type;
				if (Type == ETileType::Floor || Type == ETileType::Corridor || 
				    Type == ETileType::Door || Type == ETileType::Stair)
				{
					bIsWalkable = true;
				}
			}
			IsWalkable[Y * Resolution + X] = bIsWalkable;
			// Initialize: large positive for walkable, large negative for wall
			SignedDist[Y * Resolution + X] = bIsWalkable ? (float)DirtBlendRadius * 2.0f : -(float)DirtBlendRadius * 2.0f;
		}
	}
	
	// Second pass: Find boundary pixels and set to 0
	for (int32 Y = 1; Y < Resolution - 1; Y++)
	{
		for (int32 X = 1; X < Resolution - 1; X++)
		{
			int32 Idx = Y * Resolution + X;
			bool bCurrent = IsWalkable[Idx];
			
			// Check if any neighbor is different (boundary)
			bool bIsBoundary = false;
			if (bCurrent != IsWalkable[(Y-1) * Resolution + X] ||
			    bCurrent != IsWalkable[(Y+1) * Resolution + X] ||
			    bCurrent != IsWalkable[Y * Resolution + (X-1)] ||
			    bCurrent != IsWalkable[Y * Resolution + (X+1)])
			{
				bIsBoundary = true;
			}
			
			if (bIsBoundary)
			{
				SignedDist[Idx] = bCurrent ? 1.0f : -1.0f; // Just inside boundary
			}
		}
	}
	
	// Propagate signed distance from boundary
	for (int32 Pass = 0; Pass < DirtBlendRadius * 2; Pass++)
	{
		for (int32 Y = 1; Y < Resolution - 1; Y++)
		{
			for (int32 X = 1; X < Resolution - 1; X++)
			{
				int32 Idx = Y * Resolution + X;
				float Current = SignedDist[Idx];
				
				if (Current > 0)
				{
					// Walkable side: find minimum positive + 1
					float MinDist = Current;
					MinDist = FMath::Min(MinDist, SignedDist[(Y-1) * Resolution + X] + 1.0f);
					MinDist = FMath::Min(MinDist, SignedDist[(Y+1) * Resolution + X] + 1.0f);
					MinDist = FMath::Min(MinDist, SignedDist[Y * Resolution + (X-1)] + 1.0f);
					MinDist = FMath::Min(MinDist, SignedDist[Y * Resolution + (X+1)] + 1.0f);
					if (MinDist > 0) SignedDist[Idx] = MinDist;
				}
				else
				{
					// Wall side: find maximum negative - 1
					float MaxDist = Current;
					MaxDist = FMath::Max(MaxDist, SignedDist[(Y-1) * Resolution + X] - 1.0f);
					MaxDist = FMath::Max(MaxDist, SignedDist[(Y+1) * Resolution + X] - 1.0f);
					MaxDist = FMath::Max(MaxDist, SignedDist[Y * Resolution + (X-1)] - 1.0f);
					MaxDist = FMath::Max(MaxDist, SignedDist[Y * Resolution + (X+1)] - 1.0f);
					if (MaxDist < 0) SignedDist[Idx] = MaxDist;
				}
			}
		}
	}
	
	// Generate DIRT layer based on SIGNED distance
	// Negative = inside wall, Positive = inside walkable
	// Use configured thresholds directly
	float DirtStartThreshold = DirtStartDistance;
	float StoneThreshold = StoneStartDistance;
	
	// Blend width for smooth transitions (from settings)
	const float BlendWidth = BlendSettings ? BlendSettings->EdgeBlendWidth : 2.0f;
	
	// Noise settings for natural boundary variation
	const float NoiseScale = 0.15f;  // How frequently the noise changes (lower = more smooth)
	const float NoiseAmplitude = 3.0f; // How much variation in pixels
	
	for (int32 Y = 0; Y < Resolution; Y++)
	{
		for (int32 X = 0; X < Resolution; X++)
		{
			int32 Idx = Y * Resolution + X;
			float Dist = SignedDist[Idx]; // Now using signed distance!
			
			// Apply noise to thresholds for natural boundary variation
			// Using simple pseudo-random noise based on position
			float NoiseValue = FMath::Sin(X * NoiseScale * 2.1f) * FMath::Cos(Y * NoiseScale * 1.7f) +
			                   FMath::Sin((X + Y) * NoiseScale * 0.9f) * 0.5f;
			NoiseValue *= NoiseAmplitude;
			
			// Apply noise to local thresholds (creates wavy boundaries)
			float LocalDirtStart = DirtStartThreshold + NoiseValue; // Full noise at grass/dirt
			float LocalStoneStart = StoneThreshold + NoiseValue; // Full noise at dirt/stone
			
			// Calculate dirt strength with smooth blending at boundaries
			// DirtStart can be negative (extend into wall)
			float DirtStrength = 0.0f;
			
			if (Dist <= LocalDirtStart)
			{
				// Deep wall area - no dirt (grass)
				DirtStrength = 0.0f;
			}
			else if (Dist <= LocalDirtStart + BlendWidth)
			{
				// Gradient: grass -> dirt (at grass/dirt boundary)
				DirtStrength = (Dist - LocalDirtStart) / BlendWidth;
			}
			else if (Dist < LocalStoneStart - BlendWidth)
			{
				// Full dirt zone
				DirtStrength = 1.0f;
			}
			else if (Dist <= LocalStoneStart)
			{
				// Gradient: dirt -> stone (at dirt/stone boundary)
				DirtStrength = (LocalStoneStart - Dist) / BlendWidth;
			}
			else
			{
				// Path center - no dirt (stone)
				DirtStrength = 0.0f;
			}
			
			Data[Y * Resolution + X] = static_cast<uint8>(FMath::Clamp(DirtStrength, 0.0f, 1.0f) * 255.0f);
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Generated Wall Weightmap (Resolution=%d)"), Resolution);
	
	return Data;
}

void UDungeonLandscapeTool::PaintPaths(ADungeonWorldBuilder* DungeonActor, bool bForceFill)
{
	if (!DungeonActor) return;
	
	// 1. Find Landscape (Robust Search)
	ALandscape* Landscape = nullptr;
	if (UWorld* World = DungeonActor->GetWorld())
	{
		// Search by Tag first
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(World, ALandscape::StaticClass(), FoundActors);
		for (AActor* Actor : FoundActors)
		{
			if (Actor->Tags.Contains(FName("DungeonGeneratedLandscape")))
			{
				Landscape = Cast<ALandscape>(Actor);
				break;
			}
		}
		
		// Fallback to reference in DungeonActor
		if (!Landscape && DungeonActor->SpawnedLandscape.IsValid())
		{
			Landscape = DungeonActor->SpawnedLandscape.Get();
		}
	}
	
	if (!Landscape)
	{
		UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Landscape not found. Please click 'Generate Landscape' first."));
		return;
	}


	// Force Disable Edit Layers here as well (in case user skipped regeneration)
#if WITH_EDITOR
#pragma warning(push)
#pragma warning(disable: 4996)
	if (Landscape->CanHaveLayersContent())
	{
		Landscape->ToggleCanHaveLayersContent();
		UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Disabled Edit Layers via ToggleCanHaveLayersContent() inside PaintPaths."));
	}
#pragma warning(pop)
#endif
	
	// 2. Get Grid (Only needed if not Force Fill)
	const FDungeonGrid* GridToUse = nullptr;
	if (!bForceFill)
	{
		if (DungeonActor->DungeonRenderer)
		{
			const FDungeonGrid* Cached = DungeonActor->DungeonRenderer->GetCachedGrid();
			if (Cached && Cached->Width > 0)
			{
				GridToUse = Cached;
			}
		}

		if (!GridToUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Cached Grid is invalid. Please click 'Generate' first (Logic Generation)."));
			return;
		}
	}
	
	// 3. Paint Logic
	// ---------------------------------------------------------
	// [Robust Painting Strategy]
	// Analysis showed 'LandscapeLayers' (Edit Layers) is empty.
	// Therefore, SetAlphaData targeting a specific Edit Layer GUID is risky/pointless.
	// We will use ALandscape::Import() which reliably rebuilds the landscape data.
	// This is destructive (Resets Heightmap) but guarantees the Weightmap is applied.
	// ---------------------------------------------------------
	
	// Skip Debug checks, proceed to logic
	// if (bForceFill) DebugDumpLandscape(Landscape);

	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	if (LandscapeInfo)
	{
		// 3.1 Find correct LayerInfo objects
		// Priority: Theme settings > Auto-scan by name
		ULandscapeLayerInfoObject* PathLayerInfo = nullptr;
		ULandscapeLayerInfoObject* BaseLayerInfo = nullptr;
		ULandscapeLayerInfoObject* WallLayerInfo = nullptr;
		
		// First: Try to get from Theme (user-configured)
		if (DungeonActor->DungeonTheme)
		{
			PathLayerInfo = DungeonActor->DungeonTheme->PathLayerInfo;
			BaseLayerInfo = DungeonActor->DungeonTheme->BaseLayerInfo;
			WallLayerInfo = DungeonActor->DungeonTheme->WallLayerInfo;
			
			if (PathLayerInfo) UE_LOG(LogTemp, Log, TEXT("[LayerInfo] Using PathLayerInfo from Theme: %s"), *PathLayerInfo->GetName());
			if (BaseLayerInfo) UE_LOG(LogTemp, Log, TEXT("[LayerInfo] Using BaseLayerInfo from Theme: %s"), *BaseLayerInfo->GetName());
			if (WallLayerInfo) UE_LOG(LogTemp, Log, TEXT("[LayerInfo] Using WallLayerInfo from Theme: %s"), *WallLayerInfo->GetName());
		}
		
		// Fallback: Auto-scan by name
		for (const auto& LayerSetting : LandscapeInfo->Layers)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[DEBUG] Scanning Layer: Name='%s', InfoObject='%s'"), 
				*LayerSetting.LayerName.ToString(), 
				LayerSetting.LayerInfoObj ? *LayerSetting.LayerInfoObj->GetName() : TEXT("None"));
			
			FName Name = LayerSetting.LayerName;
			FString NameStr = Name.ToString().ToLower();
			
			if (!PathLayerInfo && (NameStr.Contains("layer3") || NameStr.Contains("stone")))
			{
				PathLayerInfo = LayerSetting.LayerInfoObj;
				UE_LOG(LogTemp, Log, TEXT("[DEBUG] -> Auto-matched PATH Layer!"));
			}
			else if (!BaseLayerInfo && (NameStr.Contains("layer1") || NameStr.Contains("grass")))
			{
				BaseLayerInfo = LayerSetting.LayerInfoObj;
				UE_LOG(LogTemp, Log, TEXT("[DEBUG] -> Auto-matched BASE Layer!"));
			}
			else if (!WallLayerInfo && (NameStr.Contains("layer2") || NameStr.Contains("dirt") || NameStr.Contains("soil")))
			{
				WallLayerInfo = LayerSetting.LayerInfoObj;
				UE_LOG(LogTemp, Log, TEXT("[DEBUG] -> Auto-matched WALL Layer!"));
			}
		}

		// Fallback to TargetLayers if not found in Layers (e.g. Editor-only mismatch)
		if (!PathLayerInfo)
		{
			if (const FLandscapeTargetLayerSettings* Found = Landscape->GetTargetLayers().Find("layer3")) PathLayerInfo = Found->LayerInfoObj;
			else if (const FLandscapeTargetLayerSettings* FoundStone = Landscape->GetTargetLayers().Find("stone")) PathLayerInfo = FoundStone->LayerInfoObj;
		}

		if (PathLayerInfo)
		{
			TArray<uint8> WeightmapData;
			// ... (Existing Resolution Logic)
			int32 MinX = MAX_int32;
			int32 MinY = MAX_int32;
			int32 MaxX = MIN_int32;
			int32 MaxY = MIN_int32;
			bool bFoundComponents = false;
			for (ULandscapeComponent* Comp : Landscape->LandscapeComponents)
			{
				if (Comp)
				{
					int32 SectionBaseX = Comp->GetSectionBase().X;
					int32 SectionBaseY = Comp->GetSectionBase().Y;
					int32 CompSize = Comp->ComponentSizeQuads; 
					if (SectionBaseX < MinX) MinX = SectionBaseX;
					if (SectionBaseY < MinY) MinY = SectionBaseY;
					int32 RightX = SectionBaseX + CompSize; 
					int32 BottomY = SectionBaseY + CompSize;
					if (RightX > MaxX) MaxX = RightX;
					if (BottomY > MaxY) MaxY = BottomY;
					bFoundComponents = true;
				}
			}

			// Resolution calculation complete

			if (!bFoundComponents) // Check if any components were found
			{
				UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: No Landscape Components found to calculate resolution."));
				return;
			}
			
			int32 Resolution = (MaxX - MinX); // Assuming 0-based
			if (Resolution <= 0) Resolution = 253; // Fallback
			else Resolution += 1; // +1 for vertices
			
			// Generate Weightmap
			WeightmapData = GenerateWeightmap(Resolution, GridToUse);
			
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Executing Robust Import() API with generated Weightmap. (Warning: Resets Heightmap)"));

			// FALLBACK: Use Import API with the Pattern Data
			TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
			TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer;
			
			TArray<uint16> HeightData;
			HeightData.SetNum(Resolution * Resolution);
			for (int32 i = 0; i < HeightData.Num(); i++) HeightData[i] = 32768; // Flat Height

			TArray<FLandscapeImportLayerInfo> MaterialLayers;

			// 1. Path Layer (using Generated WeightmapData)
			FLandscapeImportLayerInfo PathImportInfo;
			PathImportInfo.LayerName = PathLayerInfo->GetLayerName();
			PathImportInfo.LayerInfo = PathLayerInfo;
			PathImportInfo.LayerData = WeightmapData;
			MaterialLayers.Add(PathImportInfo);

			// 2. Wall Layer (Dirt on raised terrain)
			TArray<uint8> WallWeightmapData;
			bool bHasWallLayer = false;
			
			// Debug: Log state of WallLayer conditions
			UE_LOG(LogTemp, Warning, TEXT("[WallLayer Debug] WallLayerInfo=%s, Theme=%s, bEnableRaisedTerrain=%s"),
				WallLayerInfo ? *WallLayerInfo->GetName() : TEXT("NULL"),
				DungeonActor->DungeonTheme ? TEXT("Valid") : TEXT("NULL"),
				(DungeonActor->DungeonTheme && DungeonActor->DungeonTheme->bEnableRaisedTerrain) ? TEXT("True") : TEXT("False"));
			
			if (WallLayerInfo && DungeonActor->DungeonTheme && DungeonActor->DungeonTheme->bEnableRaisedTerrain)
			{
				// Create BlendSettings from Theme
				FLayerBlendSettings BlendSettings;
				BlendSettings.DirtStartDistance = DungeonActor->DungeonTheme->DirtStartDistance;
				BlendSettings.StoneStartDistance = DungeonActor->DungeonTheme->StoneStartDistance;
				BlendSettings.BlendRadius = DungeonActor->DungeonTheme->LayerBlendRadius;
				BlendSettings.EdgeBlendWidth = DungeonActor->DungeonTheme->EdgeBlendWidth;
				
				WallWeightmapData = GenerateWallWeightmap(Resolution, GridToUse, &BlendSettings);
				bHasWallLayer = true;
				
				FLandscapeImportLayerInfo WallImportInfo;
				WallImportInfo.LayerName = WallLayerInfo->GetLayerName();
				WallImportInfo.LayerInfo = WallLayerInfo;
				WallImportInfo.LayerData = WallWeightmapData;
				MaterialLayers.Add(WallImportInfo);
				
				UE_LOG(LogTemp, Log, TEXT("DungeonLandscapeTool: Added Wall Layer '%s' for raised terrain"), *WallLayerInfo->GetLayerName().ToString());
			}
			
			// 3. Base Layer (Inverse of Path + Wall)
			if (BaseLayerInfo)
			{
				TArray<uint8> BaseWeightmapData;
				BaseWeightmapData.SetNum(WeightmapData.Num());
				for (int32 i = 0; i < WeightmapData.Num(); i++)
				{
					// Base = 255 - Path - Wall
					int32 Used = WeightmapData[i];
					if (bHasWallLayer)
					{
						Used += WallWeightmapData[i];
					}
					BaseWeightmapData[i] = static_cast<uint8>(FMath::Max(0, 255 - Used));
				}

				FLandscapeImportLayerInfo BaseImportInfo;
				BaseImportInfo.LayerName = BaseLayerInfo->GetLayerName();
				BaseImportInfo.LayerInfo = BaseLayerInfo;
				BaseImportInfo.LayerData = BaseWeightmapData;
				MaterialLayers.Add(BaseImportInfo);
			}
			
			// Use Empty GUID for Base/Default Data keys, matching Engine "New Landscape" pattern
			FGuid BaseKey = FGuid(); 
			HeightDataPerLayers.Add(BaseKey, HeightData);
			MaterialLayerDataPerLayer.Add(BaseKey, MaterialLayers);

			// Import() requires empty components list AND empty Edit Layers
			// This ensures CreateDefaultLayer() is called and propagates data to new components.
			if (Landscape->LandscapeComponents.Num() > 0 || Landscape->GetLayersConst().Num() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Clearing existing Landscape Components and Layers before Import..."));
				
				// 1. Destroy Components
				for (int32 i = Landscape->LandscapeComponents.Num() - 1; i >= 0; i--)
				{
					if (Landscape->LandscapeComponents[i])
					{
						Landscape->LandscapeComponents[i]->DestroyComponent();
					}
				}
				Landscape->LandscapeComponents.Empty();
				Landscape->CollisionComponents.Empty();

				// 2. Destroy Edit Layers (Reverse order)
				int32 NumLayers = Landscape->GetLayersConst().Num();
				if (NumLayers > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Deleting %d existing Edit Layers..."), NumLayers);
					for (int32 i = NumLayers - 1; i >= 0; i--)
					{
						Landscape->DeleteLayer(i);
					}
				}
			}

			// Correct Import Signature
			// 1st Arg: New Guid for the Target Layer
			// Maps: Typed with Empty Guid (BaseKey)
			// Last Arg: Empty Layer View (Engine creates default)
			Landscape->Import(FGuid::NewGuid(), 0, 0, Resolution - 1, Resolution - 1, 
				Landscape->NumSubsections, 
				Landscape->SubsectionSizeQuads, 
				HeightDataPerLayers, 
				nullptr, // InHeightmapFileName
				MaterialLayerDataPerLayer, 
				ELandscapeImportAlphamapType::Additive,
				TArrayView<const FLandscapeLayer>()); // InImportLayers
				
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Import() Complete. Landscape Updated."));
			return; 
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("DungeonLandscapeTool: Path Layer 'layer3' or 'stone' not found. Please assign it in Landscape Mode."));
		}
	}
	}


void UDungeonLandscapeTool::DebugDumpLandscape(ALandscape* Landscape)
{
	if (!Landscape) return;

	UE_LOG(LogTemp, Log, TEXT("=== Landscape Debug Dump ==="));
	
	// 1. Property Dump
	UE_LOG(LogTemp, Log, TEXT("Properties of %s:"), *Landscape->GetName());
	for (TFieldIterator<FProperty> PropIt(Landscape->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;
		UE_LOG(LogTemp, Log, TEXT("  - %s (%s)"), *Prop->GetName(), *Prop->GetCPPType());
	}

	// 2. Specific Reflection Check for "LandscapeLayers"
	FProperty* LayersProp = Landscape->GetClass()->FindPropertyByName("LandscapeLayers");
	if (LayersProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found 'LandscapeLayers' property!"));
		
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(LayersProp);
		if (ArrayProp)
		{
			FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Landscape));
			int32 Num = ArrayHelper.Num();
			UE_LOG(LogTemp, Warning, TEXT("  -> Array Size: %d"), Num);
			
			for (int32 i = 0; i < Num; ++i)
			{
				void* ItemPtr = ArrayHelper.GetRawPtr(i);
				// Dump visible properties of the struct
				if (FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner))
				{
					UScriptStruct* Struct = StructInner->Struct;
					UE_LOG(LogTemp, Log, TEXT("    [%d] struct: %s"), i, *Struct->GetName());
					
					// Look for GUID
					if (FProperty* GuidProp = Struct->FindPropertyByName("Guid"))
					{
						if (FStructProperty* GuidStructProp = CastField<FStructProperty>(GuidProp))
						{
							FGuid* GuidPtr = GuidStructProp->ContainerPtrToValuePtr<FGuid>(ItemPtr);
							if (GuidPtr)
							{
								UE_LOG(LogTemp, Warning, TEXT("    -> GUID: %s"), *GuidPtr->ToString());
							}
						}
					}
					// Look for Name
					if (FProperty* NameProp = Struct->FindPropertyByName("Name"))
					{
						if (FNameProperty* NameNameProp = CastField<FNameProperty>(NameProp))
						{
							FName NameVal = *NameNameProp->ContainerPtrToValuePtr<FName>(ItemPtr);
							UE_LOG(LogTemp, Warning, TEXT("    -> Name: %s"), *NameVal.ToString());
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could NOT find 'LandscapeLayers' property via Reflection. API might have changed completely."));
	}

	UE_LOG(LogTemp, Log, TEXT("============================"));
}

// ==================== TERRACED TERRAIN FUNCTIONS ====================

UDungeonLandscapeTool::FRoomClusterData UDungeonLandscapeTool::AnalyzeRoomClusters(const FDungeonGrid* Grid, int32 Seed, float MaxHeightVariation)
{
	FRoomClusterData Result;
	if (!Grid || Grid->Width == 0 || Grid->Height == 0)
	{
		return Result;
	}
	
	Result.GridWidth = Grid->Width;
	Result.GridHeight = Grid->Height;
	Result.ClusterMap.Init(-1, Grid->Width * Grid->Height);
	
	FRandomStream RandStream(Seed);
	int32 CurrentClusterID = 0;
	
	// BFS Flood Fill to identify room clusters
	for (int32 StartY = 0; StartY < Grid->Height; StartY++)
	{
		for (int32 StartX = 0; StartX < Grid->Width; StartX++)
		{
			int32 StartIdx = StartY * Grid->Width + StartX;
			
			// Skip if already assigned or not a Floor tile
			if (Result.ClusterMap[StartIdx] != -1)
				continue;
				
			if (!Grid->IsValid(StartX, StartY))
				continue;
				
			ETileType Type = Grid->GetTile(StartX, StartY).Type;
			if (Type != ETileType::Floor)
				continue;
			
			// BFS from this Floor tile
			TQueue<FIntPoint> Queue;
			Queue.Enqueue(FIntPoint(StartX, StartY));
			Result.ClusterMap[StartIdx] = CurrentClusterID;
			
			while (!Queue.IsEmpty())
			{
				FIntPoint Current;
				Queue.Dequeue(Current);
				
				// Check 4 neighbors
				const FIntPoint Neighbors[] = {
					FIntPoint(Current.X - 1, Current.Y),
					FIntPoint(Current.X + 1, Current.Y),
					FIntPoint(Current.X, Current.Y - 1),
					FIntPoint(Current.X, Current.Y + 1)
				};
				
				for (const FIntPoint& Neighbor : Neighbors)
				{
					if (Neighbor.X < 0 || Neighbor.X >= Grid->Width ||
					    Neighbor.Y < 0 || Neighbor.Y >= Grid->Height)
						continue;
						
					int32 NeighborIdx = Neighbor.Y * Grid->Width + Neighbor.X;
					if (Result.ClusterMap[NeighborIdx] != -1)
						continue;
						
					if (!Grid->IsValid(Neighbor.X, Neighbor.Y))
						continue;
						
					ETileType NeighborType = Grid->GetTile(Neighbor.X, Neighbor.Y).Type;
					if (NeighborType == ETileType::Floor)
					{
						Result.ClusterMap[NeighborIdx] = CurrentClusterID;
						Queue.Enqueue(Neighbor);
					}
				}
			}
			
			// Assign random height to this cluster
			float Height = RandStream.FRandRange(-MaxHeightVariation, MaxHeightVariation);
			Result.ClusterHeights.Add(CurrentClusterID, Height);
			UE_LOG(LogTemp, Log, TEXT("[Terraced] Cluster %d assigned height: %.1f cm"), CurrentClusterID, Height);
			
			CurrentClusterID++;
		}
	}
	
	return Result;
}

float UDungeonLandscapeTool::GetInterpolatedTerraceHeight(int32 TileX, int32 TileY, const FDungeonGrid* Grid, const FRoomClusterData& ClusterData)
{
	if (!Grid || ClusterData.ClusterMap.Num() == 0)
		return 0.0f;
	
	TileX = FMath::Clamp(TileX, 0, ClusterData.GridWidth - 1);
	TileY = FMath::Clamp(TileY, 0, ClusterData.GridHeight - 1);
	
	if (!Grid->IsValid(TileX, TileY))
		return 0.0f;
	
	ETileType Type = Grid->GetTile(TileX, TileY).Type;
	int32 TileIdx = TileY * ClusterData.GridWidth + TileX;
	
	// Floor tiles: Use assigned cluster height
	if (Type == ETileType::Floor)
	{
		int32 ClusterID = ClusterData.ClusterMap[TileIdx];
		if (ClusterID >= 0 && ClusterData.ClusterHeights.Contains(ClusterID))
		{
			return ClusterData.ClusterHeights[ClusterID];
		}
		return 0.0f;
	}
	
	// Corridor/Door tiles: Interpolate between nearest rooms (Voronoi Ramps)
	if (Type == ETileType::Corridor || Type == ETileType::Door)
	{
		// Find nearest Floor tiles in each direction and interpolate
		float TotalWeight = 0.0f;
		float WeightedHeight = 0.0f;
		const int32 SearchRadius = 20;
		
		for (int32 DY = -SearchRadius; DY <= SearchRadius; DY++)
		{
			for (int32 DX = -SearchRadius; DX <= SearchRadius; DX++)
			{
				int32 NX = TileX + DX;
				int32 NY = TileY + DY;
				
				if (NX < 0 || NX >= ClusterData.GridWidth ||
				    NY < 0 || NY >= ClusterData.GridHeight)
					continue;
				
				int32 NIdx = NY * ClusterData.GridWidth + NX;
				int32 ClusterID = ClusterData.ClusterMap[NIdx];
				
				if (ClusterID >= 0 && ClusterData.ClusterHeights.Contains(ClusterID))
				{
					float Dist = FMath::Sqrt((float)(DX * DX + DY * DY));
					if (Dist < 0.1f) Dist = 0.1f;
					float Weight = 1.0f / (Dist * Dist); // Inverse square distance weighting
					
					WeightedHeight += ClusterData.ClusterHeights[ClusterID] * Weight;
					TotalWeight += Weight;
				}
			}
		}
		
		if (TotalWeight > 0.0f)
		{
			return WeightedHeight / TotalWeight;
		}
	}
	
	return 0.0f;
}
