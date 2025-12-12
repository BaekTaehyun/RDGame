#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonThemeAsset.generated.h"

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
