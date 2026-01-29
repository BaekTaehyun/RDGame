#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DungeonGrid.h"
#include "DungeonPCGRenderer.generated.h"

class UPCGGraph;
class UPCGComponent;
class UDungeonThemeAsset;

/**
 * Handles PCG-based visualization for the Dungeon Generator.
 * Separated from DungeonTileRenderer to decouple PCG logic from legacy HISM logic.
 */
UCLASS(BlueprintType, Blueprintable)
class DUNGEONGENERATOR_API UDungeonPCGRenderer : public UObject
{
	GENERATED_BODY()

public:
	UDungeonPCGRenderer();

	/**
	 * Spawns PCG graphs based on the dungeon grid and theme.
	 * @param Grid The dungeon layout.
	 * @param OwnerActor The actor to attach PCG components to.
	 * @param Theme The theme defining which graphs to use.
	 * @param Seed - Random seed to ensure consistent PCG results
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|PCG")
	void GeneratePCG(const FDungeonGrid& Grid, AActor* OwnerActor, const UDungeonThemeAsset* Theme, int32 Seed);

	/**
	 * Cleans up all spawned PCG components on the specified owner.
	 * Use this after level load when CachedOwner may be null.
	 */
	void Cleanup(AActor* OwnerActor);

	// Cache for Readers
	const FDungeonGrid& GetCachedGrid() const { return CachedGrid; }
	const UDungeonThemeAsset* GetCachedTheme() const { return CachedTheme; }

protected:
	/** Cached owner actor for finding PCG components during Cleanup */
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedOwner;

	/** Cached grid for bounds calculation */
	FDungeonGrid CachedGrid;

	/** Cached theme for tile size data */
	const UDungeonThemeAsset* CachedTheme = nullptr;

	/** Helper to spawn a single PCG graph */
	void SpawnPCGGraph(AActor* Owner, UPCGGraph* Graph, const FName& Name, int32 Seed);
};
