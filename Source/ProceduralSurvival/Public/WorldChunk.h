#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Voxel.h"
#include "WorldChunk.generated.h"

class UProceduralMeshComponent;
class AWorldManager;

UCLASS()
class PROCEDURALSURVIVAL_API AWorldChunk : public AActor
{
    GENERATED_BODY()

public:
    AWorldChunk();

    void InitializeChunk(int InChunkSize, float InVoxelScale, const FIntPoint& InChunkCoords);

    bool IsVoxelSolidLocal(int LocalX, int LocalY, int LocalZ) const;

	void SetVoxelLocal(int LocalX, int LocalY, int LocalZ, bool isSolid);

	void SetWorldManager(AWorldManager* InWorldManager) { WorldManager = InWorldManager; }

    void GenerateMesh();

    FIntPoint GetChunkCoords() const { return ChunkCoords; }
    int GetChunkSize() const { return ChunkSize; }
	float GetVoxelScale() const { return VoxelScale; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    UProceduralMeshComponent *Mesh;

	AWorldManager* WorldManager = nullptr;

    FIntPoint ChunkCoords;

    int ChunkSize = 32;
    float VoxelScale = 100.0f;

    TArray<FVoxel> VoxelData;

    void GenerateVoxels();

    void AddCubeFace(int FaceIndex, FVector& Position, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs);

    int LocalIndex(int X, int Y, int Z) const;
};