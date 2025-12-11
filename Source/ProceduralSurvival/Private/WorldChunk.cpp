#include "WorldChunk.h"
#include "WorldManager.h"
#include "MarchingCubeTables.h"
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

    return VoxelData{ Index }.isSolid;
}

void AWorldChunk::SetVoxelLocal(int LocalX, int LocalY, int LocalZ, bool isSolid)
{
    if (!isInitialized) return;

    int Index = LocalIndex(LocalX, LocalY, LocalZ);
    if (Index < 0) return;
    VoxelData{ Index }.isSolid = isSolid;

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

                FVoxel& Voxel = VoxelData{ Index };
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
        FVector Verts{ 4 };
    };

    FCubeFace Faces{ 6 } =
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
        Vertices.Add(Faces{ FaceIndex }.Verts{ i });
        Normals.Add(Faces{ FaceIndex }.Normal);
        UVs.Add(FVector2D((i == 1 || i == 2), (i == 2 || i == 3)));
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
    if (RenderMode == EVoxelRenderMode::Cubes)
    {
        GenerateCubicMesh();
    }
    else if (RenderMode == EVoxelRenderMode::MarchingCubes)
    {
        GenerateMarchingCubesMesh();
    }
}

void AWorldChunk::GenerateCubicMesh()
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

                auto NeighborSolid = { & }(int NX, int NY, int NZ) -> bool
                {
                    int GlobalX = ChunkCoords.X * ChunkSize + NX;
                    int GlobalY = ChunkCoords.Y * ChunkSize + NY;
                    int GlobalZ = NZ;

                    if (!WorldManager) return false;

                    FIntPoint NeighborChunkXY;
                    NeighborChunkXY.X = FMath::FloorToInt((float)GlobalX / ChunkSize);
                    NeighborChunkXY.Y = FMath::FloorToInt((float)GlobalY / ChunkSize);

                    if (!WorldManager->IsChunkWithinRenderDistance(NeighborChunkXY))
                    {
                        return true;
                    }

                    bool Solid = WorldManager->IsVoxelSolidGlobal(GlobalX, GlobalY, GlobalZ);

                    if (x == 0 || x == ChunkSize - 1 || y == 0 || y == ChunkSize - 1)
                    {
                        bool LocalSolid = IsVoxelSolidLocal(x, y, z);
                    }

                    return Solid;
                };

                // Check neighbors and add faces if neighbor is empty
                if (!NeighborSolid(x + 1, y, z)) AddCubeFace(0, BasePos, Vertices, Triangles, Normals, UVs); // Right
                if (!NeighborSolid(x - 1, y, z)) AddCubeFace(1, BasePos, Vertices, Triangles, Normals, UVs); // Left
                if (!NeighborSolid(x, y + 1, z)) AddCubeFace(2, BasePos, Vertices, Triangles, Normals, UVs); // Front
                if (!NeighborSolid(x, y - 1, z)) AddCubeFace(3, BasePos, Vertices, Triangles, Normals, UVs); // Back
                if (!NeighborSolid(x, y, z + 1)) AddCubeFace(4, BasePos, Vertices, Triangles, Normals, UVs); // Top

                if (!ShouldCullBottomFace(x, y, z))
                {
                    if (!NeighborSolid(x, y, z - 1)) AddCubeFace(5, BasePos, Vertices, Triangles, Normals, UVs); // Bottom
                }
            }
        }
    }

    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, true);
}

