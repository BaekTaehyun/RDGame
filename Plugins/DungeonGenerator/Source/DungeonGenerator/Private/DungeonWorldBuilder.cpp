#include "DungeonWorldBuilder.h"
#include "Components/DungeonRendererComponent.h"
#include "Components/BoxComponent.h"
#include "Algorithms/BSPGenerator.h"
#include "Algorithms/CellularAutomataGenerator.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Landscape.h"


// Define the static delegate
FOnRequestDungeonLandscape ADungeonWorldBuilder::OnRequestLandscape;
FOnRequestPaintPath ADungeonWorldBuilder::OnRequestPaintPath;

// Including Theme Asset for soft pointer loading
#include "Data/DungeonThemeAsset.h"

ADungeonWorldBuilder::ADungeonWorldBuilder()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create Box Component as Root (provides bounds for PCG)
	BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsBox"));
	RootComponent = BoundsBox;
	BoundsBox->SetBoxExtent(FVector(2500.0f, 2500.0f, 200.0f)); // Default 50x50 grid * 100 tile size
	BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsBox->SetVisibility(false);

	// Create Core Component (Logic Only)
	DungeonRenderer = CreateDefaultSubobject<UDungeonRendererComponent>(TEXT("DungeonRenderer"));

	// Init Default Config
	GeneratorConfig.Seed = FMath::Rand();
	GeneratorConfig.Algorithm = EDungeonAlgorithmType::BSP;
	GeneratorConfig.RenderMode = EDungeonRenderMode::PCG; // Default to PCG for WorldBuilder
}

void ADungeonWorldBuilder::BeginPlay()
{
	Super::BeginPlay();

	// Load Config from Table if set
	if (ConfigTable.DataTable && !ConfigTable.RowName.IsNone())
	{
		static const FString ContextString(TEXT("DungeonWorldBuilder_BeginPlay"));
		const FDungeonGenConfig* Row = ConfigTable.DataTable->FindRow<FDungeonGenConfig>(ConfigTable.RowName, ContextString);
		if (Row)
		{
			GeneratorConfig = *Row;
			if(Row->Theme.LoadSynchronous())
			{
				DungeonTheme = Row->Theme.Get();
			}
		}
	}
}

#if WITH_EDITOR
void ADungeonWorldBuilder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Auto-generate if SeedOverride related properties change
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeonWorldBuilder, SeedOverride) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ADungeonWorldBuilder, bUseSeedOverride))
	{
		Generate();
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeonWorldBuilder, ConfigTable))
	{
		if (ConfigTable.DataTable && !ConfigTable.RowName.IsNone())
		{
			static const FString ContextString(TEXT("DungeonWorldBuilder_PostEditChange"));
			const FDungeonGenConfig* Row = ConfigTable.DataTable->FindRow<FDungeonGenConfig>(ConfigTable.RowName, ContextString);
			if (Row)
			{
				GeneratorConfig = *Row;
				if(Row->Theme.LoadSynchronous())
				{
					DungeonTheme = Row->Theme.Get();
				}
			}
		}
	}
}
#endif

