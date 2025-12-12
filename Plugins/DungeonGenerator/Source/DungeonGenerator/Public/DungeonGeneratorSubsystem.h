#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DungeonGrid.h"
#include "Data/DungeonConfig.h"
#include "DungeonGeneratorSubsystem.generated.h"

// Forward declarations
class UDungeonAlgorithm;
class UBSPGenerator;
class UCellularAutomataGenerator;
class UDungeonTileRenderer;
class UDungeonMeshGenerator;
class UDungeonLightingManager;
class UDungeonNavigationBuilder;
class UInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class APointLight;

/**
 * Dungeon Generator Subsystem
 * Handles dungeon generation using different algorithms
 */
UCLASS()
class DUNGEONGENERATOR_API UDungeonGeneratorSubsystem
    : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  // Subsystem lifecycle
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  /**
   * Generate a dungeon using the specified algorithm
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon")
  FDungeonGrid GenerateDungeon(int32 Width, int32 Height, int32 Seed,
                               EDungeonAlgorithmType AlgorithmType);

  /**
   * Get the currently generated grid
   */
  UFUNCTION(BlueprintPure, Category = "Dungeon")
  FDungeonGrid GetCurrentGrid() const { return CurrentGrid; }

  /**
   * Render the dungeon with walls, ceiling, floor, and lighting
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  void RenderDungeon(const FDungeonGrid &Grid,
                     EDungeonAlgorithmType AlgorithmType, AActor *OwnerActor);

  /**
   * Clear all rendered dungeon elements
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  void ClearRenderedDungeon();

  // Rendering Settings
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
  bool bEnableWallRendering = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
  bool bEnableCeiling = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
  bool bEnableFloor = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
  bool bAutoPlaceLights = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
  bool bGenerateNavMesh = true;

  // Asset References (BSP)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets | BSP")
  TMap<uint8, UStaticMesh *> BSPWallMeshes;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets | BSP")
  UStaticMesh *BSPCeilingMesh;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets | BSP")
  UStaticMesh *BSPFloorMesh;

  // Asset References (CA)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets | CA")
  UMaterialInterface *CaveWallMaterial;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets | CA")
  UMaterialInterface *CaveFloorMaterial;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets | CA")
  UMaterialInterface *CaveCeilingMaterial;

  // Lighting
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
  TSubclassOf<APointLight> RoomLightClass;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
  TSubclassOf<APointLight> CorridorLightClass;

private:
  UPROPERTY()
  FDungeonGrid CurrentGrid;

  UPROPERTY()
  FRandomStream RandomStream;

  // Rendering Managers
  UPROPERTY()
  UDungeonTileRenderer *TileRenderer;

  UPROPERTY()
  UDungeonMeshGenerator *MeshGenerator;

  UPROPERTY()
  UDungeonLightingManager *LightingManager;

  UPROPERTY()
  UDungeonNavigationBuilder *NavigationBuilder;

  // Rendered Components (stored for cleanup)
  UPROPERTY()
  TArray<UInstancedStaticMeshComponent *> InstancedMeshComponents;

  UPROPERTY()
  TArray<UProceduralMeshComponent *> ProceduralMeshComponents;

  UPROPERTY()
  TArray<APointLight *> SpawnedLights;
};
