#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Voxel.h"
#include "VoxelRenderMode.h"
#include "WorldChunk.generated.h"

class UProceduralMeshComponent;
class AWorldManager;

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

    void GenerateMesh();
    void GenerateVoxels();

    FIntPoint GetChunkCoords() const { return ChunkCoords; }
    int GetChunkSizeXY() const { return ChunkSizeXY; }
    int GetChunkHeightZ() const { return ChunkHeightZ; }
    float GetVoxelScale() const { return VoxelScale; }
    void SetRenderMode(EVoxelRenderMode NewRenderMode) { RenderMode = NewRenderMode; }

    bool isInitialized = false;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    UProceduralMeshComponent* Mesh;

    AWorldManager* WorldManager = nullptr;

    FIntPoint ChunkCoords;

    UPROPERTY(EditAnywhere, Category = "Chunk")
    int32 ChunkSizeXY = 32;

    UPROPERTY(EditAnywhere, Category = "Chunk")
    int32 ChunkHeightZ = 32;

    UPROPERTY(EditAnywhere, Category = "Debug")
    UMaterialInterface* BiomeDebugMaterial;

    float VoxelScale = 100.0f;

    TArray<FVoxel> VoxelData;

    UPROPERTY()
    EVoxelRenderMode RenderMode;

    void AddCubeFace(int FaceIndex, FVector& Position, FColor FaceColor, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& VertexColors);

    bool ShouldCullBottomFace(int X, int Y, int Z) const;

    int LocalIndex(int X, int Y, int Z) const;

    void GenerateCubicMesh();
    void GenerateMarchingCubesMesh();

    float SampleDensityAtGlobalVoxel(int GlobalX, int GlobalY, int GlobalZ) const;

    FVector ComputeGradient(float GX, float GY, float GZ) const;

    FVector VertexInterp(float IsoLevel, const FVector& P1, const FVector& P2, float ValP1, float ValP2) const;
};
