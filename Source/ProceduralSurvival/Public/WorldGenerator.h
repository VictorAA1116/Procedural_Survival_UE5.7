// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldGenerator.generated.h"

class AWorldChunk;

UCLASS()
class PROCEDURALSURVIVAL_API AWorldGenerator : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWorldGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Chunk size in voxels
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Generation")
	int ChunkSizeXY = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation")
	int ChunkHeightZ = 32;

	// World size in chunks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Generation")
	int WorldSizeInChunks = 4;

	// Distance between chunk actors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Generation")
	float VoxelScale = 100.0f;

	// Reference to the WorldChunk class to spawn
	UPROPERTY(EditAnywhere, Category="World Generation")
	TSubclassOf<AWorldChunk> ChunkClass;
};
