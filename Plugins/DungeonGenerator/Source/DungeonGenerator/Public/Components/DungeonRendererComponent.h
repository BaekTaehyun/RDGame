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

private:
	// Helper to parse component names into chunk coords (Legacy support)
	// TODO: Use Component Tags in future
	bool ParseChunkFromComponent(UActorComponent* Comp, FIntPoint& OutChunk, uint8& OutMask);
};
