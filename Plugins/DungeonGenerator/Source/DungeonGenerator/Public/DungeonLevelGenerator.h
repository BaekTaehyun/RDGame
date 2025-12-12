#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Data/DungeonConfig.h"
#include "DungeonLevelGenerator.generated.h"

class UDungeonRendererComponent;
class UDungeonChunkStreamer;
class UDungeonThemeAsset;

/**
 * Production-ready Dungeon Generator Actor.
 * Driven by Data Table Configuration and Theme Assets.
 */
UCLASS()
class DUNGEONGENERATOR_API ADungeonLevelGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ADungeonLevelGenerator();

protected:
	virtual void BeginPlay() override;
	virtual void PostLoad() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	// --- Configuration ---

	/** Reference to the Dungeon Configuration in a Data Table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	FDataTableRowHandle DungeonConfigRow;

	/** Override Theme (Optional). If null, uses the Theme defined in the Config Row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	UDungeonThemeAsset* ThemeOverride;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDungeonRendererComponent* DungeonRenderer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDungeonChunkStreamer* ChunkStreamer;

	// --- API ---

	/**
	 * Main function to generate the dungeon.
	 * Loads Config from Table -> Builds Grid -> Renders.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Dungeon")
	void GenerateDungeon();

	/**
	 * Clears the dungeon.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Dungeon")
	void ClearDungeon();

private:
	// Helper to resolve Config from Row Handle
	bool GetDungeonConfig(FDungeonGenConfig& OutConfig);
};
