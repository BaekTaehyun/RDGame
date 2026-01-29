#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/DungeonConfig.h" // FDungeonGenConfig
#include "DungeonGrid.h" // FDungeonGrid
#include "DungeonWorldBuilder.generated.h"

class UDungeonThemeAsset;
class UDungeonRendererComponent;

// Delegate for Editor Tool interaction (Moved from DungeonFullTestActor)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRequestDungeonLandscape, class ADungeonWorldBuilder*, const struct FDungeonGrid*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRequestPaintPath, class ADungeonWorldBuilder*);

/**
 * Dedicated Actor for PCG and Landscape-based Dungeon Generation.
 * Replaces ADungeonFullTestActor for non-tile visualization.
 */
UCLASS()
class DUNGEONGENERATOR_API ADungeonWorldBuilder : public AActor
{
	GENERATED_BODY()
	
public:	
	ADungeonWorldBuilder();

	// Static Delegate to trigger Landscape Tool in Editor Module
	static FOnRequestDungeonLandscape OnRequestLandscape;
	static FOnRequestPaintPath OnRequestPaintPath;

	// --- Actions ---
	
	UFUNCTION(CallInEditor, Category = "Dungeon Actions")
	void Generate();

	UFUNCTION(CallInEditor, Category = "Dungeon Actions")
	void Clear();

	/** Triggers Landscape Generation based on cached grid */
	UFUNCTION(CallInEditor, Category = "Dungeon Actions")
	void GenerateLandscape();
	
	/** Triggers Path Painting on the landscape (Requires valid Landscape & Layers) */
	UFUNCTION(CallInEditor, Category = "Dungeon Actions", meta=(DisplayName="Paint Dungeon Paths"))
	void PaintDungeonPaths();

	// --- Config Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
	UDungeonThemeAsset* DungeonTheme;

	// Data Table Row Handle for Loading Config
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core", meta = (RowType = "/Script/DungeonGenerator.DungeonGenConfig"))
	FDataTableRowHandle ConfigTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core")
	FDungeonGenConfig GeneratorConfig;
	
	// --- Seed Override ---
	/** If true, uses the SeedOverride value instead of the Config table's Seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core", meta=(InlineEditConditionToggle))
	bool bUseSeedOverride = false;

	/** Override Seed value for quick iteration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Core", meta=(EditCondition="bUseSeedOverride"))
	int32 SeedOverride = 12345;

	// --- Components ---
	/** Box Component provides bounds for PCG system */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* BoundsBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDungeonRendererComponent* DungeonRenderer;



	/** Reference to spawned Landscape for cleanup */
	UPROPERTY(Transient)
	TWeakObjectPtr<class ALandscape> SpawnedLandscape;

protected:
	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};
