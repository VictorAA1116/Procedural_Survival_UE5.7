// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AWorldManager::AWorldManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TerrainGenerator = CreateDefaultSubobject<UTerrainGenerator>(TEXT("TerrainGenerator"));
}

// Called when the game starts or when spawned
void AWorldManager::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldManager::StaticClass(), Found);
	if (Found.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Multiple WorldManager instances found! There should only be one in the level. Disabling this one to prevent duplicate chunk spawning"));
		SetActorTickEnabled(false);
		SetActorHiddenInGame(true);
		return;
	}

	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	// Initialize CenterChunk based on player position
	if (PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerPawn start pos: %s"), *PlayerPawn->GetActorLocation().ToString());
		FVector PlayerPos = PlayerPawn->GetActorLocation();
		FIntVector GV = WorldPosToGlobalVoxel(PlayerPos);
		CenterChunk.X = FMath::FloorToInt((float)GV.X / ChunkSizeXY);
		CenterChunk.Y = FMath::FloorToInt((float)GV.Y / ChunkSizeXY);
	}

	UpdateChunks();
	EnqueueInitialLODs();
}

// Called every frame
void AWorldManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PlayerPawn) return;

	FVector PlayerPos = PlayerPawn->GetActorLocation();
	FIntVector GV = WorldPosToGlobalVoxel(PlayerPos);
	FIntPoint NewCenterChunk;
	NewCenterChunk.X = FMath::FloorToInt((float)GV.X / ChunkSizeXY);
	NewCenterChunk.Y = FMath::FloorToInt((float)GV.Y / ChunkSizeXY);

	if (NewCenterChunk != CenterChunk)
	{
		CenterChunk = NewCenterChunk;
		UpdateChunks();
	}

	for (auto& Pair : ActiveChunks)
	{
		AWorldChunk* Chunk = Pair.Value;
		if (!Chunk) continue;

		const int32 DesiredLOD = ComputeLODForChunk(Pair.Key);

		if (DesiredLOD != Chunk->GetCurrentLODLevel())
		{
			Chunk->SetCurrentLODLevel(DesiredLOD);
			if (!Chunk->isQueuedForVoxelGen)
			{
				Chunk->isQueuedForVoxelGen = true;
				ChunkGenQueue.Add(Pair.Key);
			}
		}
	}

	if (LODQueue.Num() > 0)
	{
		LODBuildAccumulator += DeltaTime * LODBuildRate;
		int32 NumToProcess = FMath::FloorToInt(LODBuildAccumulator);

		if (NumToProcess > 0)
		{
			LODBuildAccumulator -= NumToProcess;
			NumToProcess = FMath::Min(NumToProcess, LODQueue.Num());

			for (int32 i = 0; i < NumToProcess; ++i)
			{
				const FIntPoint ChunkXY = LODQueue[0];
				LODQueue.RemoveAt(0);

				AWorldChunk* Chunk = GetChunkAt(ChunkXY);
				if (!Chunk) continue;

				int32* DesiredLODPtr = PendingLOD.Find(ChunkXY);
				if (!DesiredLODPtr) continue;

				const int32 LOD = *DesiredLODPtr;
				PendingLOD.Remove(ChunkXY);

				if (LOD > 0)
				{
					Chunk->GenerateMeshLOD(LOD);
				}
			}
		}
	}

	if (ChunkGenQueue.Num() > 0)
	{

		ChunkGenAccumulator += DeltaTime * ChunkGenRate;

		int32 NumToProcess = FMath::FloorToInt(ChunkGenAccumulator);

		if (NumToProcess > 0)
		{
			ChunkGenAccumulator -= NumToProcess;

			NumToProcess = FMath::Min(NumToProcess, ChunkGenQueue.Num());

			SortChunkQueueByDistance();

			for (int32 i = 0; i < NumToProcess; ++i)
			{
				FIntPoint ChunkXY = ChunkGenQueue[0];
				ChunkGenQueue.RemoveAt(0);
				AWorldChunk** ChunkPtr = ActiveChunks.Find(ChunkXY);

				if (ChunkPtr && *ChunkPtr)
				{
					(*ChunkPtr)->GenerateVoxels();
					(*ChunkPtr)->isQueuedForVoxelGen = false;
					
					if ((*ChunkPtr)->GetCurrentLODLevel() == 0)
					{
						(*ChunkPtr)->GenerateMeshLOD(0);
						OnChunkCreated(ChunkXY);
					}
					else
					{
						int DesiredLOD = ComputeLODForChunk(ChunkXY);
						(*ChunkPtr)->GenerateMeshLOD(DesiredLOD);
					}
				}
			}
		}
	}
}

