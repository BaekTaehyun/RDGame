#pragma once

#include "CoreMinimal.h"
#include "DungeonGeneratorSubsystem.h"
#include "Rendering/DungeonTileRenderer.h"
#include "Rendering/DungeonNavigationBuilder.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DungeonGeneratorTestActor.generated.h"

UCLASS()
class DUNGEONGENERATOR_API ADungeonGeneratorTestActor : public AActor {
    GENERATED_BODY()

public:
    ADungeonGeneratorTestActor();

    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
    int32 Seed = 12345;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
    int32 Width = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
    int32 Height = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
    EDungeonAlgorithmType Algorithm = EDungeonAlgorithmType::BSP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
    float TileSize = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
    FVector WallPivotOffset = FVector(0.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
    FVector FloorPivotOffset = FVector(0.0f, 0.0f, 0.0f);

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UInstancedStaticMeshComponent* WallMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UInstancedStaticMeshComponent* FloorMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UInstancedStaticMeshComponent* PropMesh;

    UFUNCTION(CallInEditor, Category = "Dungeon")
    void Generate();

    UFUNCTION(CallInEditor, Category = "Dungeon")
    void Clear();
};
