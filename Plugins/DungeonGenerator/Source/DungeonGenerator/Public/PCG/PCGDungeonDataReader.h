#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "Data/DungeonConfig.h" // For EDungeonAlgorithmType if needed? No, CoreTypes for ETileType
#include "CoreTypes.h"
#include "PCGDungeonDataReader.generated.h"

// Define a simplified enum for the PCG Node if CoreTypes is too broad,
// but for now let's try to specific Filter Enum.
UENUM(BlueprintType)
enum class EPCGDungeonTileFilter : uint8
{
	All = 0,
	Floor,
	Wall,
	EdgeWall,     // Wall tiles adjacent to walkable tiles (1 direction)
	CornerWall,   // Wall tiles with L-shaped walkable adjacency (corners)
	ThroughWall,  // Wall tiles with walkable on both sides (N+S or E+W)
	Corridor,
	Door
};

UCLASS(BlueprintType, ClassGroup = (PCG), meta=(DisplayName="Dungeon Data Reader", Keywords="dungeon reader grid tile"))
class DUNGEONGENERATOR_API UPCGDungeonDataReaderSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGDungeonDataReaderSettings();

	//~Begin UPCGSettings Interface
	virtual FString GetAdditionalTitleInformation() const override;
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("DungeonDataReader")); } 
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCG", "DungeonDataReaderTitle", "Dungeon Data Reader"); }
	// Enable seed usage
	virtual bool UseSeed() const override { return true; }
#if WITH_EDITOR
	virtual FText GetNodeTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void ApplyDeprecation(UPCGNode* InNode) override;
#endif
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings Interface

	// Disable caching to ensure we always read the latest Grid data from the Actor
	virtual bool IsCacheable() const override { return false; }

public:
	/** Which dungeon tile type to sample points for */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	EPCGDungeonTileFilter TargetTileType = EPCGDungeonTileFilter::Floor;
};

class FPCGDungeonDataReaderElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
