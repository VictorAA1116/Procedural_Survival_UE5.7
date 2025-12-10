#include "WorldChunk.h"
#include "WorldManager.h"
#include "Engine/World.h"

AWorldChunk::AWorldChunk()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    RootComponent = Mesh;
}

void AWorldChunk::BeginPlay()
{
    Super::BeginPlay();
}

void AWorldChunk::InitializeChunk(int InChunkSize, float InVoxelScale, const FIntPoint& InChunkCoords)
{
    ChunkSize = FMath::Max(1, InChunkSize);
    VoxelScale = InVoxelScale;
	ChunkCoords = InChunkCoords;

	const int32 Total = ChunkSize * ChunkSize * ChunkSize;
	VoxelData.SetNumZeroed(Total);

    //GenerateVoxels();
    //GenerateMesh();
    isInitialized = true;
}

int AWorldChunk::LocalIndex(int X, int Y, int Z) const
{
    if (X < 0 || X >= ChunkSize || Y < 0 || Y >= ChunkSize || Z < 0 || Z >= ChunkSize)
    {
        return -1;
    }

    return X + Y * ChunkSize + Z * ChunkSize * ChunkSize;
}

bool AWorldChunk::IsVoxelSolidLocal(int LocalX, int LocalY, int LocalZ) const
{
    if (LocalX < 0 || LocalX >= ChunkSize || LocalY < 0 || LocalY >= ChunkSize || LocalZ < 0 || LocalZ >= ChunkSize) return false;

    if (!isInitialized) return false;

	int Index = LocalIndex(LocalX, LocalY, LocalZ);
	if (Index < 0) return false;

	return VoxelData[Index].isSolid;
}

void AWorldChunk::SetVoxelLocal(int LocalX, int LocalY, int LocalZ, bool isSolid)
{
    if (!isInitialized) return;

    int Index = LocalIndex(LocalX, LocalY, LocalZ);
    if (Index < 0) return;
    VoxelData[Index].isSolid = isSolid;

	GenerateMesh();
}

void AWorldChunk::GenerateVoxels()
{

    if (!isInitialized) return;

    const float NoiseScale = 0.08f;
	const float HeightMultiplier = 5.0f;
	const float BaseHeight = ChunkSize * 0.4f;
    for (int x = 0; x < ChunkSize; x++)
    {
        for (int y = 0; y < ChunkSize; y++)
        {
			float ChunkX = (ChunkCoords.X * ChunkSize + x) * NoiseScale;
			float ChunkY = (ChunkCoords.Y * ChunkSize + y) * NoiseScale;

            float Height = FMath::PerlinNoise2D(FVector2D(ChunkX, ChunkY)) * HeightMultiplier + BaseHeight;

            for (int z = 0; z < ChunkSize; z++)
            {
                int Index = LocalIndex(x, y, z);
				if (Index < 0) continue;

                FVoxel &Voxel = VoxelData[Index];
                Voxel.isSolid = (z <= FMath::FloorToInt(Height));
                Voxel.density = Voxel.isSolid ? 1.0f : -1.0f;
            }
        }
    }
}