void AWorldManager::SortChunkQueueByDistance()
{
	if (!PlayerPawn) return;

	FVector PlayerPos = PlayerPawn->GetActorLocation();

	const float ChunkWorldSize = ChunkSizeXY * VoxelScale;

	ChunkGenQueue.Sort([&](const FIntPoint& A, const FIntPoint& B) {
		FVector PosA = FVector(A.X * ChunkWorldSize, A.Y * ChunkWorldSize, 0.0f);
		FVector PosB = FVector(B.X * ChunkWorldSize, B.Y * ChunkWorldSize, 0.0f);
		return FVector::DistSquared(PosA, PlayerPos) < FVector::DistSquared(PosB, PlayerPos);
	});
}

FIntVector AWorldManager::WorldPosToGlobalVoxel(const FVector& WorldPos) const
{
	// WorldPos is in centimeters, convert to voxel indices
	FIntVector GV;
	GV.X = FMath::FloorToInt(WorldPos.X / VoxelScale);
	GV.Y = FMath::FloorToInt(WorldPos.Y / VoxelScale);
	GV.Z = FMath::FloorToInt(WorldPos.Z / VoxelScale);
	return GV;
}

void AWorldManager::GlobalVoxelToChunkCoords(int GlobalX, int GlobalY, int GlobalZ, FIntPoint& OutChunkXY, FIntVector& OutLocalXYZ) const
{
	int ChunkX = FMath::FloorToInt((float)GlobalX / ChunkSizeXY);
	int ChunkY = FMath::FloorToInt((float)GlobalY / ChunkSizeXY);

	int LocalX = GlobalX - ChunkX * ChunkSizeXY;
	int LocalY = GlobalY - ChunkY * ChunkSizeXY;
	int LocalZ = GlobalZ;

	OutChunkXY = FIntPoint(ChunkX, ChunkY);
	OutLocalXYZ = FIntVector(LocalX, LocalY, LocalZ);
}

bool AWorldManager::IsVoxelSolidGlobal(int GlobalVoxelX, int GlobalVoxelY, int GlobalVoxelZ) const
{
	FIntPoint ChunkXY;
	FIntVector LocalXYZ;
	GlobalVoxelToChunkCoords(GlobalVoxelX, GlobalVoxelY, GlobalVoxelZ, ChunkXY, LocalXYZ);

	AWorldChunk* const* ChunkPtr = ActiveChunks.Find(ChunkXY);

	if (!ChunkPtr || !(*ChunkPtr)) return true;

	const AWorldChunk* Chunk = *ChunkPtr;

	if (!Chunk->AreVoxelsGenerated()) return true;

	if (LocalXYZ.Z < 0 || LocalXYZ.Z >= Chunk->GetChunkHeightZ()) return true;

	return Chunk->IsVoxelSolidLocal(LocalXYZ.X, LocalXYZ.Y, LocalXYZ.Z);
}

void AWorldManager::UpdateChunks()
{
	if (!ChunkClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldManager: ChunkClass not set!"));
		return;
	}

	// Determine which chunks should be active
	TSet<FIntPoint> DesiredChunks;
	TArray<FIntPoint> ChunksToSpawn;
	ChunksToSpawn.Empty();

	for (int DX = -RenderDistance; DX <= RenderDistance; ++DX)
	{
		for (int DY = -RenderDistance; DY <= RenderDistance; ++DY)
		{
			FIntPoint ChunkXY = FIntPoint(CenterChunk.X + DX, CenterChunk.Y + DY);
			DesiredChunks.Add(ChunkXY);

			if (!ActiveChunks.Find(ChunkXY))
			{
				RegisterChunkAt(ChunkXY);
				ChunkGenQueue.Add(ChunkXY);
			}
		}
	}

	// Destroy chunks that are no longer needed
	TArray<FIntPoint> ChunksToRemove;

	for (auto& Pair : ActiveChunks)
	{
		if (!DesiredChunks.Contains(Pair.Key))
		{
			ChunksToRemove.Add(Pair.Key);
		}
	}

	for (const FIntPoint& ChunkXY : ChunksToRemove)
	{
		DestroyChunkAt(ChunkXY);
	}

	UE_LOG(LogTemp, Warning, TEXT("Active chunks: %d"), ActiveChunks.Num());
}

void AWorldManager::RegisterChunkAt(const FIntPoint& ChunkXY)
{
	if (!GetWorld() || !ChunkClass) return;

	float ChunkWorldSize = ChunkSizeXY * VoxelScale;
	float WorldX = ChunkXY.X * ChunkWorldSize;
	float WorldY = ChunkXY.Y * ChunkWorldSize;

	FVector SpawnLocation = FVector(WorldX, WorldY, 0.0f);

	if (!FMath::IsFinite(WorldX) || !FMath::IsFinite(WorldY) || FMath::Abs(WorldX) > 1e6f || FMath::Abs(WorldY) > 1e6f)
	{
		UE_LOG(LogTemp, Error, TEXT("WorldManager: Invalid spawn location for chunk at {%d,%d}"), ChunkXY.X, ChunkXY.Y);
		return;
	}

	FActorSpawnParameters SpawnParams;
	AWorldChunk* NewChunk = GetWorld()->SpawnActor<AWorldChunk>(ChunkClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (NewChunk)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawning chunk at {%d,%d} world pos (%.1f, %.1f) - Active: %d"), ChunkXY.X, ChunkXY.Y, WorldX, WorldY, ActiveChunks.Num());

		ActiveChunks.Add(ChunkXY, NewChunk);
		NewChunk->SetWorldManager(this);
		NewChunk->SetRenderMode(RenderMode);
		NewChunk->InitializeChunk(ChunkSizeXY, ChunkHeightZ, VoxelScale, ChunkXY);

		if (NewChunk->GetCurrentLODLevel() == 0)
		{
			NewChunk->isQueuedForVoxelGen = true;
			ChunkGenQueue.Add(ChunkXY);
		}
	}
}

