#pragma once

#include "CoreMinimal.h"
#include "WorldChunk.h"
#include "VoxelRenderMode.h"
#include "TerrainGenerator.h"
#include "GameFramework/Actor.h"
#include "WorldManager.generated.h"

UCLASS()
class PROCEDURALSURVIVAL_API AWorldManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWorldManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Query global voxel by global voxel coordinates (Voxel coordinates across the whole world)
	bool IsVoxelSolidGlobal(int GlobalVoxelX, int GlobalVoxelY, int GlobalVoxelZ) const;

	// Convert world-space position (cm) to global voxel coordinates (voxel indices)
	FIntVector WorldPosToGlobalVoxel(const FVector& WorldPos) const;

	// Convert global voxel coords to chunk coords and local voxel coords
	void GlobalVoxelToChunkCoords(int GlobalX, int GlobalY, int GlobalZ, FIntPoint& OutChunkXY, FIntVector& OutLocalXYZ) const;

	bool IsChunkWithinRenderDistance(const FIntPoint& ChunkXY) const;

	bool IsNeighborChunkLoaded(const FIntPoint& NChunkXY) const;

	bool AreAllNeighborChunksVoxelReady(const FIntPoint& ChunkXY) const;

	AWorldChunk* GetChunkAt(const FIntPoint& ChunkXY) const;

	int32 ComputeLODForChunk(const FIntPoint& ChunkXY) const;

	// Voxel rendering mode (Cubes or Marching Cubes)
	UPROPERTY(EditAnywhere, Category = "World Generation")
	EVoxelRenderMode RenderMode = EVoxelRenderMode::Cubes;


	// Terrain generator instance reference
	UPROPERTY(EditAnywhere, Instanced, Category = "Terrain")
	UTerrainGenerator* TerrainGenerator;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Size of chunks in voxels on the X and Y axis
	UPROPERTY(EditAnywhere, Category = "World Generation")
	int ChunkSizeXY = 32;

	// Height of chunks in voxels on the Z axis 
	UPROPERTY(EditAnywhere, Category = "World Generation")
	int ChunkHeightZ = 32;

	// Size of voxel in centimeters
	UPROPERTY(EditAnywhere, Category = "World Generation")
	float VoxelScale = 100.0f; // 1 meter per voxel

	// Load radius in chunks around the player in X and Y directions
	UPROPERTY(EditAnywhere, Category = "World Generation")
	int RenderDistance = 4;

	// Chunk actor class to spawn (set to BP_WorldChunk)
	UPROPERTY(EditAnywhere, Category = "World Generation")
	TSubclassOf<AWorldChunk> ChunkClass;

private:

	// Maximum allowed active chunks before unloading the farthest ones
	UPROPERTY(EditAnywhere, Category = "World Generation")
	int MaxAllowedChunks = 200;

	// Active chunk map keyed by chunk coordinates
	UPROPERTY()
	TMap<FIntPoint, AWorldChunk*> ActiveChunks;

	// Reference to the player pawn for chunk loading
	UPROPERTY()
	APawn* PlayerPawn = nullptr;

	TArray<FIntPoint> ChunkGenQueue;

	// Chunk generation rate in chunks per second (60 = 1 chunk per frame at 60 FPS)
	UPROPERTY(EditAnywhere, Category = "World Generation")
	float ChunkGenRate = 60.0f; // chunks per second

	float ChunkGenAccumulator = 0.0f;

	// Maximum LOD level (0 = highest detail, no LODs and lower performance)
	UPROPERTY(EditAnywhere, Category = "LOD")
	int32 MaxLODLevel = 4;

	// Render distance for LOD0 (highest detail) in chunks
	UPROPERTY(EditAnywhere, Category = "LOD")
	int32 LOD0RenderDistance = 6;

	// Multiplier for LOD distances (e.g., 2 means each higher LOD has double the distance of the previous)
	UPROPERTY(EditAnywhere, Category = "LOD")
	int32 LODStepMultiplier = 2;

	// Current center chunk coordinates based on player position
	FIntPoint CenterChunk = FIntPoint::ZeroValue;

	void UpdateChunks();
	void RegisterChunkAt(const FIntPoint& ChunkXY);
	void DestroyChunkAt(const FIntPoint& ChunkXY);
	void OnChunkCreated(const FIntPoint& ChunkXY);
	void SortChunkQueueByDistance();
};
