#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Voxel.h"
#include "VoxelRenderMode.h"
#include "WorldChunk.generated.h"

class UProceduralMeshComponent;
class AWorldManager;

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

    FIntPoint GetChunkCoords() const { return ChunkCoords; }
    int GetChunkSizeXY() const { return ChunkSizeXY; }
    int GetChunkHeightZ() const { return ChunkHeightZ; }
    float GetVoxelScale() const { return VoxelScale; }
    void SetRenderMode(EVoxelRenderMode NewRenderMode) { RenderMode = NewRenderMode; }
	bool AreVoxelsGenerated() const { return VoxelsGenerated; }
	float GetVoxelDensity(const FIntVector& LocalXYZ) const;
	int GetCurrentLODLevel() const { return CurrentLODLevel; }
    void SetCurrentLODLevel(int NewLODLevel) { CurrentLODLevel = NewLODLevel; }

    bool isInitialized = false;
    bool isQueuedForVoxelGen = false;
    bool useProceduralDensityOnly = false;
	bool isLOD0Built = false;
	bool isLOD0SeamDirty = false;

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

	// Rendering mode for this chunk
    UPROPERTY()
    EVoxelRenderMode RenderMode;

    void AddCubeFace(int FaceIndex, FVector& Position, float CubeSize, FColor FaceColor, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& VertexColors);

    bool ShouldCullBottomFace(int X, int Y, int Z) const;

    int LocalIndex(int X, int Y, int Z) const;

    bool GenerateCubicMesh();
    bool GenerateMarchingCubesMesh();

    float SampleDensityForMarching(int GlobalX, int GlobalY, int GlobalZ) const;

    bool VoxelsGenerated = false;

    void ComputeGradient(TArray<FVector>& GradientCache);

    FVector VertexInterp(float IsoLevel, const FVector& P1, const FVector& P2, float ValP1, float ValP2) const;
};
