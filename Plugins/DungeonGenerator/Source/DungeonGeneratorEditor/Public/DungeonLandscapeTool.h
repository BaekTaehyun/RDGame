#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DungeonWorldBuilder.h"
#include "DungeonLandscapeTool.generated.h"

class ALandscape;

/**
 * Editor-only tool to generate Landscape from Dungeon Grid.
 */
UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonLandscapeTool : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Generates a Landscape actor based on the Dungeon Grid.
	 * @param DungeonActor The source dungeon builder actor.
	 * @param bUpdateExisting If true, attempts to update existing landscape (not fully implemented).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Editor")
	static ALandscape* GenerateLandscape(class ADungeonWorldBuilder* DungeonActor, bool bUpdateExisting = true);

	// Native C++ version to avoid UHT issues with struct pointers
	static ALandscape* GenerateLandscapeWithGrid(class ADungeonWorldBuilder* DungeonActor, bool bUpdateExisting, const struct FDungeonGrid* InGrid);

	/**
	 * Paints the landscape layers (e.g. paths) based on the dungeon grid.
	 * Requires that the landscape has layers assigned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Landscape")
	static void PaintPaths(class ADungeonWorldBuilder* DungeonActor, bool bForceFill = false);

	// Debug Helper
	static void DebugDumpLandscape(ALandscape* Landscape);

private:
	// Raised Terrain Settings struct for Heightmap/Weightmap generation
	struct FRaisedTerrainSettings
	{
		bool bEnabled = false;
		float Height = 100.0f;
		float HeightNoise = 50.0f;
		int32 EdgeSteepness = 2;
		
		// Path noise settings
		float PathDepressionDepth = 150.0f;
		float PathNoiseAmplitude1 = 25.0f;
		float PathNoiseAmplitude2 = 10.0f;
		float PathNoiseAmplitude3 = 5.0f;
		float PathDomainWarp = 30.0f;
	};

	// Layer Blend Settings for configurable thresholds
	struct FLayerBlendSettings
	{
		float DirtStartDistance = 0.0f;
		float StoneStartDistance = 6.0f;
		float BlendRadius = 12.0f;
		float EdgeBlendWidth = 2.0f;
	};

	// Terraced Terrain Settings (계단식 대지)
	struct FTerracedTerrainSettings
	{
		bool bEnabled = false;
		float MaxHeightVariation = 400.0f; // cm
		int32 Seed = 12345;
	};

	// Room cluster data for terraced terrain
	struct FRoomClusterData
	{
		TArray<int32> ClusterMap; // Pixel -> ClusterID
		TMap<int32, float> ClusterHeights; // ClusterID -> Height
		int32 GridWidth = 0;
		int32 GridHeight = 0;
	};

	// Changed signature to take explicit dimensions instead of Grid struct, allowing fallback to Actor properties.
	static TArray<uint16> GenerateHeightmap(int32 Width, int32 Height, int32& OutResolution, int32& OutComponentSize, int32& OutSectionsPerComponent, const struct FDungeonGrid* Grid = nullptr, const FRaisedTerrainSettings* RaisedSettings = nullptr, const FTerracedTerrainSettings* TerracedSettings = nullptr);
	
	/**
	 * Generates a weightmap for landscape layers based on tile types.
	 * @param Resolution Heightmap resolution (from GenerateHeightmap).
	 * @param Grid The dungeon grid.
	 * @param BlendSettings Optional layer blend settings.
	 * @return Weightmap data where 255 = Path (Layer 3), 0 = Grass (Layer 1).
	 */
	static TArray<uint8> GenerateWeightmap(int32 Resolution, const struct FDungeonGrid* Grid, const FLayerBlendSettings* BlendSettings = nullptr);
	
	/** Generate separate weightmap for Wall layer (dirt on raised terrain) */
	static TArray<uint8> GenerateWallWeightmap(int32 Resolution, const struct FDungeonGrid* Grid, const FLayerBlendSettings* BlendSettings = nullptr);

	// --- Terraced Terrain Helper Functions ---
	static FRoomClusterData AnalyzeRoomClusters(const struct FDungeonGrid* Grid, int32 Seed, float MaxHeightVariation);
	static float GetInterpolatedTerraceHeight(int32 TileX, int32 TileY, const struct FDungeonGrid* Grid, const FRoomClusterData& ClusterData);
};
