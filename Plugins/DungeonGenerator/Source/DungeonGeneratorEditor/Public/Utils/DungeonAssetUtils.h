#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DungeonAssetUtils.generated.h"

/**
 * MCP Server Helper Library (Editor Module)
 * Exposes Editor-only functionality to Python
 */
UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonAssetUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Captures the thumbnail of an asset and returns it as a PNG byte array.
	 * Returns empty array if failed.
	 * Wrapper for ThumbnailTools::GenerateThumbnailForObjectToSaveToDisk
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon Generator|MCP")
	static TArray<uint8> CaptureThumbnail(FString AssetPath);

	/**
	 * Analyzes PCG Graph connection topology.
	 * Returns a JSON string mapping Node Names to their connected Downstream Nodes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon Generator|MCP")
	static FString AnalyzePCGTopology(FString GraphPath);

    /**
     * Connects two PCG nodes by Name/Label.
     * Returns true if successful.
     */
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generator|MCP")
    static bool ConnectPCGNodes(FString GraphPath, FString UpstreamNodeName, FString DownstreamNodeName, FString UpPinLabel = "Out", FString DownPinLabel = "In");

    /**
     * Marks the Blueprint as structurally modified to trigger an editor refresh.
     * Useful when modifying graphs or properties via Python/C++ to ensure the Editor UI syncs.
     * WARNING: Recompiles the Blueprint. Use sparingly.
     */
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generator|MCP")
    static void RefreshBlueprint(UObject* BlueprintAsset);
};