void AWorldManager::DestroyChunkAt(const FIntPoint& ChunkXY)
{
	AWorldChunk** Found = ActiveChunks.Find(ChunkXY);

	if (!Found) return;

	AWorldChunk* Chunk = *Found;
	if (Chunk)
	{
		Chunk->Destroy();
	}

	ActiveChunks.Remove(ChunkXY);
}

bool AWorldManager::IsChunkWithinRenderDistance(const FIntPoint& ChunkXY) const
{
	const int DX = FMath::Abs(ChunkXY.X - CenterChunk.X);
	const int DY = FMath::Abs(ChunkXY.Y - CenterChunk.Y);
	return (DX <= RenderDistance && DY <= RenderDistance);
}

void AWorldManager::OnChunkCreated(const FIntPoint& ChunkXY)
{
	static const FIntPoint Neighbors[4] = {
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	for (const FIntPoint& Offset : Neighbors)
	{
		FIntPoint NeighborXY = ChunkXY + Offset;
		AWorldChunk** NeighborPtr = ActiveChunks.Find(NeighborXY);
		if (NeighborPtr && *NeighborPtr)
		{
			if ((*NeighborPtr)->GetCurrentLODLevel() == 0 && (*NeighborPtr)->AreVoxelsGenerated())
			{
				(*NeighborPtr)->GenerateMeshLOD(0);
			}
		}
	}
}

bool AWorldManager::IsNeighborChunkLoaded(const FIntPoint& NChunkXY) const
{
	return ActiveChunks.Contains(NChunkXY);
}

bool AWorldManager::AreAllNeighborChunksVoxelReady(const FIntPoint& ChunkXY) const
{
	static const FIntPoint Neighbors[4] = {
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};
	for (const FIntPoint& Offset : Neighbors)
	{
		const FIntPoint NeighborXY = ChunkXY + Offset;
		AWorldChunk* const* NeighborPtr = ActiveChunks.Find(NeighborXY);
		if (!NeighborPtr || !(*NeighborPtr) || !(*NeighborPtr)->AreVoxelsGenerated())
		{
			return false;
		}
	}
	return true;
}

AWorldChunk* AWorldManager::GetChunkAt(const FIntPoint& ChunkXY) const
{
	AWorldChunk* const* ChunkPtr = ActiveChunks.Find(ChunkXY);
	if (ChunkPtr)
	{
		return *ChunkPtr;
	}
	return nullptr;
}

int32 AWorldManager::ComputeLODForChunk(const FIntPoint& ChunkXY) const
{
	const FIntPoint Center = CenterChunk;

	const int32 Dist = FMath::Max(FMath::Abs(ChunkXY.X - Center.X), FMath::Abs(ChunkXY.Y - Center.Y));

	int32 LOD = 0;
	int32 Threshold = LOD0RenderDistance;

	while (Dist > Threshold && LOD < MaxLODLevel)
	{
		Threshold *= LODStepMultiplier;
		LOD++;
	}

	return FMath::Clamp(LOD, 0, MaxLODLevel);
}

void AWorldManager::EnqueueInitialLODs()
{
	for (auto& Pair : ActiveChunks)
	{
		const FIntPoint ChunkXY = Pair.Key;
		AWorldChunk* Chunk = Pair.Value;
		if (!Chunk) continue;

		const int32 DesiredLOD = ComputeLODForChunk(ChunkXY);
		Chunk->SetCurrentLODLevel(DesiredLOD);

		if (DesiredLOD == 0)
		{
			if (!Chunk->AreVoxelsGenerated() && !Chunk->isQueuedForVoxelGen)
			{
				Chunk->isQueuedForVoxelGen = true;
				ChunkGenQueue.Add(ChunkXY);
			}
		}
		else
		{
			EnqueueLODMeshBuild(ChunkXY, DesiredLOD);
		}
	}
}

void AWorldManager::EnqueueLODMeshBuild(const FIntPoint& ChunkXY, int32 LOD)
{
	if (!ActiveChunks.Contains(ChunkXY)) return;

	PendingLOD.Add(ChunkXY, LOD);

	if (!LODQueue.Contains(ChunkXY))
	{
		LODQueue.Add(ChunkXY);
	}
}
