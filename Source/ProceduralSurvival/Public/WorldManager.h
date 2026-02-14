#pragma once

#include "CoreMinimal.h"
#include "WorldChunk.h"
#include "VoxelRenderMode.h"
#include "TerrainGenerator.h"
#include "GameFramework/Actor.h"
#include <atomic>
#include "WorldManager.generated.h"

UCLASS()
class PROCEDURALSURVIVAL_API AWorldManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWorldManager();

	virtual bool ShouldTickIfViewportsOnly() const override;

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

	void EnqueueInitialLODs();

	void EnqueueLODMeshBuild(const FIntPoint& ChunkXY, int32 LOD);

	UFUNCTION(BlueprintCallable)
	bool RemoveVoxel(const FVector& VoxelLocation);

	UFUNCTION(BlueprintCallable)
	bool AddVoxel(const FVector& VoxelLocation);

	// Voxel rendering mode (Cubes or Marching Cubes)
	UPROPERTY(EditAnywhere, Category = "World Generation")
	EVoxelRenderMode RenderMode = EVoxelRenderMode::Cubes;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "World Generation | Editor View")
	bool bAutoGenerateInEditor = false;
#endif

	// Terrain generator instance reference
	UPROPERTY(EditAnywhere, Instanced, Category = "Terrain")
	UTerrainGenerator* TerrainGenerator;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "World Generation | Editor View")
	void GenerateWorldInEditor();

	UFUNCTION(CallInEditor, Category = "World Generation | Editor View")
	void ClearWorldInEditor();
#endif

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

	// Max num of async voxel generation tasks to keep memory usage stable
	UPROPERTY(EditAnywhere, Category = "World Generation")
	int32 MaxVoxelTasks = 2;

	std::atomic<int32> ActiveVoxelTasks = 0;

	// Active chunk map keyed by chunk coordinates
	UPROPERTY()
	TMap<FIntPoint, AWorldChunk*> ActiveChunks;

	// Reference to the player pawn for chunk loading
	UPROPERTY()
	APawn* PlayerPawn = nullptr;

	TArray<FIntPoint> ChunkGenQueue;
	TArray<FIntPoint> ChunkRegisterQueue;

	// Chunk generation rate in chunks per second (60 = 1 chunk per frame at 60 FPS)
	UPROPERTY(EditAnywhere, Category = "World Generation")
	float ChunkGenRate = 60.0f; // chunks per second

	float ChunkGenAccumulator = 0.0f;

	UPROPERTY(EditAnywhere, Category = "World Generation")
	float ChunkRegisterRate = 60.0f;

	float SpawnAccumulator = 0.0f;

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

	TArray<FIntPoint> LODQueue;
	TMap<FIntPoint, int32> PendingLOD;

	float LODBuildAccumulator = 0.0f;

	// LOD build rate in LOD builds per second (60 = 1 LOD build per frame at 60 FPS)
	UPROPERTY(EditAnywhere, Category = "LOD")
	float LODBuildRate = 30.0f; // LOD builds per second

	void UpdateChunks();
	void RegisterChunkAt(const FIntPoint& ChunkXY);
	void DestroyChunkAt(const FIntPoint& ChunkXY);
	void RegenerateChunk(const FIntPoint& Center, int32 OldLOD, int32 NewLOD);
	void SortChunkQueueByDistance();
	void SortLODQueueByDistance();
	void SortRegisterQueueByDistance();
	void MarkLOD0Dirty(const FIntPoint& ChunkXY);
	void MarkChunkAndNeighborsDirty(const FIntPoint& Center);
	void MarkLOD0NeighborSeamDirty(const FIntPoint& Center);

	void UpdateCenterChunk();
	void ProcessLODUpdates(TArray <FIntPoint> ActiveChunkKeys);
	void LOD0SafetyNet(TArray <FIntPoint> ActiveChunkKeys);
	void ProcessChunkGenQueue(float DeltaTime);
	void ProcessLODQueue(float DeltaTime);
	void ProcessChunkRegisterQueue(float DeltaTime);
	
	void ProcessVoxelPhase(AWorldChunk* Chunk, const FIntPoint& ChunkXY);
	void ProcessMeshLOD0Phase(AWorldChunk* Chunk, const FIntPoint& ChunkXY);
	void CatchUnqueuedChunks(AWorldChunk* Chunk, const FIntPoint& ChunkXY);

	void ResetGenerationState(bool DestroyChunkActors);
	void SetCenterChunkFromWorldLocation(const FVector& WorldLocation);

	bool HasPendingLOD0Work() const;

	void StartAsyncVoxelGen(AWorldChunk* Chunk, const FIntPoint& ChunkXY);
	void BuildChunkSnapshot(AWorldChunk* Chunk, const FIntPoint& ChunkXY, int32 LODLevel, FChunkSnapshot& OutSnapshot) const;
	void StartAsyncMeshBuild(AWorldChunk* Chunk, const FIntPoint& ChunkXY, int32 LODLevel, bool MarkNeighborsOnSuccess);
};