void AWorldChunk::AddCubeFace(int FaceIndex, FVector& Position, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs)
{
    float S = VoxelScale;

    struct FCubeFace
    {
        FVector Normal;
        FVector Verts[4];
    };

    FCubeFace Faces[6] =
    {
        // Right
        { FVector(1,0,0), {
            Position + FVector(S,0,0),
            Position + FVector(S,0,S),
            Position + FVector(S,S,S),
            Position + FVector(S,S,0) } },

        // Left
        { FVector(-1,0,0), {
            Position + FVector(0,0,0),
            Position + FVector(0,S,0),
            Position + FVector(0,S,S),
            Position + FVector(0,0,S) } },

        // Front
        { FVector(0,1,0), {
            Position + FVector(0,S,0),
            Position + FVector(S,S,0),
            Position + FVector(S,S,S),
            Position + FVector(0,S,S) } },

        // Back
        { FVector(0,-1,0), {
            Position + FVector(0,0,0),
            Position + FVector(0,0,S),
            Position + FVector(S,0,S),
            Position + FVector(S,0,0) } },

        // Top
        { FVector(0,0,1), {
            Position + FVector(0,0,S),
            Position + FVector(0,S,S),
            Position + FVector(S,S,S),
            Position + FVector(S,0,S) } },

        // Bottom
        { FVector(0,0,-1), {
            Position + FVector(0,0,0),
            Position + FVector(S,0,0),
            Position + FVector(S,S,0),
            Position + FVector(0,S,0) } },
    };

    int Start = Vertices.Num();

    // Add vertices
    for (int i = 0; i < 4; ++i)
    {
        Vertices.Add(Faces[FaceIndex].Verts[i]);
        Normals.Add(Faces[FaceIndex].Normal);
        UVs.Add(FVector2D( (i == 1 || i == 2), (i == 2 || i == 3) ));
    }

    // Add triangles
    Triangles.Add(Start + 0);
    Triangles.Add(Start + 1);
    Triangles.Add(Start + 2);

    Triangles.Add(Start + 0);
    Triangles.Add(Start + 2);
    Triangles.Add(Start + 3);
}

void AWorldChunk::GenerateMesh()
{
    if (Mesh)
    {
        Mesh->ClearAllMeshSections();
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    const int EstimatedFaces = ChunkSize * ChunkSize * ChunkSize;
    Vertices.Reserve(EstimatedFaces * 6 * 4);
    Triangles.Reserve(EstimatedFaces * 6 * 6);
    Normals.Reserve(EstimatedFaces * 6 * 4);
    UVs.Reserve(EstimatedFaces * 6 * 4);


    for (int x = 0; x < ChunkSize; x++)
    {
        for (int y = 0; y < ChunkSize; y++)
        {
            for (int z = 0; z < ChunkSize; z++)
            {
                if (!IsVoxelSolidLocal(x, y, z)) continue;

				FVector BasePos = FVector(
                    x * VoxelScale,
                    y * VoxelScale, 
                    z * VoxelScale
                );

                auto NeighborSolid = [&](int NX, int NY, int NZ) -> bool
                {
					int GlobalX = ChunkCoords.X * ChunkSize + NX;
					int GlobalY = ChunkCoords.Y * ChunkSize + NY;
					int GlobalZ = NZ;

					if (!WorldManager) return false;

					bool Solid = WorldManager->IsVoxelSolidGlobal(GlobalX, GlobalY, GlobalZ);

                    if (x == 0 || x == ChunkSize - 1 || y == 0 || y == ChunkSize - 1)
                    {
                        bool LocalSolid = IsVoxelSolidLocal(x, y, z);

                        /*
                        UE_LOG(LogTemp, Warning,
                            TEXT("Chunk (%d,%d) voxel (%d,%d,%d): LocalSolid=%d, NeighborSolid(%d,%d,%d)=%d"),
                            ChunkCoords.X, ChunkCoords.Y,
                            x, y, z,
                            LocalSolid,
                            NX, NY, NZ,
                            Solid);
                        */
                        
                    }

                    return Solid;
				};

				// Check neighbors and add faces if neighbor is empty
				if (!NeighborSolid(x + 1, y, z)) AddCubeFace(0, BasePos, Vertices, Triangles, Normals, UVs); // Right
				if (!NeighborSolid(x - 1, y, z)) AddCubeFace(1, BasePos, Vertices, Triangles, Normals, UVs); // Left
				if (!NeighborSolid(x, y + 1, z)) AddCubeFace(2, BasePos, Vertices, Triangles, Normals, UVs); // Front
				if (!NeighborSolid(x, y - 1, z)) AddCubeFace(3, BasePos, Vertices, Triangles, Normals, UVs); // Back
				if (!NeighborSolid(x, y, z + 1)) AddCubeFace(4, BasePos, Vertices, Triangles, Normals, UVs); // Top
				if (!NeighborSolid(x, y, z - 1)) AddCubeFace(5, BasePos, Vertices, Triangles, Normals, UVs); // Bottom
            }
        }
    }

    //const bool bCreateCollision = false;

    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, true);
}
