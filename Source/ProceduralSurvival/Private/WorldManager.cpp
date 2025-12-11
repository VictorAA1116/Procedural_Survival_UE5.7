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
		CenterChunk.X = FMath::FloorToInt((float)GV.X / ChunkSize);
		CenterChunk.Y = FMath::FloorToInt((float)GV.Y / ChunkSize);
	}

	UpdateChunks();
}

// Called every frame
void AWorldManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PlayerPawn) return;

	FVector PlayerPos = PlayerPawn->GetActorLocation();
	FIntVector GV = WorldPosToGlobalVoxel(PlayerPos);
	FIntPoint NewCenterChunk;
	NewCenterChunk.X = FMath::FloorToInt((float)GV.X / ChunkSize);
	NewCenterChunk.Y = FMath::FloorToInt((float)GV.Y / ChunkSize);

	if (NewCenterChunk != CenterChunk)
	{
		CenterChunk = NewCenterChunk;
		UpdateChunks();
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
				FIntPoint ChunkXY = ChunkGenQueue{ 0 };
				ChunkGenQueue.RemoveAt(0);
				AWorldChunk** ChunkPtr = ActiveChunks.Find(ChunkXY);

				if (ChunkPtr && *ChunkPtr)
				{
					(*ChunkPtr)->GenerateVoxels();
					(*ChunkPtr)->GenerateMesh();
					OnChunkCreated(ChunkXY);
				}
			}
		}
	}
}

void AWorldManager::SortChunkQueueByDistance()
{
	if (!PlayerPawn) return;

	FVector PlayerPos = PlayerPawn->GetActorLocation();

	const float ChunkWorldSize = ChunkSize * VoxelScale;

	ChunkGenQueue.Sort({ & }(const FIntPoint & A, const FIntPoint & B) {
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
	int ChunkX = FMath::FloorToInt((float)GlobalX / ChunkSize);
	int ChunkY = FMath::FloorToInt((float)GlobalY / ChunkSize);

	int LocalX = GlobalX - ChunkX * ChunkSize;
	int LocalY = GlobalY - ChunkY * ChunkSize;
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

	if (!ChunkPtr || !(*ChunkPtr)) return false;

	const AWorldChunk* Chunk = *ChunkPtr;

	if (LocalXYZ.Z < 0 || LocalXYZ.Z >= Chunk->GetChunkSize()) return false;

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

			if (!ActiveChunks.Contains(ChunkXY))
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

	float ChunkWorldSize = ChunkSize * VoxelScale;
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
		NewChunk->InitializeChunk(ChunkSize, VoxelScale, ChunkXY);
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
	static const FIntPoint Neighbors{ 4 } = {
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
			(*NeighborPtr)->GenerateMesh();
		}
	}
}