void ADungeonWorldBuilder::Generate()
{
	// Reload Config from DataTable before each generation
	if (ConfigTable.DataTable && !ConfigTable.RowName.IsNone())
	{
		static const FString ContextString(TEXT("DungeonWorldBuilder_Generate"));
		const FDungeonGenConfig* Row = ConfigTable.DataTable->FindRow<FDungeonGenConfig>(ConfigTable.RowName, ContextString);
		if (Row)
		{
			GeneratorConfig = *Row;
			if(Row->Theme.LoadSynchronous())
			{
				DungeonTheme = Row->Theme.Get();
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Input Check] bUseSeedOverride: %s, SeedOverride: %d, ConfigSeed: %d"), 
		bUseSeedOverride ? TEXT("TRUE") : TEXT("FALSE"), SeedOverride, GeneratorConfig.Seed);

	// Apply Seed Override
	if (bUseSeedOverride)
	{
		GeneratorConfig.Seed = SeedOverride;
		UE_LOG(LogTemp, Warning, TEXT("[Applied] New Seed: %d"), GeneratorConfig.Seed);
	}

	UE_LOG(LogTemp, Warning, TEXT("ADungeonWorldBuilder::Generate() - Config: Width=%d, Height=%d, Seed=%d, Algorithm=%d"),
		GeneratorConfig.Width, GeneratorConfig.Height, GeneratorConfig.Seed, (int)GeneratorConfig.Algorithm);
	
	if (DungeonTheme)
	{
		UE_LOG(LogTemp, Warning, TEXT("ADungeonWorldBuilder::Generate() - Theme: TileSize=%.1f, WallPivotOffset=%s, ThroughWallPivotOffset=%s"),
			DungeonTheme->TileSize, *DungeonTheme->WallPivotOffset.ToString(), *DungeonTheme->ThroughWallPivotOffset.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ADungeonWorldBuilder::Generate() - DungeonTheme is NULL!"));
	}

	// 1. Prepare Algorithm
	UDungeonAlgorithm* Algo = nullptr;
	switch (GeneratorConfig.Algorithm)
	{
	case EDungeonAlgorithmType::BSP:
		{
			UBSPGenerator* BSPAlgo = NewObject<UBSPGenerator>(this);
			BSPAlgo->CorridorWidth = GeneratorConfig.CorridorWidth;
			BSPAlgo->SplitRatio = 0.5f; // Add missing property or hardcode
			BSPAlgo->MinRoomSize = GeneratorConfig.MinRoomSize;
			Algo = BSPAlgo;
		}
		break;
	case EDungeonAlgorithmType::CellularAutomata:
		Algo = NewObject<UCellularAutomataGenerator>(this);
		break;
	}

	if (!Algo)
	{
		UE_LOG(LogTemp, Error, TEXT("ADungeonWorldBuilder: Algorithm not selected or failed to create."));
		return;
	}

	// 2. Generate Grid
	FDungeonGrid Grid;
	Grid.Init(GeneratorConfig.Width, GeneratorConfig.Height, ETileType::Wall);
	
	FRandomStream RandomStream(GeneratorConfig.Seed);
	Algo->Generate(Grid, RandomStream);

	// Update BoundsBox to match dungeon size (critical for PCG bounds validation)
	// 3. Update BoundsBox to match dungeon size (critical for PCG bounds validation)
	if (BoundsBox && DungeonTheme)
	{
		const float TileSize = DungeonTheme->TileSize;
		const FVector DungeonExtent(
			GeneratorConfig.Width * TileSize * 0.5f,
			GeneratorConfig.Height * TileSize * 0.5f,
			5000.0f); // Increased Z-height to ensure PCG Volume covers Landscape fluctuations
		BoundsBox->SetBoxExtent(DungeonExtent);
		// Position box so that grid (0,0) is at actor origin
		// Box center should be at half the dungeon size
		BoundsBox->SetRelativeLocation(FVector(DungeonExtent.X, DungeonExtent.Y, 0.0f));
		
		UE_LOG(LogTemp, Log, TEXT("DungeonWorldBuilder: TileSize=%.1f, BoundsBox Extent=%s, Location=%s"), 
			TileSize, *DungeonExtent.ToString(), *BoundsBox->GetRelativeLocation().ToString());
	}

#if WITH_EDITOR
	// 2.5. Cache Grid for Landscape Tool
	if (DungeonRenderer)
	{
		DungeonRenderer->CacheGrid(Grid);
	}
	
	// 4. Auto-Generate Landscape (Required for Nature PCG)
	// Grid is now cached in Renderer, so Tool can retrieve it.
	if (OnRequestLandscape.IsBound())
	{
		OnRequestLandscape.Broadcast(this, &Grid);
	}
#endif

	// 3. Render
	if (DungeonRenderer)
	{
		DungeonRenderer->GenerateDungeon(Grid, GeneratorConfig, DungeonTheme, nullptr /* No ChunkStreamer for now */);
	}
}

void ADungeonWorldBuilder::Clear()
{
	if (DungeonRenderer)
	{
		DungeonRenderer->ClearDungeon();
	}
	
	// Destroy spawned Landscape
	// Destroy spawned Landscape
	if (SpawnedLandscape.IsValid())
	{
		SpawnedLandscape->Destroy();
		SpawnedLandscape.Reset();
	}

	// 2. Destroy any "DungeonGeneratedLandscape" found in the world (Robust cleanup)
	// Also check by Name "DungeonLandscape" for backward compatibility
	UWorld* World = GetWorld();
	if (World)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(World, ALandscape::StaticClass(), FoundActors);
		
		for (AActor* Actor : FoundActors)
		{
			if (Actor && !Actor->IsPendingKillPending())
			{
				bool bMatchTag = Actor->Tags.Contains(FName("DungeonGeneratedLandscape"));
				bool bMatchName = Actor->GetName().StartsWith(TEXT("DungeonLandscape"));
				
				if (bMatchTag || bMatchName)
				{
					Actor->Destroy();
					UE_LOG(LogTemp, Log, TEXT("DungeonWorldBuilder: Cleaned up landscape '%s'"), *Actor->GetName());
				}
			}
		}
	}
	
	// Cleanup PCG generated actors (they have PCG tag)
	// World variable is already defined above
	if (World)
	{
		TArray<AActor*> ActorsToDestroy;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Tags.Contains(FName("PCG Generated Actor")))
			{
				ActorsToDestroy.Add(Actor);
			}
		}
		
		for (AActor* Actor : ActorsToDestroy)
		{
			Actor->Destroy();
		}
		
		if (ActorsToDestroy.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("ADungeonWorldBuilder: Destroyed %d PCG actors."), ActorsToDestroy.Num());
		}
	}
}

void ADungeonWorldBuilder::GenerateLandscape()
{
#if WITH_EDITOR
	// Delegate only supports Actor param usually.
	// If we want to support passing grid via delegate, we need to change delegate type.
	// OLD: OnRequestLandscape.Broadcast(this);
	// We can't change delegate signature easily if it is MulticastDelegate(ADungeonWorldBuilder*)
	
	// BUT, GenerateLandscape calling chain starts from Generate() usually. 
	// If called manually (via button), we don't have a grid.
	// In that case, Tool will try to get CachedGrid (which will be valid if Generate ran before).
	
	if (OnRequestLandscape.IsBound())
	{
		// Manual call usually implies no grid passed, pass nullptr and let Tool use Cache/Config
		OnRequestLandscape.Broadcast(this, nullptr);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ADungeonWorldBuilder: Landscape generation tool is not bound. Ensure DungeonGeneratorEditor module is loaded."));
	}
#endif
}

void ADungeonWorldBuilder::PaintDungeonPaths()
{
#if WITH_EDITOR
	if (OnRequestPaintPath.IsBound())
	{
		OnRequestPaintPath.Broadcast(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ADungeonWorldBuilder: Paint Path tool is not bound. Ensure DungeonGeneratorEditor module is loaded."));
	}
#endif
}
