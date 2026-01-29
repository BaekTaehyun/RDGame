#include "Debug/DungeonTileMarker.h"
#include "DungeonWorldBuilder.h"
#include "Components/DungeonRendererComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Data/DungeonThemeAsset.h"

ADungeonTileMarker::ADungeonTileMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Billboard for visibility in editor
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	BillboardComponent->SetupAttachment(RootComponent);
	BillboardComponent->SetHiddenInGame(true);

	// Text component to display coordinates
	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	TextComponent->SetupAttachment(RootComponent);
	TextComponent->SetRelativeLocation(FVector(0, 0, 100));
	TextComponent->SetHorizontalAlignment(EHTA_Center);
	TextComponent->SetVerticalAlignment(EVRTA_TextCenter);
	TextComponent->SetWorldSize(50.0f);
	TextComponent->SetTextRenderColor(FColor::Yellow);
	TextComponent->SetHiddenInGame(true);
}

void ADungeonTileMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateTileInfo();
}

#if WITH_EDITOR
void ADungeonTileMarker::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	// Update on every move, not just when finished
	UpdateTileInfo();
}
#endif

void ADungeonTileMarker::UpdateTileInfo()
{
	// Find DungeonWorldBuilder in the level
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADungeonWorldBuilder::StaticClass(), FoundActors);

	if (FoundActors.Num() == 0)
	{
		GridX = -1;
		GridY = -1;
		TileType = TEXT("No Builder Found");
		WalkableNeighbors = TEXT("");
		WalkableCount = 0;
		if (TextComponent)
		{
			TextComponent->SetText(FText::FromString(TEXT("No DungeonWorldBuilder")));
		}
		return;
	}

	ADungeonWorldBuilder* Builder = Cast<ADungeonWorldBuilder>(FoundActors[0]);
	FoundBuilder = Builder;

	if (!Builder)
	{
		return;
	}

	// Get TileSize from Theme
	float TileSize = 300.0f; // Default
	if (Builder->DungeonTheme)
	{
		TileSize = Builder->DungeonTheme->TileSize;
	}

	// Calculate Grid coordinates
	FVector MyLocation = GetActorLocation();
	FVector BuilderLocation = Builder->GetActorLocation();

	GridX = FMath::FloorToInt((MyLocation.X - BuilderLocation.X) / TileSize);
	GridY = FMath::FloorToInt((MyLocation.Y - BuilderLocation.Y) / TileSize);

	// Get Grid from DungeonRendererComponent
	UDungeonRendererComponent* Renderer = Builder->FindComponentByClass<UDungeonRendererComponent>();
	if (Renderer)
	{
		const FDungeonGrid* Grid = Renderer->GetCachedGrid();
		if (Grid && Grid->Width > 0 && Grid->Height > 0 && Grid->IsValid(GridX, GridY))
		{
			const FDungeonTile& Tile = Grid->GetTile(GridX, GridY);

			// Get tile type name
			switch (Tile.Type)
			{
			case ETileType::None: TileType = TEXT("None"); break;
			case ETileType::Floor: TileType = TEXT("Floor"); break;
			case ETileType::Wall: TileType = TEXT("Wall"); break;
			case ETileType::Corridor: TileType = TEXT("Corridor"); break;
			case ETileType::Door: TileType = TEXT("Door"); break;
			default: TileType = TEXT("Unknown"); break;
			}

			// Check walkable neighbors
			auto CheckWalkable = [Grid](int32 tx, int32 ty) -> bool {
				if (!Grid->IsValid(tx, ty)) return false;
				ETileType t = Grid->GetTile(tx, ty).Type;
				return t == ETileType::Floor || t == ETileType::Corridor || t == ETileType::Door;
			};

			bool bN = CheckWalkable(GridX, GridY + 1);
			bool bS = CheckWalkable(GridX, GridY - 1);
			bool bE = CheckWalkable(GridX + 1, GridY);
			bool bW = CheckWalkable(GridX - 1, GridY);

			WalkableNeighbors = FString::Printf(TEXT("N:%d S:%d E:%d W:%d"), bN, bS, bE, bW);
			WalkableCount = (bN ? 1 : 0) + (bS ? 1 : 0) + (bE ? 1 : 0) + (bW ? 1 : 0);
		}
		else
		{
			// Show more debug info
			if (!Grid || Grid->Width == 0)
			{
				TileType = TEXT("Grid Empty");
			}
			else
			{
				TileType = FString::Printf(TEXT("OOB (Grid:%dx%d)"), Grid->Width, Grid->Height);
			}
			WalkableNeighbors = TEXT("");
			WalkableCount = 0;
		}
	}
	else
	{
		TileType = TEXT("No Renderer");
		WalkableNeighbors = TEXT("");
		WalkableCount = 0;
	}

	// Update text display
	if (TextComponent)
	{
		FString DisplayText = FString::Printf(TEXT("(%d, %d)\n%s\n%s\nWC:%d"),
			GridX, GridY, *TileType, *WalkableNeighbors, WalkableCount);
		TextComponent->SetText(FText::FromString(DisplayText));
	}
}
