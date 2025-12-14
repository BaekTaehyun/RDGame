#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DungeonConfig.generated.h"



class UDungeonThemeAsset;
class UDungeonAlgorithm;

UENUM(BlueprintType)
enum class EDungeonAlgorithmType : uint8 {
  BSP UMETA(DisplayName = "Binary Space Partitioning"),
  CellularAutomata UMETA(DisplayName = "Cellular Automata"),
  PresetAssembly UMETA(DisplayName = "Preset Module Assembly (Diablo Style)")
};

/**
 * Configuration struct for Dungeon Generation.
 * Designed to be used as a Data Table Row.
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonGenConfig : public FTableRowBase {
  GENERATED_BODY()

public:
  // --- Generation Settings ---

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
  int32 Width = 50;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
  int32 Height = 50;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
  int32 Seed = 12345;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
  EDungeonAlgorithmType Algorithm = EDungeonAlgorithmType::BSP;

  // Algorithm Specifics (Consider moving to a separate config if this grows)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation",
            meta = (EditCondition = "Algorithm == EDungeonAlgorithmType::BSP"))
  int32 MinRoomSize = 6;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation",
            meta = (EditCondition = "Algorithm == EDungeonAlgorithmType::BSP"))
  int32 CorridorWidth = 2;

  // --- Preset Assembly Specifics ---
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation",
            meta = (EditCondition =
                        "Algorithm == EDungeonAlgorithmType::PresetAssembly"))
  int32 MaxRoomCount = 20;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation",
            meta = (EditCondition =
                        "Algorithm == EDungeonAlgorithmType::PresetAssembly"))
  TSoftObjectPtr<class UPresetModuleDatabase> PresetDatabase;

  // --- Theme ---

  /** The Visual Theme to use for this dungeon (Meshes, Materials) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
  TSoftObjectPtr<UDungeonThemeAsset> Theme;

  // --- System Settings ---

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
  bool bUseChunking = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System",
            meta = (EditCondition = "bUseChunking"))
  int32 ChunkSize = 10;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
  bool bEnableStreaming = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System",
            meta = (EditCondition = "bEnableStreaming"))
  int32 StreamingDistance = 2;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System",
            meta = (EditCondition = "bEnableStreaming"))
  float StreamingUpdateInterval = 0.5f;

  // --- Optimization ---

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization")
  bool bEnableChunkMerging = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization")
  bool bRemoveOriginalAfterMerge = true;
};
