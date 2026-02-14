#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Voxel.h"
#include "VoxelRenderMode.h"
#include "TerrainGenerator.h"
#include "WorldChunk.generated.h"

class UProceduralMeshComponent;
class AWorldManager;

struct FChunkMeshBuffers
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> VertexColors;
};

struct FChunkSnapshot
{
    FIntPoint ChunkCoords = FIntPoint::ZeroValue;
	int32 ChunkSizeXY = 0;
	int32 ChunkHeightZ = 0;
	float VoxelScale = 100.0f;
	int32 CurrentLODStep = 1;

	TArray<FVoxel> VoxelDataCopy;
	bool hasVoxelData = false;

	bool isNeighborLoadedPosX = false;
	bool isNeighborLoadedNegX = false;
	bool isNeighborLoadedPosY = false;
	bool isNeighborLoadedNegY = false;
	int32 NeighborLODPosX = 0;
	int32 NeighborLODNegX = 0;
	int32 NeighborLODPosY = 0;
	int32 NeighborLODNegY = 0;

    TArray<float> DensityGrid;
	int32 SampleOriginX = 0;
	int32 SampleOriginY = 0;
	int32 SampleSizeX = 0;
	int32 SampleSizeY = 0;

    TArray<uint8> BiomeGrid;

    FORCEINLINE float GetSampledDensity(int32 GlobalX, int32 GlobalY, int32 GlobalZ) const
    {
		const int32 gx = GlobalX - SampleOriginX;
		const int32 gy = GlobalY - SampleOriginY;
        if (gx < 0 || gy < 0 || gx >= SampleSizeX || gy >= SampleSizeY) return 1.0f;
		if (GlobalZ < 0 || GlobalZ >= ChunkHeightZ) return (GlobalZ < 0) ? 1.0f : -1.0f;
		const int32 Index = gx + gy * SampleSizeX + GlobalZ * SampleSizeX * SampleSizeY;
		return DensityGrid.IsValidIndex(Index) ? DensityGrid[Index] : 1.0f;
	}

    FORCEINLINE bool TryGetVoxelDensityFromCopy(int32 GlobalX, int32 GlobalY, int32 GlobalZ, float& OutDensity) const
    {
		if (!hasVoxelData || GlobalZ < 0 || GlobalZ>= ChunkHeightZ) return false;

		const int32 LocalX = GlobalX - ChunkCoords.X * ChunkSizeXY;
		const int32 LocalY = GlobalY - ChunkCoords.Y * ChunkSizeXY;

		if (LocalX < 0 || LocalY < 0 || LocalX >= ChunkSizeXY || LocalY >= ChunkSizeXY) return false;

		const int32 LocalIndex = LocalX + LocalY * ChunkSizeXY + GlobalZ * ChunkSizeXY * ChunkSizeXY;
		if (!VoxelDataCopy.IsValidIndex(LocalIndex)) return false;

		OutDensity = VoxelDataCopy[LocalIndex].density;
		return true;
    }

    FORCEINLINE EBiomeType GetSampledBiome(int32 GlobalX, int32 GlobalY) const
    {
		const int32 gx = GlobalX - SampleOriginX;
        const int32 gy = GlobalY - SampleOriginY;
		if (gx < 0 || gy < 0 || gx >= SampleSizeX || gy >= SampleSizeY) return EBiomeType::Plains;

		const int32 Index = gx + gy * SampleSizeX;
		return BiomeGrid.IsValidIndex(Index) ? static_cast<EBiomeType>(BiomeGrid[Index]) : EBiomeType::Plains;
    }
};

enum class EChunkGenPhase : uint8
{
    None,
    Voxels,
    MeshLOD0
};

UCLASS()
class PROCEDURALSURVIVAL_API AWorldChunk : public AActor
{
    GENERATED_BODY()

public:
    AWorldChunk();

    void InitializeChunk(int InChunkSizeXY, int InChunkHeightZ, float InVoxelScale, const FIntPoint& InChunkCoords);

    bool IsVoxelSolidLocal(int LocalX, int LocalY, int LocalZ) const;

    void SetVoxelLocal(int LocalX, int LocalY, int LocalZ, bool isSolid);