void AWorldChunk::GenerateMarchingCubesMesh()
{
    const float IsoLevel = 0.0f;

    if (Mesh)
    {
        Mesh->ClearAllMeshSections();
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    Vertices.Reserve(ChunkSize * ChunkSize * ChunkSize);
    Triangles.Reserve(ChunkSize * ChunkSize * ChunkSize * 5);
    Normals.Reserve(ChunkSize * ChunkSize * ChunkSize);
    UVs.Reserve(ChunkSize * ChunkSize * ChunkSize);

    for (int x = 0; x < ChunkSize - 1; x++)
    {
        for (int y = 0; y < ChunkSize - 1; y++)
        {
            for (int z = 0; z < ChunkSize - 1; z++)
            {
                int gx = ChunkCoords.X * ChunkSize + x;
                int gy = ChunkCoords.Y * ChunkSize + y;
                int gz = z;

                float val{ 8 };
                FVector pos{ 8 };

                pos{ 0 } = FVector(gx * VoxelScale, gy * VoxelScale, gz * VoxelScale);
                pos{ 1 } = FVector((gx + 1) * VoxelScale, gy * VoxelScale, gz * VoxelScale);
                pos{ 2 } = FVector((gx + 1) * VoxelScale, (gy + 1) * VoxelScale, gz * VoxelScale);
                pos{ 3 } = FVector(gx * VoxelScale, (gy + 1) * VoxelScale, gz * VoxelScale);
                pos{ 4 } = FVector(gx * VoxelScale, gy * VoxelScale, (gz + 1) * VoxelScale);
                pos{ 5 } = FVector((gx + 1) * VoxelScale, gy * VoxelScale, (gz + 1) * VoxelScale);
                pos{ 6 } = FVector((gx + 1) * VoxelScale, (gy + 1) * VoxelScale, (gz + 1) * VoxelScale);
                pos{ 7 } = FVector(gx * VoxelScale, (gy + 1) * VoxelScale, (gz + 1) * VoxelScale);

                val{ 0 } = SampleDensityAtGlobalVoxel(gx, gy, gz);
                val{ 1 } = SampleDensityAtGlobalVoxel(gx + 1, gy, gz);
                val{ 2 } = SampleDensityAtGlobalVoxel(gx + 1, gy + 1, gz);
                val{ 3 } = SampleDensityAtGlobalVoxel(gx, gy + 1, gz);
                val{ 4 } = SampleDensityAtGlobalVoxel(gx, gy, gz + 1);
                val{ 5 } = SampleDensityAtGlobalVoxel(gx + 1, gy, gz + 1);
                val{ 6 } = SampleDensityAtGlobalVoxel(gx + 1, gy + 1, gz + 1);
                val{ 7 } = SampleDensityAtGlobalVoxel(gx, gy + 1, gz + 1);

                int cubeIndex = 0;

                if (val{ 0 } < IsoLevel) cubeIndex |= 1;
                if (val{ 1 } < IsoLevel) cubeIndex |= 2;
                if (val{ 2 } < IsoLevel) cubeIndex |= 4;
                if (val{ 3 } < IsoLevel) cubeIndex |= 8;
                if (val{ 4 } < IsoLevel) cubeIndex |= 16;
                if (val{ 5 } < IsoLevel) cubeIndex |= 32;
                if (val{ 6 } < IsoLevel) cubeIndex |= 64;
                if (val{ 7 } < IsoLevel) cubeIndex |= 128;

                if (edgeTable{ cubeIndex } == 0) continue;

                FVector vertList{ 12 };
                if (edgeTable{ cubeIndex } &1) vertList{ 0 } = VertexInterp(IsoLevel, pos{ 0 }, pos{ 1 }, val{ 0 }, val{ 1 });
                if (edgeTable{ cubeIndex } &2) vertList{ 1 } = VertexInterp(IsoLevel, pos{ 1 }, pos{ 2 }, val{ 1 }, val{ 2 });
                if (edgeTable{ cubeIndex } &4) vertList{ 2 } = VertexInterp(IsoLevel, pos{ 2 }, pos{ 3 }, val{ 2 }, val{ 3 });
                if (edgeTable{ cubeIndex } &8) vertList{ 3 } = VertexInterp(IsoLevel, pos{ 3 }, pos{ 0 }, val{ 3 }, val{ 0 });
                if (edgeTable{ cubeIndex } &16) vertList{ 4 } = VertexInterp(IsoLevel, pos{ 4 }, pos{ 5 }, val{ 4 }, val{ 5 });
                if (edgeTable{ cubeIndex } &32) vertList{ 5 } = VertexInterp(IsoLevel, pos{ 5 }, pos{ 6 }, val{ 5 }, val{ 6 });
                if (edgeTable{ cubeIndex } &64) vertList{ 6 } = VertexInterp(IsoLevel, pos{ 6 }, pos{ 7 }, val{ 6 }, val{ 7 });
                if (edgeTable{ cubeIndex } &128) vertList{ 7 } = VertexInterp(IsoLevel, pos{ 7 }, pos{ 4 }, val{ 7 }, val{ 4 });
                if (edgeTable{ cubeIndex } &256) vertList{ 8 } = VertexInterp(IsoLevel, pos{ 0 }, pos{ 4 }, val{ 0 }, val{ 4 });
                if (edgeTable{ cubeIndex } &512) vertList{ 9 } = VertexInterp(IsoLevel, pos{ 1 }, pos{ 5 }, val{ 1 }, val{ 5 });
                if (edgeTable{ cubeIndex } &1024) vertList{ 10 } = VertexInterp(IsoLevel, pos{ 2 }, pos{ 6 }, val{ 2 }, val{ 6 });
                if (edgeTable{ cubeIndex } &2048) vertList{ 11 } = VertexInterp(IsoLevel, pos{ 3 }, pos{ 7 }, val{ 3 }, val{ 7 });

                for (int i = 0; triTable{ cubeIndex }{i} != -1; i += 3)
                {
                    int idx0 = triTable{ cubeIndex }{i};
                    int idx1 = triTable{ cubeIndex }{i + 1};
                    int idx2 = triTable{ cubeIndex }{i + 2};

                    FVector v0 = vertList{ idx0 };
                    FVector v1 = vertList{ idx1 };
                    FVector v2 = vertList{ idx2 };

                    int vertStartIndex = Vertices.Num();
                    Vertices.Add(v0);
                    Vertices.Add(v1);
                    Vertices.Add(v2);

                    FVector normal = FVector::CrossProduct(v1 - v0, v2 - v0).GetSafeNormal();
                    Normals.Add(normal);
                    Normals.Add(normal);
                    Normals.Add(normal);

                    UVs.Add(FVector2D(v0.X / 1000.0f, v0.Y / 1000.0f));
                    UVs.Add(FVector2D(v1.X / 1000.0f, v1.Y / 1000.0f));
                    UVs.Add(FVector2D(v2.X / 1000.0f, v2.Y / 1000.0f));

                    Triangles.Add(vertStartIndex + 0);
                    Triangles.Add(vertStartIndex + 1);
                    Triangles.Add(vertStartIndex + 2);
                }
            }
        }
    }

    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, true);
}

