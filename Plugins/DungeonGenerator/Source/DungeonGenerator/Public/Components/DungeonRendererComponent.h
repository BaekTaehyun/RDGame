#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Rendering/DungeonTileRenderer.h"
#include "DungeonGrid.h"
#include "Data/DungeonConfig.h"
#include "DungeonRendererComponent.generated.h"

class UDungeonThemeAsset;
class UDungeonChunkStreamer;
class UDynamicMeshComponent;

/**
 * Component responsible for rendering the dungeon and managing HISM components.
 * Encapsulates PIE fixups (PostLoad) and Collision management.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonRendererComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDungeonRendererComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// --- Dependencies ---
	
	UPROPERTY()
	UDungeonTileRenderer* TileRenderer;

	// --- State ---

	/** Map of Chunk Coordinate to Wall HISMs (for Streaming) */
	TMap<FIntPoint, TArray<UHierarchicalInstancedStaticMeshComponent*>> ChunkHISMMap;

	/** Map of Chunk Coordinate to Merged Dynamic Mesh (Optimization) */
	UPROPERTY()
	TMap<FIntPoint, UDynamicMeshComponent*> MergedChunkMeshes;

	/** Array of generated Wall HISMs (for cleanup) */
	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> CreatedWallHISMs;

	// --- Managed Components ---
    
    UPROPERTY()
    UHierarchicalInstancedStaticMeshComponent* FloorHISM;
    
    UPROPERTY()
    UHierarchicalInstancedStaticMeshComponent* CeilingHISM;

	// --- API ---

	/**
	 * Generates the dungeon visuals based on the Grid and Theme.
	 * @param Grid The dungeon layout.
	 * @param Config The generation configuration (includes Merging options).
	 * @param Theme The visual theme to apply.
	 * @param ChunkStreamer Optional streamer to update with new map data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	void GenerateDungeon(const FDungeonGrid& Grid, const FDungeonGenConfig& Config, const UDungeonThemeAsset* Theme, UDungeonChunkStreamer* ChunkStreamer = nullptr);

	/**
	 * Clears all generated meshes and data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	void ClearDungeon();

	/**
	 * Manually rebuilds the internal Chunk Maps from existing components.
	 * Useful after Load or PIE duplication.
	 */
	void RebuildChunkMaps();

	/**
	 * Force collision updates for PIE.
	 * Call this during PostLoad in the Actor if appropriate.
	 */
	void ForceUpdateCollision();

    /**
     * Handles PostLoad logic (PIE Fixups, Zombie Cleanup).
     * Call this from Actor's PostLoad.
     * @param bIsPIE True if loading in PIE.
     */
    void HandlePostLoad(bool bIsPIE);

	/**
	 * Manually cache grid (e.g. before Landscape generation).
	 * Useful to ensure GetCachedGrid() returns valid data for tools.
	 */
	void CacheGrid(const FDungeonGrid& Grid) { CachedGrid = Grid; }

	/**
	 * Access the last generated Grid data. Used by PCG Data Readers.
	 */
	const FDungeonGrid* GetCachedGrid() const { return &CachedGrid; }

	/**
	 * Access the last used Theme. Used by PCG Data Readers for TileSize, etc.
	 */
	const UDungeonThemeAsset* GetCachedTheme() const { return CachedTheme; }

	/**
	 * Access the cached Landscape world size. Set by DungeonLandscapeTool.
	 * Returns (0,0) if no Landscape was generated.
	 * Note: This is a STATIC value shared across all instances.
	 */
	static FVector2D GetCachedLandscapeWorldSize() { return CachedLandscapeWorldSize; }

	/**
	 * Set the cached Landscape world size. Called by DungeonLandscapeTool after Landscape generation.
	 * Note: This is a STATIC value shared across all instances.
	 */
	static void SetCachedLandscapeWorldSize(const FVector2D& WorldSize) { CachedLandscapeWorldSize = WorldSize; }

	/**
	 * Thread-safe static accessor for PCG nodes.
	 * Returns the last active DungeonRendererComponent (set during GenerateDungeon).
	 * This avoids needing TActorIterator which is not thread-safe.
	 */
	UFUNCTION(BlueprintPure, Category = "Dungeon")
	static UDungeonRendererComponent* GetLastActiveRenderer() { return LastActiveRenderer; }

	/** Sets the last active renderer. Called during GenerateDungeon(). */
	static void SetLastActiveRenderer(UDungeonRendererComponent* Renderer) { LastActiveRenderer = Renderer; }

private:
	/** Static cached reference for thread-safe access from PCG nodes */
	static UDungeonRendererComponent* LastActiveRenderer;

protected:
	/** Central storage of the grid data for sub-renderers and PCG readers */
	FDungeonGrid CachedGrid;

	/** Central storage of the theme for PCG readers */
	UPROPERTY()
	TObjectPtr<const UDungeonThemeAsset> CachedTheme;

	/** Cached Landscape world size for coordinate calculation in PCG nodes (STATIC - shared across all instances) */
	static FVector2D CachedLandscapeWorldSize;

	/** Specialized Renderer for PCG logic */
	UPROPERTY(Transient)
	TObjectPtr<class UDungeonPCGRenderer> PCGRenderer;

private:
	// Helper to parse component names into chunk coords (Legacy support)
	// TODO: Use Component Tags in future
	bool ParseChunkFromComponent(UActorComponent* Comp, FIntPoint& OutChunk, uint8& OutMask);
};