    void SetWorldManager(AWorldManager* InWorldManager) { WorldManager = InWorldManager; }

    void GenerateVoxels();

	bool GenerateMeshLOD(int32 LODLevel);

    bool BuildMeshLODData(int32 LODLevel, FChunkMeshBuffers& OutBuffers, const FChunkSnapshot* Snapshot = nullptr);

    void ApplyMeshData(const FChunkMeshBuffers& Buffers);

    void ApplyGeneratedVoxels(TArray<FVoxel>&& InVoxels);

    void AllocateVoxelData();
    void ReleaseVoxelData();
	void MaybeReleaseVoxelData();

    FIntPoint GetChunkCoords() const { return ChunkCoords; }
    int GetChunkSizeXY() const { return ChunkSizeXY; }
    int GetChunkHeightZ() const { return ChunkHeightZ; }
    float GetVoxelScale() const { return VoxelScale; }
    void SetRenderMode(EVoxelRenderMode NewRenderMode) { RenderMode = NewRenderMode; }
	bool AreVoxelsGenerated() const { return VoxelsGenerated; }
	float GetVoxelDensity(const FIntVector& LocalXYZ) const;
	int GetCurrentLODLevel() const { return CurrentLODLevel; }
    void SetCurrentLODLevel(int NewLODLevel) { CurrentLODLevel = NewLODLevel; }

    UFUNCTION(BlueprintCallable)
    AWorldManager* GetWorldManager() const { return WorldManager; }

    bool isInitialized = false;
    bool isQueuedForVoxelGen = false;
    bool useProceduralDensityOnly = false;
	bool isLOD0Built = false;
	bool isLOD0SeamDirty = false;
    bool isVoxelTaskInProgress = false;
    bool isMeshTaskInProgress = false;

	EChunkGenPhase CurrentGenPhase = EChunkGenPhase::None;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    UProceduralMeshComponent* Mesh;

    AWorldManager* WorldManager = nullptr;

    FIntPoint ChunkCoords;

	// Size of chunk in voxels along X and Y axis (Horizontal plane)
    UPROPERTY(EditAnywhere, Category = "Chunk")
    int32 ChunkSizeXY = 32;

	// Height of chunk in voxels along Z axis
    UPROPERTY(EditAnywhere, Category = "Chunk")
    int32 ChunkHeightZ = 32;

    UPROPERTY(EditAnywhere, Category = "Debug")
    UMaterialInterface* BiomeDebugMaterial;

	// Current LOD level of this chunk (-1 means not set)
    UPROPERTY()
	int32 CurrentLODLevel = 0;

	// Current LOD step for progressive LOD generation
    UPROPERTY()
    int32 CurrentLODStep = 0;

    UPROPERTY()
    bool isFinalMesh = false;

	// Size of each voxel in Unreal units (100 units = 1 meter)
    float VoxelScale = 100.0f;

    TArray<FVoxel> VoxelData;

    bool pendingVoxelRelease = false;

	// Rendering mode for this chunk
    UPROPERTY()
    EVoxelRenderMode RenderMode;

    void AddCubeFace(int FaceIndex, FVector& Position, float CubeSize, FColor FaceColor, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& VertexColors);

    bool ShouldCullBottomFace(int X, int Y, int Z) const;

    int LocalIndex(int X, int Y, int Z) const;

    bool BuildCubicMeshData(int32 LODLevel, int32 LODStep, bool ProceduralOnly, FChunkMeshBuffers& OutBuffers, const FChunkSnapshot* Snapshot = nullptr);
    bool BuildMarchingCubeData(int32 LODLevel, int32 LODStep, bool ProceduralOnly, FChunkMeshBuffers& OutBuffers, const FChunkSnapshot* Snapshot = nullptr);

    float SampleDensityForMarching(int GlobalX, int GlobalY, int GlobalZ, bool ProceduralOnly, const FChunkSnapshot* Snapshot = nullptr) const;

    bool VoxelsGenerated = false;

    void ComputeGradient(TArray<FVector>& GradientCache, const FChunkSnapshot* Snapshot = nullptr);

    FVector VertexInterp(float IsoLevel, const FVector& P1, const FVector& P2, float ValP1, float ValP2) const;
};