FVector AWorldChunk::VertexInterp(float IsoLevel, const FVector& P1, const FVector& P2, float ValP1, float ValP2) const
{
    if (FMath::Abs(IsoLevel - ValP1) < KINDA_SMALL_NUMBER)
        return P1;
    if (FMath::Abs(IsoLevel - ValP2) < KINDA_SMALL_NUMBER)
        return P2;
    if (FMath::Abs(ValP1 - ValP2) < KINDA_SMALL_NUMBER)
        return P1;
    float Mu = (IsoLevel - ValP1) / (ValP2 - ValP1);
    return P1 + Mu * (P2 - P1);
}

float AWorldChunk::SampleDensityAtGlobalVoxel(int GlobalX, int GlobalY, int GlobalZ) const
{
    const float NoiseScale = 0.08f;
    const float HeightMultiplier = 5.0f;
    const float BaseHeight = ChunkSize * 0.4f;

    float SampleX = GlobalX * NoiseScale;
    float SampleY = GlobalY * NoiseScale;

    float Height = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY)) * HeightMultiplier + BaseHeight;

    return Height - (float)GlobalZ;
}

FVector AWorldChunk::ComputeGradientAt(const AWorldChunk* Chunk, int GX, int GY, int GZ) const
{
    const float EPS = 0.5f;
    float DX1 = Chunk->SampleDensityAtGlobalVoxel(GX + 1, GY, GZ);
    float DX2 = Chunk->SampleDensityAtGlobalVoxel(GX - 1, GY, GZ);
    float Dy1 = Chunk->SampleDensityAtGlobalVoxel(GX, GY + 1, GZ);
    float Dy2 = Chunk->SampleDensityAtGlobalVoxel(GX, GY - 1, GZ);
    float Dz1 = Chunk->SampleDensityAtGlobalVoxel(GX, GY, GZ + 1);
    float Dz2 = Chunk->SampleDensityAtGlobalVoxel(GX, GY, GZ - 1);

    FVector grad;
    grad.X = (DX1 - DX2) * EPS;
    grad.Y = (Dy1 - Dy2) * EPS;
    grad.Z = (Dz1 - Dz2) * EPS;

    return grad;
}

bool AWorldChunk::ShouldCullBottomFace(int X, int Y, int Z) const
{
    if (Z == 0) return true;
    return IsVoxelSolidLocal(X, Y, Z - 1);
}
