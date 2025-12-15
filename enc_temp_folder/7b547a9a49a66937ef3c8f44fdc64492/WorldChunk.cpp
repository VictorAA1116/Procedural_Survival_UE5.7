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

    GenerateMesh();
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

void AWorldChunk::AddCubeFace(int FaceIndex, FVector& Position, FColor FaceColor, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& VertexColors)
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
	TArray<FColor> VertexColors;

    const int EstimatedFaces = ChunkSizeXY * ChunkSizeXY * ChunkHeightZ;
    Vertices.Reserve(EstimatedFaces * 6 * 4);
    Triangles.Reserve(EstimatedFaces * 6 * 6);
    Normals.Reserve(EstimatedFaces * 6 * 4);
    UVs.Reserve(EstimatedFaces * 6 * 4);


    for (int x = 0; x < ChunkSizeXY; x++)
    {
        for (int y = 0; y < ChunkSizeXY; y++)
        {
            for (int z = 0; z < ChunkHeightZ; z++)
            {
                if (!IsVoxelSolidLocal(x, y, z)) continue;

                FVector BasePos = FVector(
                    x * VoxelScale,
                    y * VoxelScale,
                    z * VoxelScale
                );

                auto NeighborSolid = [&](int NX, int NY, int NZ) -> bool
                {
                    int GlobalX = ChunkCoords.X * ChunkSizeXY + NX;
                    int GlobalY = ChunkCoords.Y * ChunkSizeXY + NY;
                    int GlobalZ = NZ;

                    if (!WorldManager) return true;

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

                int gx = ChunkCoords.X * ChunkSizeXY + x;
                int gy = ChunkCoords.Y * ChunkSizeXY + y;

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
                if (!SampleDensityForCubic(x + 1, y, z)) AddCubeFace(0, BasePos, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Right
                if (!SampleDensityForCubic(x - 1, y, z)) AddCubeFace(1, BasePos, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Left
                if (!SampleDensityForCubic(x, y + 1, z)) AddCubeFace(2, BasePos, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Front
                if (!SampleDensityForCubic(x, y - 1, z)) AddCubeFace(3, BasePos, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Back
                if (!SampleDensityForCubic(x, y, z + 1)) AddCubeFace(4, BasePos, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Top

                if (!ShouldCullBottomFace(x, y, z))
                {
                    if (!SampleDensityForCubic(x, y, z - 1)) AddCubeFace(5, BasePos, BiomeColor, Vertices, Triangles, Normals, UVs, VertexColors); // Bottom
                }
            }
        }
    }

    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, {}, true);

    if (BiomeDebugMaterial)
    {
        Mesh->SetMaterial(0, BiomeDebugMaterial);
    }
}

void AWorldChunk::GenerateMarchingCubesMesh()
{
    if (!WorldManager || !WorldManager->AreAllNeighborChunksVoxelReady(ChunkCoords))
    {
        return;
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


    for (int x = 0; x < ChunkSizeXY; x++)
    {
        for (int y = 0; y < ChunkSizeXY; y++)
        {
            for (int z = 0; z < ChunkHeightZ; z++)
            {
                int gx = ChunkCoords.X * ChunkSizeXY + x;
                int gy = ChunkCoords.Y * ChunkSizeXY + y;
                int gz = z;

                float val[8];
                FVector pos[8];

                pos[0] = FVector(x, y, z) * VoxelScale;
                pos[1] = FVector(x + 1, y, z) * VoxelScale;
                pos[2] = FVector(x + 1, y + 1, z) * VoxelScale;
                pos[3] = FVector(x, y + 1, z) * VoxelScale;
                pos[4] = FVector(x, y, z + 1) * VoxelScale;
                pos[5] = FVector(x + 1, y, z + 1) * VoxelScale;
                pos[6] = FVector(x + 1, y + 1, z + 1) * VoxelScale;
                pos[7] = FVector(x, y + 1, z + 1) * VoxelScale;

                val[0] = SampleDensityForMarching(gx, gy, gz);
                val[1] = SampleDensityForMarching(gx + 1, gy, gz);
                val[2] = SampleDensityForMarching(gx + 1, gy + 1, gz);
                val[3] = SampleDensityForMarching(gx, gy + 1, gz);
                val[4] = SampleDensityForMarching(gx, gy, gz + 1);
                val[5] = SampleDensityForMarching(gx + 1, gy, gz + 1);
                val[6] = SampleDensityForMarching(gx + 1, gy + 1, gz + 1);
                val[7] = SampleDensityForMarching(gx, gy + 1, gz + 1);

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

    if (BiomeDebugMaterial)
    {
		Mesh->SetMaterial(0, BiomeDebugMaterial);
    }
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

float AWorldChunk::SampleDensityForCubic(int GlobalX, int GlobalY, int GlobalZ) const
{
    if (!WorldManager || !WorldManager->TerrainGenerator)
    {
        return 1.0f;
	}

	FIntPoint ChunkXY;
    FIntVector LocalXYZ;
	WorldManager->GlobalVoxelToChunkCoords(GlobalX, GlobalY, GlobalZ, ChunkXY, LocalXYZ);

	AWorldChunk* Neighbor = WorldManager->GetChunkAt(ChunkXY);
    if (!Neighbor || !Neighbor->AreVoxelsGenerated())
    {
        return 1.0f;
    }

    if (!WorldManager->IsChunkWithinRenderDistance(ChunkXY) || !WorldManager->IsNeighborChunkLoaded(ChunkXY) || LocalXYZ.Z < 0)
    {
        return 1.0f;
	}

    if (LocalXYZ.Z >= ChunkHeightZ) return -1.0f;

    return Neighbor->GetVoxelDensity(LocalXYZ);
}

float AWorldChunk::SampleDensityForMarching(int GlobalX, int GlobalY, int GlobalZ) const
{
    if (!WorldManager || !WorldManager->TerrainGenerator) return 1.0f;
	if (GlobalZ < 0) return 1.0f;
	if (GlobalZ >= ChunkHeightZ) return -1.0f;

    FIntPoint ChunkXY;
    FIntVector LocalXYZ;
    WorldManager->GlobalVoxelToChunkCoords(GlobalX, GlobalY, GlobalZ, ChunkXY, LocalXYZ);

    if (AWorldChunk* Neighbor = WorldManager->GetChunkAt(ChunkXY))
    {
        if (Neighbor->AreVoxelsGenerated() && LocalXYZ.X >= 0 && LocalXYZ.X < Neighbor->GetChunkSizeXY() && LocalXYZ.Y >= 0 && LocalXYZ.Y < Neighbor->GetChunkSizeXY() && LocalXYZ.Z >= 0 && LocalXYZ.Z < Neighbor->GetChunkHeightZ())
        {
            return Neighbor->GetVoxelDensity(LocalXYZ);;
        }
    }

    if (!WorldManager->IsChunkWithinRenderDistance(ChunkXY) || !WorldManager->IsNeighborChunkLoaded(ChunkXY))
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
