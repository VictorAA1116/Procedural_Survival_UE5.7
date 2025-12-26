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

bool AWorldChunk::GenerateCubicMesh()
{
    if (Mesh)
    {
        Mesh->ClearAllMeshSections();
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;

    const int EstimatedFaces = ChunkSizeXY * ChunkSizeXY * ChunkHeightZ;
    Vertices.Reserve(EstimatedFaces * 6 * 4);
    Triangles.Reserve(EstimatedFaces * 6 * 6);
    Normals.Reserve(EstimatedFaces * 6 * 4);
    UVs.Reserve(EstimatedFaces * 6 * 4);

    const int Step = CurrentLODStep;
	const int ScaledVoxel = VoxelScale * Step;

    for (int x = 0; x < ChunkSizeXY; x += Step)
    {
        for (int y = 0; y < ChunkSizeXY; y += Step)
        {
            const int DepthSteps = 1;
			int32 SurfaceZ = -1;

            auto SampleSolidAt = [&](int LX, int LY, int LZ) -> bool
            {
                int gx2 = ChunkCoords.X * ChunkSizeXY + LX;
                int gy2 = ChunkCoords.Y * ChunkSizeXY + LY;
                
				const float Density = (CurrentLODLevel == 0 && AreVoxelsGenerated()) 
                    ? GetVoxelDensity({ LX, LY, LZ }) 
                    : WorldManager->TerrainGenerator->GetDensity(gx2, gy2, LZ);

				return (Density >= 0.0f);
            };

            for (int z2 = ChunkHeightZ - Step; z2 >= 0; z2 -= Step)
            {
				const bool isSolid = SampleSolidAt(x, y, z2);
				const bool isAirAbove = (z2 + Step >= ChunkHeightZ) ? true : !SampleSolidAt(x, y, z2 + Step);

                if (isSolid && isAirAbove)
                {
                    SurfaceZ = z2;
                    break;
				}
			}

            const int32 MinSeamZ = (SurfaceZ < 0) ? INT32_MAX : FMath::Max(0, SurfaceZ - DepthSteps * Step);

            for (int z = 0; z < ChunkHeightZ; z += Step)
            {
				const int gx = ChunkCoords.X * ChunkSizeXY + x;
				const int gy = ChunkCoords.Y * ChunkSizeXY + y;

				const float Density = (CurrentLODLevel == 0 && AreVoxelsGenerated())? GetVoxelDensity({x, y, z}) : WorldManager->TerrainGenerator->GetDensity(gx, gy, z);

				if (Density < 0.0f) continue; // Empty space

                FVector BasePos = FVector(
                    x * VoxelScale,
                    y * VoxelScale,
                    z * VoxelScale
                );

                auto IsOnChunkBorder = [&](int x, int y)
                {
                    return (x == 0 || x + Step >= ChunkSizeXY || y == 0 || y + Step >= ChunkSizeXY);
                };

                auto IsNeighborDifferentLOD = [&](int dx, int dy) -> bool
                {
					FIntPoint NeighborXY = ChunkCoords + FIntPoint(dx, dy);
                    
					AWorldChunk* Neighbor = WorldManager->GetChunkAt(NeighborXY);
					if (!Neighbor) return false;

                    int NeighborLOD = Neighbor->GetCurrentLODLevel();
					return (NeighborLOD != CurrentLODLevel);
				};

                auto NeighborSolid = [&](int NX, int NY, int NZ) -> bool
                {
                    int GlobalX = ChunkCoords.X * ChunkSizeXY + NX;
                    int GlobalY = ChunkCoords.Y * ChunkSizeXY + NY;
                    int GlobalZ = NZ;

                    if (!WorldManager) return true;

                    if (useProceduralDensityOnly)
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

                if (x + Step >= ChunkSizeXY)
                {
                    if (IsNeighborDifferentLOD(1, 0) && isNearSurface)
                    {
                        AddCubeFace(0, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Right
					}
                    else if (!NeighborSolid(x + Step, y, z))
                    {
                        AddCubeFace(0, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Right
                    }
                }
                else if (!NeighborSolid(x + Step, y, z))
                {
                    AddCubeFace(0, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Right
				}

                if (x - Step < 0)
                {
                    if (IsNeighborDifferentLOD(-1, 0) && isNearSurface)
                    {
                        AddCubeFace(1, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Left
                    }
                    else if (!NeighborSolid(x - Step, y, z))
                    {
                        AddCubeFace(1, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Left
                    }
                }
                else if (!NeighborSolid(x - Step, y, z))
                {
                    AddCubeFace(1, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Left
				}

                if (y + Step >= ChunkSizeXY)
                {
                    if (IsNeighborDifferentLOD(0, 1) && isNearSurface)
                    {
                        AddCubeFace(2, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Front
                    }
                    else if (!NeighborSolid(x, y + Step, z))
                    {
                        AddCubeFace(2, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Front
                    }
                }
                else if (!NeighborSolid(x, y + Step, z))
                {
                    AddCubeFace(2, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Front
                }

                if (y - Step < 0)
                {
                    if (IsNeighborDifferentLOD(0, -1) && isNearSurface)
                    {
                        AddCubeFace(3, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Back
                    }
                    else if (!NeighborSolid(x, y - Step, z))
                    {
                        AddCubeFace(3, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Back
                    }
                }
                else if (!NeighborSolid(x, y - Step, z))
                {
                    AddCubeFace(3, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Back
                }

                if (!NeighborSolid(x, y, z + Step)) AddCubeFace(4, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Top

                if (!ShouldCullBottomFace(x, y, z))
                {
                    if (!NeighborSolid(x, y, z - Step)) AddCubeFace(5, BasePos, ScaledVoxel, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Bottom
                }
            }
        }
    }

    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, {}, true);

    isLOD0Built = true;

    if (BiomeDebugMaterial)
    {
        Mesh->SetMaterial(0, BiomeDebugMaterial);
    }

    return true;
}

bool AWorldChunk::GenerateMarchingCubesMesh()
{
    if (CurrentLODLevel == 0 && (!WorldManager || !WorldManager->AreAllNeighborChunksVoxelReady(ChunkCoords)))
    {
        return false;
    }

    const float IsoLevel = 0.0f;

    if (Mesh)
    {
        Mesh->ClearAllMeshSections();
    }

    TArray<FVector> GradientCache;
    GradientCache.SetNumZeroed(ChunkSizeXY * ChunkSizeXY * ChunkHeightZ);
    ComputeGradient(GradientCache);

    TArray<FVector> Vertices; Vertices.Reset();
    TArray<int32> Triangles; Triangles.Reset();
    TArray<FVector> Normals; Normals.Reset();
	TArray<FVector2D> UVs; UVs.Reset();
	TArray<FColor> VertexColors; VertexColors.Reset();

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
    Vertices.Reserve(EstimatedCells * 2);
    Triangles.Reserve(EstimatedCells * 5);
	Normals.Reserve(EstimatedCells * 2);
    NormalAcc.Reserve(EstimatedCells * 2);
    UVs.Reserve(EstimatedCells * 2);

    const int Step = CurrentLODStep;

    for (int x = 0; x < ChunkSizeXY; x += Step)
    {
        for (int y = 0; y < ChunkSizeXY; y += Step)
        {
            for (int z = 0; z < ChunkHeightZ; z += Step)
            {
                int gx = ChunkCoords.X * ChunkSizeXY + x;
                int gy = ChunkCoords.Y * ChunkSizeXY + y;
                int gz = z;

                float val[8];
                FVector pos[8];

                pos[0] = FVector(x, y, z) * VoxelScale;
                pos[1] = FVector(x + Step, y, z) * VoxelScale;
                pos[2] = FVector(x + Step, y + Step, z) * VoxelScale;
                pos[3] = FVector(x, y + Step, z) * VoxelScale;
                pos[4] = FVector(x, y, z + Step) * VoxelScale;
                pos[5] = FVector(x + Step, y, z + Step) * VoxelScale;
                pos[6] = FVector(x + Step, y + Step, z + Step) * VoxelScale;
                pos[7] = FVector(x, y + Step, z + Step) * VoxelScale;

                val[0] = SampleDensityForMarching(gx, gy, gz);
                val[1] = SampleDensityForMarching(gx + Step, gy, gz);
                val[2] = SampleDensityForMarching(gx + Step, gy + Step, gz);
                val[3] = SampleDensityForMarching(gx, gy + Step, gz);
                val[4] = SampleDensityForMarching(gx, gy, gz + Step);
                val[5] = SampleDensityForMarching(gx + Step, gy, gz + Step);
                val[6] = SampleDensityForMarching(gx + Step, gy + Step, gz + Step);
                val[7] = SampleDensityForMarching(gx, gy + Step, gz + Step);

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
                            const int32 NewIndex = Vertices.Add(Vertex);
                            VertexIndexMap.Add(Key, NewIndex);

							NormalAcc.Add(FVector::ZeroVector);
							UVs.Add(FVector2D(Vertex.X / 1000.0f, Vertex.Y / 1000.0f));

                            const EBiomeType Biome = WorldManager->TerrainGenerator->GetDominantBiome(gx, gy);
                            
                            VertexColors.Add(
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

                    Triangles.Add(i0);
                    Triangles.Add(i1);
					Triangles.Add(i2);

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
                            SampleDensityForMarching(gx + 1, gy, gz) -
                            SampleDensityForMarching(gx - 1, gy, gz);

                        const float dy =
                            SampleDensityForMarching(gx, gy + 1, gz) -
                            SampleDensityForMarching(gx, gy - 1, gz);

                        const float dz =
                            SampleDensityForMarching(gx, gy, gz + 1) -
                            SampleDensityForMarching(gx, gy, gz - 1);

                        return -FVector(dx, dy, dz).GetSafeNormal();
                    };

                    NormalAcc[i0] += SampleNormal(v0);
                    NormalAcc[i1] += SampleNormal(v1);
                    NormalAcc[i2] += SampleNormal(v2);
                }
            }
        }
    }

    Vertices.Reserve(Vertices.Num() + 1024);
    Normals.Reserve(Normals.Num() + 1024);
    UVs.Reserve(UVs.Num() + 1024);
    VertexColors.Reserve(VertexColors.Num() + 1024);
    Triangles.Reserve(Triangles.Num() + 2048);


    const float SkirtDepth = VoxelScale * CurrentLODStep * 2.0f;

    auto AddSkirtQuad = [&](const FVector& A, const FVector& B)
    {
        int32 Start = Vertices.Num();

        FVector A2 = A - FVector(0, 0, SkirtDepth);
        FVector B2 = B - FVector(0, 0, SkirtDepth);

        Vertices.Add(A);
        Vertices.Add(B);
        Vertices.Add(B2);
        Vertices.Add(A2);

        FVector Normal = FVector::CrossProduct(B - A, A2 - A).GetSafeNormal();
        for (int i = 0; i < 4; ++i)
        {
            Normals.Add(Normal);
            UVs.Add(FVector2D(0.0f, 0.0f));
            VertexColors.Add(FColor::Black);
        }

        Triangles.Append({
            Start + 0, Start + 1, Start + 2,
            Start + 0, Start + 2, Start + 3
        });
    };

	Normals.Init(FVector::ZeroVector, Vertices.Num());

    for (int32 i = 0; i < NormalAcc.Num(); ++i)
    {
        FVector Normal = NormalAcc[i].GetSafeNormal();

        if (!Normal.IsNearlyZero())
        {
            Normals[i] = Normal;
        }
        else
        {
            Normals[i] = FVector::UpVector;
		}
	}

    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, {}, true);

	const int32 VertexCount = Vertices.Num();

    for (int i = 0; i < VertexCount; ++i)
    {
		const FVector V = Vertices[i];
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

    if (BiomeDebugMaterial)
    {
		Mesh->SetMaterial(0, BiomeDebugMaterial);
    }

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

float AWorldChunk::SampleDensityForMarching(int GlobalX, int GlobalY, int GlobalZ) const
{
    if (!WorldManager || !WorldManager->TerrainGenerator) return 1.0f;
	if (GlobalZ < 0) return 1.0f;
	if (GlobalZ >= ChunkHeightZ) return -1.0f;

    FIntPoint ChunkXY;
    FIntVector LocalXYZ;
    WorldManager->GlobalVoxelToChunkCoords(GlobalX, GlobalY, GlobalZ, ChunkXY, LocalXYZ);

    if (!useProceduralDensityOnly)
    {
        if (AWorldChunk* Neighbor = WorldManager->GetChunkAt(ChunkXY))
        {
            if (Neighbor->AreVoxelsGenerated() && LocalXYZ.X >= 0 && LocalXYZ.X < Neighbor->GetChunkSizeXY() && LocalXYZ.Y >= 0 && LocalXYZ.Y < Neighbor->GetChunkSizeXY() && LocalXYZ.Z >= 0 && LocalXYZ.Z < Neighbor->GetChunkHeightZ())
            {
                return Neighbor->GetVoxelDensity(LocalXYZ);
            }
        }
    }

    if (!useProceduralDensityOnly && (!WorldManager->IsChunkWithinRenderDistance(ChunkXY) || !WorldManager->IsNeighborChunkLoaded(ChunkXY)))
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

                const float DX = (float)SampleDensityForMarching(GX + EPS, GY, GZ) - (float)SampleDensityForMarching(GX - EPS, GY, GZ);
                const float DY = (float)SampleDensityForMarching(GX, GY + EPS, GZ) - (float)SampleDensityForMarching(GX, GY - EPS, GZ);
                const float DZ = (float)SampleDensityForMarching(GX, GY, GZ + EPS) - (float)SampleDensityForMarching(GX, GY, GZ - EPS);

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
	CurrentLODLevel = LODLevel;

    useProceduralDensityOnly = (LODLevel > 0);

	isFinalMesh = (LODLevel == 0);

	CurrentLODStep = 1 << LODLevel;

    if (RenderMode == EVoxelRenderMode::Cubes)
    {
        return GenerateCubicMesh();
    }
    else if (RenderMode == EVoxelRenderMode::MarchingCubes)
    {
        return GenerateMarchingCubesMesh();
	}

	return false;
}