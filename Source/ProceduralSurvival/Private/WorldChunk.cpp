#include "WorldChunk.h"
#include "WorldManager.h"
#include "TerrainGenerator.h"
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

void AWorldChunk::InitializeChunk(int InChunkSizeXY, int InChunkHeightZ, float InVoxelScale, const FIntPoint& InChunkCoords)
{
    ChunkSizeXY = FMath::Max(1, InChunkSizeXY);
    ChunkHeightZ = FMath::Max(1, InChunkHeightZ);

    VoxelScale = InVoxelScale;
    ChunkCoords = InChunkCoords;

    const int32 Total = ChunkSizeXY * ChunkSizeXY * ChunkHeightZ;
    VoxelData.SetNumZeroed(Total);

	isLOD0Built = false;
	isLOD0SeamDirty = true;
    isInitialized = true;
}

int AWorldChunk::LocalIndex(int X, int Y, int Z) const
{
    if (X < 0 || X >= ChunkSizeXY || Y < 0 || Y >= ChunkSizeXY || Z < 0 || Z >= ChunkHeightZ)
    {
        return -1;
    }

    return X + Y * ChunkSizeXY + Z * ChunkSizeXY * ChunkSizeXY;
}

bool AWorldChunk::IsVoxelSolidLocal(int LocalX, int LocalY, int LocalZ) const
{
    if (LocalX < 0 || LocalX >= ChunkSizeXY || LocalY < 0 || LocalY >= ChunkSizeXY || LocalZ < 0 || LocalZ >= ChunkHeightZ) return false;

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
    VoxelData[Index].density = isSolid ? 1.0f : -1.0f;

    GenerateMeshLOD(WorldManager->ComputeLODForChunk(ChunkCoords));
}

void AWorldChunk::GenerateVoxels()
{
    if (!isInitialized || !WorldManager || !WorldManager->TerrainGenerator) return;

	UTerrainGenerator* TerrainGen = WorldManager->TerrainGenerator;

	const int32 BaseX = ChunkCoords.X * ChunkSizeXY;
	const int32 BaseY = ChunkCoords.Y * ChunkSizeXY;

    for (int x = 0; x < ChunkSizeXY; x++)
    {
        for (int y = 0; y < ChunkSizeXY; y++)
        {
            const float GX = BaseX + x;
			const float GY = BaseY + y;

            for (int z = 0; z < ChunkHeightZ; z++)
            {
				const float GZ = z;

                int32 Index = LocalIndex(x, y, z);
                if (Index < 0) continue;

                FVoxel& Voxel = VoxelData[Index];

				float Density = TerrainGen->GetDensity(GX, GY, GZ);

                Voxel.density = Density;
                Voxel.isSolid = (Density >= 0.0f);
                
            }
        }
    }

	VoxelsGenerated = true;
}

void AWorldChunk::AddCubeFace(int FaceIndex, FVector& Position, float CubeSize, FColor FaceColor, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& VertexColors)
{
    float S = CubeSize;

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
            Position + FVector(S,S,0) } 
        },

        // Left
        { FVector(-1,0,0), {
            Position + FVector(0,0,0),
            Position + FVector(0,S,0),
            Position + FVector(0,S,S),
            Position + FVector(0,0,S) } 
        },

        // Front
        { FVector(0,1,0), {
            Position + FVector(0,S,0),
            Position + FVector(S,S,0),
            Position + FVector(S,S,S),
            Position + FVector(0,S,S) } 
        },

        // Back
        { FVector(0,-1,0), {
            Position + FVector(0,0,0),
            Position + FVector(0,0,S),
            Position + FVector(S,0,S),
            Position + FVector(S,0,0) } 
        },

        // Top
        { FVector(0,0,1), {
            Position + FVector(0,0,S),
            Position + FVector(0,S,S),
            Position + FVector(S,S,S),
            Position + FVector(S,0,S) } 
        },

        // Bottom
        { FVector(0,0,-1), {
            Position + FVector(0,0,0),
            Position + FVector(S,0,0),
            Position + FVector(S,S,0),
            Position + FVector(0,S,0) } 
        }
    };

    int Start = Vertices.Num();

    // Add vertices
    for (int i = 0; i < 4; ++i)
    {
        Vertices.Add(Faces[FaceIndex].Verts[i]);
        Normals.Add(Faces[FaceIndex].Normal);
        UVs.Add(FVector2D((i == 1 || i == 2), (i == 2 || i == 3)));
        VertexColors.Add(FaceColor);
    }

    // Add triangles
    Triangles.Add(Start + 0);
    Triangles.Add(Start + 1);
    Triangles.Add(Start + 2);

    Triangles.Add(Start + 0);
    Triangles.Add(Start + 2);
    Triangles.Add(Start + 3);
}

