// Fill out your copyright notice in the Description page of Project Settings.

#include "WorldGenerator.h"
#include "WorldChunk.h"
#include "Engine/World.h"

// Sets default values
AWorldGenerator::AWorldGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AWorldGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (!ChunkClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ChunkClass is not set in WorldGenerator"));
		return;
	}

	const float ChunkWorldSize = ChunkSizeXY * VoxelScale;

	for (int x = 0; x < WorldSizeInChunks; x++)
	{
		for (int y = 0; y < WorldSizeInChunks; y++)
		{
			FVector Position = FVector(x * ChunkWorldSize, y * ChunkWorldSize, 0.0f);

			AWorldChunk *NewChunk = GetWorld()->SpawnActor<AWorldChunk>(ChunkClass, Position, FRotator::ZeroRotator);

			if (NewChunk)
			{
				NewChunk->InitializeChunk(ChunkSizeXY, ChunkHeightZ, VoxelScale, FIntPoint(x, y));
			}
		}
	}
}
