#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/DungeonConfig.h" 
#include "DungeonThemeAsset.generated.h"

class UPCGGraph;
class UMaterialInterface;

/**
 * Data Asset that defines the visual theme of a dungeon.
 * Logic is separated into FDungeonGenConfig.
 */
UCLASS(BlueprintType)
class DUNGEONGENERATOR_API UDungeonThemeAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Geometric Properties ---
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float TileSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float WallHeight = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	FVector WallPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	FVector CornerWallPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	FVector ThroughWallPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	FVector FloorPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	FVector CeilingPivotOffset = FVector(0, 0, 400.0f);

	// --- Meshes ---

	/** Map of Neighbor Mask (0-15) to Wall Mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	TMap<int32, UStaticMesh*> WallMeshTable;

	/** Fallback wall mesh if table entry is missing or table is empty */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* FallbackWallMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* FloorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* CeilingMesh;

	// --- PCG Integration ---

	/** Which algorithm this theme is designed for. Hides irrelevant properties. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	EDungeonAlgorithmType CompatibilityMode = EDungeonAlgorithmType::BSP;

	// --- BSP Mode PCG Graphs ---

	/** PCG Graph for Room Floors (e.g. Center decorations, furniture) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|BSP", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::BSP", EditConditionHides))
	TObjectPtr<UPCGGraph> RoomPCGGraph;

	/** PCG Graph for Corridors (e.g. Torches, pipes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|BSP", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::BSP", EditConditionHides))
	TObjectPtr<UPCGGraph> CorridorPCGGraph;

	/** PCG Graph to spawn for blocking Walls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|BSP", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::BSP", EditConditionHides))
	TObjectPtr<UPCGGraph> WallPCGGraph;

	/** PCG Graph for Corner Walls (fills gaps at wall corners) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|BSP", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::BSP", EditConditionHides))
	TObjectPtr<UPCGGraph> CornerWallPCGGraph;

	/** PCG Graph for Through Walls (walkable on both sides) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|BSP", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::BSP", EditConditionHides))
	TObjectPtr<UPCGGraph> ThroughWallPCGGraph;

	/** PCG Graph for Doors (e.g. Door frames, arches) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|BSP", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::BSP", EditConditionHides))
	TObjectPtr<UPCGGraph> DoorPCGGraph;

	/** PCG Graph to spawn on General Floors (fallback) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|BSP", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::BSP", EditConditionHides))
	TObjectPtr<UPCGGraph> FloorPCGGraph;

	// --- Cellular Automata Mode PCG Graphs ---

	/** PCG Graph for Nature assets on non-walkable areas (trees, rocks, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|CellularAutomata", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides))
	TObjectPtr<UPCGGraph> NaturePCGGraph;

	/** PCG Graph for Path decoration on walkable areas (grass, flowers, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|CellularAutomata", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides))
	TObjectPtr<UPCGGraph> PathPCGGraph;

	/** Density of trees/nature to spawn (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|CellularAutomata", 
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides, ClampMin="0.0", ClampMax="1.0"))
	float TreeDensity = 0.5f;

	// --- PCG Zone Settings (3 Zone Types) ---

	/** PCG Graph for Path Edge zone (roadside decorations, small rocks, grass patches) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Zones",
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides))
	TObjectPtr<UPCGGraph> PathEdgePCGGraph;

	/** PCG Graph for Path zone (road decorations, dirt patches, gravel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Zones",
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides))
	TObjectPtr<UPCGGraph> PathZonePCGGraph;

	/** PCG Graph for Building zone (room interiors, structural elements) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Zones",
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides))
	TObjectPtr<UPCGGraph> BuildingPCGGraph;

	/** Width of path edge zone in dungeon tiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Zones",
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides, ClampMin="1", ClampMax="10"))
	int32 PathEdgeWidth = 3;

	/** Enable zone-based PCG placement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Zones",
		meta=(EditCondition="CompatibilityMode == EDungeonAlgorithmType::CellularAutomata", EditConditionHides))
	bool bEnableZonePCG = false;

	// --- Common (Both Modes) ---

	/** Material to apply to the generated Landscape */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape")
	TObjectPtr<UMaterialInterface> LandscapeMaterial;

	/** Layer Info for Path/Dungeon areas (layer3/stone) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape")
	TObjectPtr<class ULandscapeLayerInfoObject> PathLayerInfo;

	/** Layer Info for Base/Grass areas (layer1/grass) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape")
	TObjectPtr<class ULandscapeLayerInfoObject> BaseLayerInfo;

	/** Layer Info for Wall/Dirt areas (layer2/dirt) - Used on raised terrain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape")
	TObjectPtr<class ULandscapeLayerInfoObject> WallLayerInfo;

	// --- Raised Terrain Settings ---

	/** Enable raised terrain for wall/forest areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape")
	bool bEnableRaisedTerrain = false;

	/** Height to raise wall areas (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="0", ClampMax="500"))
	float WallTerrainHeight = 100.0f;

	/** Noise variation for natural height variation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="0", ClampMax="100"))
	float WallHeightNoise = 50.0f;

	/** Edge steepness (1=gentle slope, 8=cliff-like) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="1", ClampMax="8"))
	int32 WallEdgeSteepness = 2;

	// --- Layer Blend Settings (configurable thresholds) ---

	/** Distance from wall where Dirt starts (pixels). Negative = extend into wall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="-10", ClampMax="20"))
	float DirtStartDistance = 0.0f;

	/** Distance from wall where Stone starts (pixels). Higher = more dirt, less stone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="2", ClampMax="30"))
	float StoneStartDistance = 6.0f;

	/** Blend radius for layer transitions (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="2", ClampMax="20"))
	float LayerBlendRadius = 12.0f;

	/** Edge blend width for smooth transitions (pixels). Higher = softer edges */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="1", ClampMax="10"))
	float EdgeBlendWidth = 2.0f;

	// --- Terraced Terrain Settings (계단식 대지) ---

	/** Enable terraced terrain - each room at different height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain"))
	bool bEnableTerracedTerrain = false;

	/** Maximum height variation between rooms (cm). Higher = more dramatic terraces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableTerracedTerrain", ClampMin="0", ClampMax="1000"))
	float MaxRoomHeightVariation = 400.0f;

	/** Seed for random room height assignment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableTerracedTerrain"))
	int32 TerrainSeed = 12345;

	// --- Path Ground Noise Settings (길 노이즈) ---

	/** Base depression depth for walkable areas (cm). How much to lower paths */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="0", ClampMax="500"))
	float PathDepressionDepth = 150.0f;

	/** Large undulation amplitude (cm). Creates gentle rolling hills on paths */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="0", ClampMax="100"))
	float PathNoiseAmplitude1 = 25.0f;

	/** Medium bump amplitude (cm). Creates medium-sized ground variation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="0", ClampMax="50"))
	float PathNoiseAmplitude2 = 10.0f;

	/** Fine detail amplitude (cm). Creates gravel-like texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="0", ClampMax="20"))
	float PathNoiseAmplitude3 = 5.0f;

	/** Domain warp strength. Higher = more organic/curved noise patterns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Landscape", meta=(EditCondition="bEnableRaisedTerrain", ClampMin="0", ClampMax="100"))
	float PathDomainWarp = 30.0f;


	// --- Generation Flags ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	bool bGenerateFloor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	bool bGenerateCeiling = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	bool bGenerateFloorUnderWalls = true;

	// --- LOD Settings ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bUseLOD = true;

	/** Distances for LOD transitions (e.g. [1000, 2500, 5000]) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD", meta=(EditCondition="bUseLOD"))
	TArray<float> LODDistances;

	// --- Materials / PostProcess ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	bool bUseDynamicMaterials = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials", meta=(EditCondition="bUseDynamicMaterials", ClampMin="0.0", ClampMax="1.0"))
	float WetnessIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials", meta=(EditCondition="bUseDynamicMaterials", ClampMin="0.0", ClampMax="1.0"))
	float MossIntensity = 0.0f;
};
