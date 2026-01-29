#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonTileMarker.generated.h"

class ADungeonWorldBuilder;

/**
 * Debug marker actor that displays tile coordinates based on its position.
 * Place this actor in the editor to see Grid coordinates at that location.
 * Works in editor only (uses OnConstruction).
 */
UCLASS(Blueprintable)
class DUNGEONGENERATOR_API ADungeonTileMarker : public AActor
{
	GENERATED_BODY()

public:
	ADungeonTileMarker();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif

	/** Found Grid X coordinate */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Info")
	int32 GridX = -1;

	/** Found Grid Y coordinate */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Info")
	int32 GridY = -1;

	/** Tile type at this location */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Info")
	FString TileType = TEXT("Unknown");

	/** Walkable neighbors (N/S/E/W) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Info")
	FString WalkableNeighbors = TEXT("");

	/** WalkableCount */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Info")
	int32 WalkableCount = 0;

	/** Reference to found DungeonWorldBuilder */
	UPROPERTY(VisibleAnywhere, Category = "Tile Info")
	TWeakObjectPtr<ADungeonWorldBuilder> FoundBuilder;

protected:
	/** Updates tile info based on current position */
	void UpdateTileInfo();

	/** Billboard component for visibility */
	UPROPERTY(VisibleAnywhere)
	class UBillboardComponent* BillboardComponent;

	/** Text render component to show coordinates */
	UPROPERTY(VisibleAnywhere)
	class UTextRenderComponent* TextComponent;
};