bool AWorldChunk::BuildCubicMeshData(int32 LODLevel, int32 LODStep, bool ProceduralOnly, FChunkMeshBuffers& OutBuffers)
{
    OutBuffers.Vertices.Reset();
    OutBuffers.Triangles.Reset();
    OutBuffers.Normals.Reset();
    OutBuffers.UVs.Reset();
    OutBuffers.VertexColors.Reset();

    const int EstimatedFaces = ChunkSizeXY * ChunkSizeXY * ChunkHeightZ;
    OutBuffers.Vertices.Reserve(EstimatedFaces * 6 * 4);
    OutBuffers.Triangles.Reserve(EstimatedFaces * 6 * 6);
    OutBuffers.Normals.Reserve(EstimatedFaces * 6 * 4);
    OutBuffers.UVs.Reserve(EstimatedFaces * 6 * 4);

	const int ScaledVoxel = VoxelScale * LODStep;

    for (int x = 0; x < ChunkSizeXY; x += LODStep)
    {
        for (int y = 0; y < ChunkSizeXY; y += LODStep)
        {
            const int DepthSteps = 1;
			int32 SurfaceZ = -1;

            auto SampleSolidAt = [&](int LX, int LY, int LZ) -> bool
            {
                int gx2 = ChunkCoords.X * ChunkSizeXY + LX;
                int gy2 = ChunkCoords.Y * ChunkSizeXY + LY;
                
				const float Density = (LODLevel == 0 && AreVoxelsGenerated()) 
                    ? GetVoxelDensity({ LX, LY, LZ }) 
                    : WorldManager->TerrainGenerator->GetDensity(gx2, gy2, LZ);

				return (Density >= 0.0f);
            };

            for (int z2 = ChunkHeightZ - LODStep; z2 >= 0; z2 -= LODStep)
            {
				const bool isSolid = SampleSolidAt(x, y, z2);
				const bool isAirAbove = (z2 + LODStep >= ChunkHeightZ) ? true : !SampleSolidAt(x, y, z2 + LODStep);

                if (isSolid && isAirAbove)
                {
                    SurfaceZ = z2;
                    break;
				}
			}

            const int32 MinSeamZ = (SurfaceZ < 0) ? INT32_MAX : FMath::Max(0, SurfaceZ - DepthSteps * LODStep);

            for (int z = 0; z < ChunkHeightZ; z += LODStep)
            {
				const int gx = ChunkCoords.X * ChunkSizeXY + x;
				const int gy = ChunkCoords.Y * ChunkSizeXY + y;

				const float Density = (LODLevel == 0 && AreVoxelsGenerated())? GetVoxelDensity({x, y, z}) : WorldManager->TerrainGenerator->GetDensity(gx, gy, z);

				if (Density < 0.0f) continue; // Empty space

                FVector BasePos = FVector(
                    x * VoxelScale,
                    y * VoxelScale,
                    z * VoxelScale
                );

                auto IsOnChunkBorder = [&](int x, int y)
                {
                    return (x == 0 || x + LODStep >= ChunkSizeXY || y == 0 || y + LODStep >= ChunkSizeXY);
                };

                auto IsNeighborDifferentLOD = [&](int dx, int dy) -> bool
                {
					FIntPoint NeighborXY = ChunkCoords + FIntPoint(dx, dy);
                    
					AWorldChunk* Neighbor = WorldManager->GetChunkAt(NeighborXY);
					if (!Neighbor) return false;

                    int NeighborLOD = Neighbor->GetCurrentLODLevel();
					return (NeighborLOD != LODLevel);
				};

                auto NeighborSolid = [&](int NX, int NY, int NZ) -> bool
                {
                    int GlobalX = ChunkCoords.X * ChunkSizeXY + NX;
                    int GlobalY = ChunkCoords.Y * ChunkSizeXY + NY;
                    int GlobalZ = NZ;

                    if (!WorldManager) return true;

                    if (ProceduralOnly)
                    {
                        float NeighborDensity = WorldManager->TerrainGenerator->GetDensity(GlobalX, GlobalY, GlobalZ);
                        return (NeighborDensity >= 0.0f);
					}

                    FIntPoint NeighborChunkXY;
                    NeighborChunkXY.X = FMath::FloorToInt((float)GlobalX / ChunkSizeXY);
                    NeighborChunkXY.Y = FMath::FloorToInt((float)GlobalY / ChunkSizeXY);

                    if (GlobalZ < 0 || GlobalZ >= ChunkHeightZ)
                    {
                        return true;
                    }

                    if (!WorldManager->IsChunkWithinRenderDistance(NeighborChunkXY))
                    {
                        return true;
                    }

                    if (!WorldManager->IsNeighborChunkLoaded(NeighborChunkXY))
                    {
                        return true;
                    }

                    bool Solid = WorldManager->IsVoxelSolidGlobal(GlobalX, GlobalY, GlobalZ);

                    if (x == 0 || x == ChunkSizeXY - 1 || y == 0 || y == ChunkSizeXY - 1)
                    {
                        bool LocalSolid = IsVoxelSolidLocal(x, y, z);
                    }

                    return Solid;
                };

                EBiomeType Biome = WorldManager->TerrainGenerator->GetDominantBiome(gx, gy);
                FColor BiomeColor;

                switch (Biome)
                {
                case EBiomeType::Plains:
                    BiomeColor = FColor::Green;
                    break;
                case EBiomeType::Hills:
                    BiomeColor = FColor::Blue;
                    break;
                case EBiomeType::Mountains:
                    BiomeColor = FColor::Red;
                    break;
                }

                // Check neighbors and add faces if neighbor is empty
				const bool isNearSurface = (z >= MinSeamZ);

                if (x + LODStep >= ChunkSizeXY)
                {
                    if (IsNeighborDifferentLOD(1, 0) && isNearSurface)
                    {
                        AddCubeFace(0, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Right
					}
                    else if (!NeighborSolid(x + LODStep, y, z))
                    {
                        AddCubeFace(0, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Right
                    }
                }
                else if (!NeighborSolid(x + LODStep, y, z))
                {
                    AddCubeFace(0, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Right
				}

                if (x - LODStep < 0)
                {
                    if (IsNeighborDifferentLOD(-1, 0) && isNearSurface)
                    {
                        AddCubeFace(1, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Left
                    }
                    else if (!NeighborSolid(x - LODStep, y, z))
                    {
                        AddCubeFace(1, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Left
                    }
                }
                else if (!NeighborSolid(x - LODStep, y, z))
                {
                    AddCubeFace(1, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Left
				}

                if (y + LODStep >= ChunkSizeXY)
                {
                    if (IsNeighborDifferentLOD(0, 1) && isNearSurface)
                    {
                        AddCubeFace(2, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Front
                    }
                    else if (!NeighborSolid(x, y + LODStep, z))
                    {
                        AddCubeFace(2, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Front
                    }
                }
                else if (!NeighborSolid(x, y + LODStep, z))
                {
                    AddCubeFace(2, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Front
                }

                if (y - LODStep < 0)
                {
                    if (IsNeighborDifferentLOD(0, -1) && isNearSurface)
                    {
                        AddCubeFace(3, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Back
                    }
                    else if (!NeighborSolid(x, y - LODStep, z))
                    {
                        AddCubeFace(3, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Back
                    }
                }
                else if (!NeighborSolid(x, y - LODStep, z))
                {
                    AddCubeFace(3, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Back
                }

                if (!NeighborSolid(x, y, z + LODStep)) AddCubeFace(4, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Top

                if (!ShouldCullBottomFace(x, y, z))
                {
                    if (!NeighborSolid(x, y, z - LODStep)) AddCubeFace(5, BasePos, ScaledVoxel, BiomeColor, OutBuffers.Vertices, OutBuffers.Triangles, OutBuffers.Normals, OutBuffers.UVs, OutBuffers.VertexColors); // Bottom
                }
            }
        }
    }

    return true;
}

bool AWorldChunk::BuildMarchingCubeData(int32 LODLevel, int32 LODStep, bool ProceduralOnly, FChunkMeshBuffers& OutBuffers)
{
    if (LODLevel == 0 && (!WorldManager || !WorldManager->AreAllNeighborChunksVoxelReady(ChunkCoords))) return false;

    const float IsoLevel = 0.0f;

    TArray<FVector> GradientCache;
    GradientCache.SetNumZeroed(ChunkSizeXY * ChunkSizeXY * ChunkHeightZ);
    ComputeGradient(GradientCache);

    OutBuffers.Vertices.Reset();
    OutBuffers.Triangles.Reset();
    OutBuffers.Normals.Reset();
    OutBuffers.UVs.Reset();
    OutBuffers.VertexColors.Reset();

	TMap<FIntVector, int32> VertexIndexMap;
	VertexIndexMap.Reserve(1024);

	TArray<FVector> NormalAcc; NormalAcc.Reset();

    auto MakeVertexKey = [&](const FVector& Vertex) -> FIntVector
    {
        const FVector WorldVertex = GetActorLocation() + Vertex;

		return FIntVector(
			FMath::RoundToInt(WorldVertex.X * 100.0f),
			FMath::RoundToInt(WorldVertex.Y * 100.0f),
			FMath::RoundToInt(WorldVertex.Z * 100.0f)
        );
	};

	const int32 EstimatedCells = ChunkSizeXY * ChunkSizeXY * ChunkHeightZ;
    OutBuffers.Vertices.Reserve(EstimatedCells * 2);
    OutBuffers.Triangles.Reserve(EstimatedCells * 5);
    OutBuffers.Normals.Reserve(EstimatedCells * 2);
    NormalAcc.Reserve(EstimatedCells * 2);
    OutBuffers.UVs.Reserve(EstimatedCells * 2);

    for (int x = 0; x < ChunkSizeXY; x += LODStep)
    {
        for (int y = 0; y < ChunkSizeXY; y += LODStep)
        {
            for (int z = 0; z < ChunkHeightZ; z += LODStep)
            {
                int gx = ChunkCoords.X * ChunkSizeXY + x;
                int gy = ChunkCoords.Y * ChunkSizeXY + y;
                int gz = z;

                float val[8];
                FVector pos[8];

                pos[0] = FVector(x, y, z) * VoxelScale;
                pos[1] = FVector(x + LODStep, y, z) * VoxelScale;
                pos[2] = FVector(x + LODStep, y + LODStep, z) * VoxelScale;
                pos[3] = FVector(x, y + LODStep, z) * VoxelScale;
                pos[4] = FVector(x, y, z + LODStep) * VoxelScale;
                pos[5] = FVector(x + LODStep, y, z + LODStep) * VoxelScale;
                pos[6] = FVector(x + LODStep, y + LODStep, z + LODStep) * VoxelScale;
                pos[7] = FVector(x, y + LODStep, z + LODStep) * VoxelScale;

                val[0] = SampleDensityForMarching(gx, gy, gz, ProceduralOnly);
                val[1] = SampleDensityForMarching(gx + LODStep, gy, gz, ProceduralOnly);
                val[2] = SampleDensityForMarching(gx + LODStep, gy + LODStep, gz, ProceduralOnly);
                val[3] = SampleDensityForMarching(gx, gy + LODStep, gz, ProceduralOnly);
                val[4] = SampleDensityForMarching(gx, gy, gz + LODStep, ProceduralOnly);
                val[5] = SampleDensityForMarching(gx + LODStep, gy, gz + LODStep, ProceduralOnly);
                val[6] = SampleDensityForMarching(gx + LODStep, gy + LODStep, gz + LODStep, ProceduralOnly);
                val[7] = SampleDensityForMarching(gx, gy + LODStep, gz + LODStep, ProceduralOnly);

                int cubeIndex = 0;

                if (val[0] > IsoLevel) cubeIndex |= 1;
                if (val[1] > IsoLevel) cubeIndex |= 2;
                if (val[2] > IsoLevel) cubeIndex |= 4;
                if (val[3] > IsoLevel) cubeIndex |= 8;
                if (val[4] > IsoLevel) cubeIndex |= 16;
                if (val[5] > IsoLevel) cubeIndex |= 32;
                if (val[6] > IsoLevel) cubeIndex |= 64;
                if (val[7] > IsoLevel) cubeIndex |= 128;

                if (MarchingCubeTables::edgeTable[cubeIndex] == 0) continue;

                FVector vertList[12];
                if (MarchingCubeTables::edgeTable[cubeIndex] &1) vertList[0] = VertexInterp(IsoLevel, pos[0], pos[1], val[0], val[1]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 2) vertList[1] = VertexInterp(IsoLevel, pos[1], pos[2], val[1], val[2]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 4) vertList[2] = VertexInterp(IsoLevel, pos[2], pos[3], val[2], val[3]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 8) vertList[3] = VertexInterp(IsoLevel, pos[3], pos[0], val[3], val[0]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 16) vertList[4] = VertexInterp(IsoLevel, pos[4], pos[5], val[4], val[5]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 32) vertList[5] = VertexInterp(IsoLevel, pos[5], pos[6], val[5], val[6]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 64) vertList[6] = VertexInterp(IsoLevel, pos[6], pos[7], val[6], val[7]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 128) vertList[7] = VertexInterp(IsoLevel, pos[7], pos[4], val[7], val[4]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 256) vertList[8] = VertexInterp(IsoLevel, pos[0], pos[4], val[0], val[4]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 512) vertList[9] = VertexInterp(IsoLevel, pos[1], pos[5], val[1], val[5]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 1024) vertList[10] = VertexInterp(IsoLevel, pos[2], pos[6], val[2], val[6]);
				if (MarchingCubeTables::edgeTable[cubeIndex] & 2048) vertList[11] = VertexInterp(IsoLevel, pos[3], pos[7], val[3], val[7]);

                for (int i = 0; MarchingCubeTables::triTable[cubeIndex][i] != -1; i += 3)
                {
                    int idx0 = MarchingCubeTables::triTable[cubeIndex][i];
                    int idx1 = MarchingCubeTables::triTable[cubeIndex][i + 1];
                    int idx2 = MarchingCubeTables::triTable[cubeIndex][i + 2];

                    FVector v0 = vertList[idx0];
                    FVector v1 = vertList[idx1];
                    FVector v2 = vertList[idx2];

                    auto GetOrCreateVertexIndex = [&](const FVector& Vertex) -> int32
                    {
                        const FIntVector Key = MakeVertexKey(Vertex);

                        if (VertexIndexMap.Contains(Key))
                        {
                            return VertexIndexMap[Key];
                        }
                        else
                        {
                            const int32 NewIndex = OutBuffers.Vertices.Add(Vertex);
                            VertexIndexMap.Add(Key, NewIndex);

							NormalAcc.Add(FVector::ZeroVector);
                            OutBuffers.UVs.Add(FVector2D(Vertex.X / 1000.0f, Vertex.Y / 1000.0f));

                            const EBiomeType Biome = WorldManager->TerrainGenerator->GetDominantBiome(gx, gy);
                            
                            OutBuffers.VertexColors.Add(
                                (Biome == EBiomeType::Plains) ? FColor::Green :
                                (Biome == EBiomeType::Hills) ? FColor::Blue :
                                (Biome == EBiomeType::Mountains) ? FColor::Red :
								FColor::Black
                            );

                            return NewIndex;
                        }
					};

					int i0 = GetOrCreateVertexIndex(v0);
					int i1 = GetOrCreateVertexIndex(v1);
                    int i2 = GetOrCreateVertexIndex(v2);

					FVector faceNormal = FVector::CrossProduct(v2 - v0, v1 - v0);
					faceNormal.Normalize();

                    OutBuffers.Triangles.Add(i0);
                    OutBuffers.Triangles.Add(i1);
                    OutBuffers.Triangles.Add(i2);

                    auto SampleNormal = [&](const FVector& V) -> FVector
                    {
                        const float lx = V.X / VoxelScale;
                        const float ly = V.Y / VoxelScale;
                        const float lz = V.Z / VoxelScale;

						const int ix = FMath::FloorToInt(lx);
						const int iy = FMath::FloorToInt(ly);
						const int iz = FMath::FloorToInt(lz);

                        const bool bInterior =
                            ix > 0 && ix < ChunkSizeXY - 1 &&
                            iy > 0 && iy < ChunkSizeXY - 1 &&
                            iz > 0 && iz < ChunkHeightZ - 1;

                        if (bInterior)
                        {
                            const int Index = LocalIndex(ix, iy, iz);
                            if (Index >= 0)
                            {
                                return -GradientCache[Index];
                            }
                        }

                        const int gx = ChunkCoords.X * ChunkSizeXY + ix;
						const int gy = ChunkCoords.Y * ChunkSizeXY + iy;
						const int gz = iz;

                        const float dx =
                            SampleDensityForMarching(gx + 1, gy, gz, ProceduralOnly) -
                            SampleDensityForMarching(gx - 1, gy, gz, ProceduralOnly);

                        const float dy =
                            SampleDensityForMarching(gx, gy + 1, gz, ProceduralOnly) -
                            SampleDensityForMarching(gx, gy - 1, gz, ProceduralOnly);

                        const float dz =
                            SampleDensityForMarching(gx, gy, gz + 1, ProceduralOnly) -
                            SampleDensityForMarching(gx, gy, gz - 1, ProceduralOnly);

                        return -FVector(dx, dy, dz).GetSafeNormal();
                    };

                    NormalAcc[i0] += SampleNormal(v0);
                    NormalAcc[i1] += SampleNormal(v1);
                    NormalAcc[i2] += SampleNormal(v2);
                }
            }
        }
    }

    OutBuffers.Vertices.Reserve(OutBuffers.Vertices.Num() + 1024);
    OutBuffers.Normals.Reserve(OutBuffers.Normals.Num() + 1024);
    OutBuffers.UVs.Reserve(OutBuffers.UVs.Num() + 1024);
    OutBuffers.VertexColors.Reserve(OutBuffers.VertexColors.Num() + 1024);
    OutBuffers.Triangles.Reserve(OutBuffers.Triangles.Num() + 2048);


    const float SkirtDepth = VoxelScale * CurrentLODStep * 2.0f;

    auto AddSkirtQuad = [&](const FVector& A, const FVector& B)
    {
        int32 Start = OutBuffers.Vertices.Num();

        FVector A2 = A - FVector(0, 0, SkirtDepth);
        FVector B2 = B - FVector(0, 0, SkirtDepth);

        OutBuffers.Vertices.Add(A);
        OutBuffers.Vertices.Add(B);
        OutBuffers.Vertices.Add(B2);
        OutBuffers.Vertices.Add(A2);

        FVector Normal = FVector::CrossProduct(B - A, A2 - A).GetSafeNormal();
        for (int i = 0; i < 4; ++i)
        {
            OutBuffers.Normals.Add(Normal);
            OutBuffers.UVs.Add(FVector2D(0.0f, 0.0f));
            OutBuffers.VertexColors.Add(FColor::Black);
        }

        OutBuffers.Triangles.Append({
            Start + 0, Start + 1, Start + 2,
            Start + 0, Start + 2, Start + 3
        });
    };

    OutBuffers.Normals.Init(FVector::ZeroVector, OutBuffers.Vertices.Num());

    for (int32 i = 0; i < NormalAcc.Num(); ++i)
    {
        FVector Normal = NormalAcc[i].GetSafeNormal();

        if (!Normal.IsNearlyZero())
        {
            OutBuffers.Normals[i] = Normal;
        }
        else
        {
            OutBuffers.Normals[i] = FVector::UpVector;
		}
	}

	const int32 VertexCount = OutBuffers.Vertices.Num();

    for (int i = 0; i < VertexCount; ++i)
    {
		const FVector V = OutBuffers.Vertices[i];
        const bool bBorder = V.X <= 0.01f || V.X >= ChunkSizeXY * VoxelScale - 0.01f || V.Y <= 0.01f || V.Y >= ChunkSizeXY * VoxelScale - 0.01f;

        if (bBorder)
        {
            if (V.X <= 0.01f)
            {
                AddSkirtQuad(V, V + FVector(0.01, 0, 0));
            }
            else if (V.X >= ChunkSizeXY * VoxelScale - 0.01f)
            {
                AddSkirtQuad(V, V - FVector(0.01, 0, 0));
			}
            else if (V.Y <= 0.01f)
            {
                AddSkirtQuad(V, V + FVector(0, 0.01, 0));
            }
            else if (V.Y >= ChunkSizeXY * VoxelScale - 0.01f)
            {
                AddSkirtQuad(V, V - FVector(0, 0.01, 0));
			}
        }
    }

    /*if (BiomeDebugMaterial)
    {
		Mesh->SetMaterial(0, BiomeDebugMaterial);
    }*/

	return true;
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
	Mu = FMath::Clamp(Mu, 0.0f, 1.0f);
    return P1 + Mu * (P2 - P1);
}

void AWorldChunk::ApplyGeneratedVoxels(TArray<FVoxel>&& InVoxels)
{
    VoxelData = MoveTemp(InVoxels);
    VoxelsGenerated = true;
}

float AWorldChunk::SampleDensityForMarching(int GlobalX, int GlobalY, int GlobalZ, bool ProceduralOnly) const
{
    if (!WorldManager || !WorldManager->TerrainGenerator) return 1.0f;
	if (GlobalZ < 0) return 1.0f;
	if (GlobalZ >= ChunkHeightZ) return -1.0f;

    FIntPoint ChunkXY;
    FIntVector LocalXYZ;
    WorldManager->GlobalVoxelToChunkCoords(GlobalX, GlobalY, GlobalZ, ChunkXY, LocalXYZ);

    if (!ProceduralOnly)
    {
        if (AWorldChunk* Neighbor = WorldManager->GetChunkAt(ChunkXY))
        {
            if (Neighbor->AreVoxelsGenerated() && LocalXYZ.X >= 0 && LocalXYZ.X < Neighbor->GetChunkSizeXY() && LocalXYZ.Y >= 0 && LocalXYZ.Y < Neighbor->GetChunkSizeXY() && LocalXYZ.Z >= 0 && LocalXYZ.Z < Neighbor->GetChunkHeightZ())
            {
                return Neighbor->GetVoxelDensity(LocalXYZ);
            }
        }
    }

    if (!ProceduralOnly && (!WorldManager->IsChunkWithinRenderDistance(ChunkXY) || !WorldManager->IsNeighborChunkLoaded(ChunkXY)))
    {
        return -1.0f;
    }

	return WorldManager->TerrainGenerator->GetDensity((float)GlobalX, (float)GlobalY, (float)GlobalZ);
}

void AWorldChunk::ComputeGradient(TArray<FVector>& GradientCache)
{
    const int EPS = 1;

    for (int x = 0; x < ChunkSizeXY; x++)
    {
        for (int y = 0; y < ChunkSizeXY; y++)
        {
            for (int z = 0; z < ChunkHeightZ; z++)
            {
                int GX = ChunkCoords.X * ChunkSizeXY + x;
                int GY = ChunkCoords.Y * ChunkSizeXY + y;
                int GZ = z;

                const float DX = (float)SampleDensityForMarching(GX + EPS, GY, GZ, useProceduralDensityOnly) - (float)SampleDensityForMarching(GX - EPS, GY, GZ, useProceduralDensityOnly);
                const float DY = (float)SampleDensityForMarching(GX, GY + EPS, GZ, useProceduralDensityOnly) - (float)SampleDensityForMarching(GX, GY - EPS, GZ, useProceduralDensityOnly);
                const float DZ = (float)SampleDensityForMarching(GX, GY, GZ + EPS, useProceduralDensityOnly) - (float)SampleDensityForMarching(GX, GY, GZ - EPS, useProceduralDensityOnly);

				GradientCache[LocalIndex(x, y, z)] = FVector(DX, DY, DZ).GetSafeNormal();
            }
        }
	}
}

bool AWorldChunk::ShouldCullBottomFace(int X, int Y, int Z) const
{
    if (Z == 0) return true;
    return IsVoxelSolidLocal(X, Y, Z - 1);
}

float AWorldChunk::GetVoxelDensity(const FIntVector& LocalXYZ) const
{
    if (!isInitialized) return 1.0f;

     const int Index = LocalIndex(LocalXYZ.X, LocalXYZ.Y, LocalXYZ.Z);
    if (Index < 0) return -1.0f;

    return VoxelData[Index].density;
}

bool AWorldChunk::GenerateMeshLOD(int32 LODLevel)
{
    FChunkMeshBuffers Buffers;

    if (!BuildMeshLODData(LODLevel, Buffers))
    {
        return false;
	}

	ApplyMeshData(Buffers);

	return true;
}

bool AWorldChunk::BuildMeshLODData(int32 LODLevel, FChunkMeshBuffers& OutBuffers)
{
	CurrentLODLevel = LODLevel;
	useProceduralDensityOnly = (LODLevel > 0);
	isFinalMesh = (LODLevel == 0);
	CurrentLODStep = 1 << LODLevel;
	const int32 LODStep = CurrentLODStep;
	const bool ProceduralOnly = useProceduralDensityOnly;

    if (RenderMode == EVoxelRenderMode::Cubes)
    {
        return BuildCubicMeshData(LODLevel, LODStep, ProceduralOnly, OutBuffers);
    }
    else if (RenderMode == EVoxelRenderMode::MarchingCubes)
    {
        return BuildMarchingCubeData(LODLevel, LODStep, ProceduralOnly, OutBuffers);
    }

    return false;
}

void AWorldChunk::ApplyMeshData(const FChunkMeshBuffers& Buffers)
{
	if (!Mesh) return;

	Mesh->ClearAllMeshSections();
	Mesh->CreateMeshSection(0, Buffers.Vertices, Buffers.Triangles, Buffers.Normals, Buffers.UVs, Buffers.VertexColors, {}, true);

    if (BiomeDebugMaterial)
    {
		Mesh->SetMaterial(0, BiomeDebugMaterial);
    }
}
