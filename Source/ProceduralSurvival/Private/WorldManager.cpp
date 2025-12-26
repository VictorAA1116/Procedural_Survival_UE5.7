// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"

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

	TerrainGenerator->InitializeSeed();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldManager::StaticClass(), Found);
	if (Found.Num() > 1)
	{
		/*UE_LOG(LogTemp, Error, TEXT("Multiple WorldManager instances found! There should only be one in the level. Disabling this one to prevent duplicate chunk spawning"));*/
		SetActorTickEnabled(false);
		SetActorHiddenInGame(true);
		return;
	}

	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	// Initialize CenterChunk based on player position
	if (PlayerPawn)
	{
		/*UE_LOG(LogTemp, Warning, TEXT("PlayerPawn start pos: %s"), *PlayerPawn->GetActorLocation().ToString());*/
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

	UpdateCenterChunk();

	TArray<FIntPoint> ActiveChunkKeys;
	ActiveChunks.GetKeys(ActiveChunkKeys);

	ProcessLODUpdates(ActiveChunkKeys);

	LOD0SafetyNet(ActiveChunkKeys);

	ProcessChunkGenQueue(DeltaTime);

	ProcessLODQueue(DeltaTime);
}

void AWorldManager::UpdateCenterChunk()
{
	if (!PlayerPawn) return;

	FVector PlayerPos = PlayerPawn->GetActorLocation();
	FIntVector GV = WorldPosToGlobalVoxel(PlayerPos);
	FIntPoint NewCenterChunk;
	NewCenterChunk.X = FMath::FloorToInt((float)GV.X / ChunkSizeXY);
	NewCenterChunk.Y = FMath::FloorToInt((float)GV.Y / ChunkSizeXY);

	// Check if center chunk has changed, update it
	if (NewCenterChunk != CenterChunk)
	{
		CenterChunk = NewCenterChunk;
		UpdateChunks();
	}
}

void AWorldManager::ProcessLODUpdates(TArray<FIntPoint> ActiveChunkKeys)
{
	// Update LODs for active chunks
	for (const FIntPoint& ChunkXY : ActiveChunkKeys)
	{
		AWorldChunk* Chunk = GetChunkAt(ChunkXY);
		if (!Chunk) continue;

		const int32 DesiredLOD = ComputeLODForChunk(ChunkXY);

		if (DesiredLOD != Chunk->GetCurrentLODLevel())
		{
			const int32 OldLOD = Chunk->GetCurrentLODLevel();
			Chunk->SetCurrentLODLevel(DesiredLOD);
			RegenerateChunk(ChunkXY, OldLOD, DesiredLOD);

			if (OldLOD != DesiredLOD)
			{
				MarkChunkAndNeighborsDirty(ChunkXY);
			}

			if (DesiredLOD == 0)
			{
				if (!Chunk->AreVoxelsGenerated())
				{
					Chunk->CurrentGenPhase = EChunkGenPhase::Voxels;
				}
				else
				{
					Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
				}

				if (!Chunk->isQueuedForVoxelGen)
				{
					ChunkGenQueue.Add(ChunkXY);
					Chunk->isQueuedForVoxelGen = true;
				}
			}
			else if (DesiredLOD > 0)
			{
				EnqueueLODMeshBuild(ChunkXY, DesiredLOD);
			}
		}
	}
}

void AWorldManager::LOD0SafetyNet(TArray<FIntPoint> ActiveChunkKeys)
{
	// LOD0 safety net
	for (const FIntPoint& ChunkXY : ActiveChunkKeys)
	{
		AWorldChunk* Chunk = GetChunkAt(ChunkXY);

		if (!Chunk) continue;
		if (Chunk->GetCurrentLODLevel() != 0) continue;

		if (!Chunk->AreVoxelsGenerated())
		{
			Chunk->CurrentGenPhase = EChunkGenPhase::Voxels;
		}
		else if (!Chunk->isLOD0Built)
		{
			Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
		}
		else
		{
			continue;
		}

		if (!Chunk->isQueuedForVoxelGen)
		{
			ChunkGenQueue.Add(ChunkXY);
			Chunk->isQueuedForVoxelGen = true;
		}
	}
}

void AWorldManager::ProcessChunkGenQueue(float DeltaTime)
{
	// Process chunk generation queue
	if (ChunkGenQueue.Num() > 0)
	{
		ChunkGenAccumulator += DeltaTime * ChunkGenRate;

		int32 NumToProcess = FMath::FloorToInt(ChunkGenAccumulator);

		if (NumToProcess > 0)
		{
			ChunkGenAccumulator -= NumToProcess;

			NumToProcess = FMath::Min(NumToProcess, ChunkGenQueue.Num());

			SortChunkQueueByDistance();

			TArray<FIntPoint> Batch;
			Batch.Reserve(NumToProcess);

			for (int32 i = 0; i < NumToProcess; ++i)
			{
				Batch.Add(ChunkGenQueue[i]);
			}

			ChunkGenQueue.RemoveAt(0, NumToProcess);

			for (const FIntPoint& ChunkXY : Batch)
			{
				AWorldChunk* Chunk = GetChunkAt(ChunkXY);
				if (!Chunk)
				{
					/*UE_LOG(LogTemp, Error,
						TEXT("[ChunkGen] Missing chunk actor at %d,%d"),
						ChunkXY.X, ChunkXY.Y);*/
					continue;
				}

				Chunk->isQueuedForVoxelGen = false;

				/*UE_LOG(LogTemp, Warning,
					TEXT("[ChunkGen] POP %d,%d | LOD=%d Phase=%d Vox=%d Built=%d SeamDirty=%d"),
					ChunkXY.X, ChunkXY.Y,
					Chunk->GetCurrentLODLevel(),
					(int32)Chunk->CurrentGenPhase,
					Chunk->AreVoxelsGenerated() ? 1 : 0,
					Chunk->isLOD0Built ? 1 : 0,
					Chunk->isLOD0SeamDirty ? 1 : 0);*/

				switch (Chunk->CurrentGenPhase)
				{
					case EChunkGenPhase::Voxels:
					{
						ProcessVoxelPhase(Chunk, ChunkXY);
						break;
					}
					case EChunkGenPhase::MeshLOD0:
					{
						ProcessMeshLOD0Phase(Chunk, ChunkXY);
						break;
					}
					default:
					{
						CatchUnqueuedChunks(Chunk, ChunkXY);
						break;
					}
				}
			}
		}
	}
}

void AWorldManager::ProcessVoxelPhase(AWorldChunk* Chunk, const FIntPoint& ChunkXY)
{
	if (ActiveVoxelTasks.load() >= MaxVoxelTasks)
	{
		if (!Chunk->isQueuedForVoxelGen)
		{
			ChunkGenQueue.Add(ChunkXY);
			Chunk->isQueuedForVoxelGen = true;
		}
		return;
	}

	if (Chunk->isVoxelTaskInProgress)
	{
		if (!Chunk->isQueuedForVoxelGen)
		{
			ChunkGenQueue.Add(ChunkXY);
			Chunk->isQueuedForVoxelGen = true;
		}
		return;
	}

	/*UE_LOG(LogTemp, Warning, TEXT("[ChunkGen] -> GeneratedVoxels %d, %d (async)"), ChunkXY.X, ChunkXY.Y);*/

	Chunk->isVoxelTaskInProgress = true;

	StartAsyncVoxelGen(Chunk, ChunkXY);
}

void AWorldManager::StartAsyncVoxelGen(AWorldChunk* Chunk, const FIntPoint& ChunkXY)
{
	if (!Chunk || !TerrainGenerator)
	{
		if (Chunk)
		{
			Chunk->isVoxelTaskInProgress = false;
		}
		return;
	}

	ActiveVoxelTasks.fetch_add(1);

	const int32 SizeXY = Chunk->GetChunkSizeXY();
	const int32 HeightZ = Chunk->GetChunkHeightZ();
	const int32 BaseX = ChunkXY.X * SizeXY;
	const int32 BaseY = ChunkXY.Y * SizeXY;

	UTerrainGenerator* TerrainGen = TerrainGenerator;
	TWeakObjectPtr<AWorldChunk> WeakChunk = Chunk;
	TWeakObjectPtr<AWorldManager> WeakManager = this;

	Async(EAsyncExecution::ThreadPool, [WeakChunk, WeakManager, TerrainGen, SizeXY, HeightZ, BaseX, BaseY, ChunkXY]() mutable
	{
		const bool hasGenerator = (TerrainGen != nullptr);
		const int32 Total = SizeXY * SizeXY * HeightZ;
		TArray<FVoxel> GeneratedVoxels;
		bool isComputed = false;

		if (hasGenerator)
		{
			GeneratedVoxels.SetNumZeroed(Total);

			auto LocalIndex = [SizeXY](int X, int Y, int Z)
				{
					if (X < 0 || X >= SizeXY || Y < 0 || Y >= SizeXY || Z < 0)
					{
						return -1;
					}

					return X + Y * SizeXY + Z * SizeXY * SizeXY;
				};

			for (int x = 0; x < SizeXY; x++)
			{
				for (int y = 0; y < SizeXY; y++)
				{
					const float GX = BaseX + x;
					const float GY = BaseY + y;

					for (int z = 0; z < HeightZ; z++)
					{
						const int32 Index = LocalIndex(x, y, z);

						if (Index < 0) continue;

						const float GZ = z;
						const float Density = TerrainGen->GetDensity(GX, GY, GZ);

						FVoxel& Voxel = GeneratedVoxels[Index];
						Voxel.density = Density;
						Voxel.isSolid = (Density >= 0.0f);
					}
				}
			}

			isComputed = true;
		}
		

		AsyncTask(ENamedThreads::GameThread, [WeakChunk, WeakManager, ChunkXY, Voxels = MoveTemp(GeneratedVoxels), isComputed]() mutable
		{
			if (!WeakManager.IsValid()) return;

			AWorldManager* StrongManager = WeakManager.Get();
			StrongManager->ActiveVoxelTasks = FMath::Max(0, StrongManager->ActiveVoxelTasks - 1);

			if (!WeakChunk.IsValid()) return;

			AWorldChunk* StrongChunk = WeakChunk.Get();

			if (isComputed)
			{
				StrongChunk->ApplyGeneratedVoxels(MoveTemp(Voxels));
				StrongChunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
				StrongChunk->isVoxelTaskInProgress = false;

				StrongManager->MarkLOD0NeighborSeamDirty(ChunkXY);

				if (!StrongChunk->isQueuedForVoxelGen)
				{
					StrongManager->ChunkGenQueue.Add(ChunkXY);
					StrongChunk->isQueuedForVoxelGen = true;
				}
			}
			else
			{
				StrongChunk->isVoxelTaskInProgress = false;
			}
		});
	});

	ActiveVoxelTasks.fetch_sub(1);
}

void AWorldManager::ProcessMeshLOD0Phase(AWorldChunk* Chunk, const FIntPoint& ChunkXY)
{
	const bool needsFirstBuild = !Chunk->isLOD0Built;
	const bool needsSeamUpdate = Chunk->isLOD0SeamDirty;

	if (needsFirstBuild || needsSeamUpdate)
	{
		/*UE_LOG(LogTemp, Warning, TEXT("[ChunkGen] -> GenerateMeshLOD0 %d,%d (First=%d Seam=%d)"),
			ChunkXY.X, ChunkXY.Y, needsFirstBuild ? 1 : 0, needsSeamUpdate ? 1 : 0);*/

		const bool isBuilt = Chunk->GenerateMeshLOD(0);

		if (isBuilt)
		{
			Chunk->isLOD0Built = true;
			Chunk->isLOD0SeamDirty = false;

			/*UE_LOG(LogTemp, Warning, TEXT("[ChunkGen] -> Mesh done %d,%d (Built=%d SeamDirty=%d)"),
				ChunkXY.X, ChunkXY.Y, Chunk->isLOD0Built ? 1 : 0, Chunk->isLOD0SeamDirty ? 1 : 0);*/

			if (needsFirstBuild)
			{
				MarkLOD0NeighborSeamDirty(ChunkXY);
			}
		}
		else
		{
			Chunk->isLOD0SeamDirty = true;

			if (!Chunk->isQueuedForVoxelGen)
			{
				Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
				ChunkGenQueue.Add(ChunkXY);
				Chunk->isQueuedForVoxelGen = true;
			}
		}
	}

	Chunk->CurrentGenPhase = EChunkGenPhase::None;
	Chunk->isQueuedForVoxelGen = false;
}

void AWorldManager::CatchUnqueuedChunks(AWorldChunk* Chunk, const FIntPoint& ChunkXY)
{
	/*UE_LOG(LogTemp, Warning,
		TEXT("[ChunkGen] DEFAULT phase %d,%d"),
		ChunkXY.X, ChunkXY.Y);*/

	if (Chunk->GetCurrentLODLevel() == 0)
	{
		Chunk->CurrentGenPhase = Chunk->AreVoxelsGenerated() ? EChunkGenPhase::MeshLOD0 : EChunkGenPhase::Voxels;

		ChunkGenQueue.Add(ChunkXY);
		Chunk->isQueuedForVoxelGen = true;
	}
	else
	{
		Chunk->isQueuedForVoxelGen = false;
	}
}

void AWorldManager::ProcessLODQueue(float DeltaTime)
{
	// Process LOD mesh build queue
	if (LODQueue.Num() > 0)
	{
		LODBuildAccumulator += DeltaTime * LODBuildRate;
		int32 NumToProcess = FMath::FloorToInt(LODBuildAccumulator);

		if (NumToProcess > 0)
		{
			LODBuildAccumulator -= NumToProcess;
			NumToProcess = FMath::Min(NumToProcess, LODQueue.Num());

			SortLODQueueByDistance();

			TArray<FIntPoint> Batch;
			Batch.Reserve(NumToProcess);

			for (int32 i = 0; i < NumToProcess; ++i)
			{
				Batch.Add(LODQueue[i]);
			}

			LODQueue.RemoveAt(0, NumToProcess);

			for (const FIntPoint& ChunkXY : Batch)
			{
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

void AWorldManager::SortLODQueueByDistance()
{
	if (!PlayerPawn) return;

	FVector PlayerPos = PlayerPawn->GetActorLocation();

	const float ChunkWorldSize = ChunkSizeXY * VoxelScale;

	LODQueue.Sort([&](const FIntPoint& A, const FIntPoint& B) {
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
		/*UE_LOG(LogTemp, Warning, TEXT("WorldManager: ChunkClass not set!"));*/
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

	/*UE_LOG(LogTemp, Warning, TEXT("Active chunks: %d"), ActiveChunks.Num());*/
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
		/*UE_LOG(LogTemp, Error, TEXT("WorldManager: Invalid spawn location for chunk at {%d,%d}"), ChunkXY.X, ChunkXY.Y);*/
		return;
	}

	FActorSpawnParameters SpawnParams;
	AWorldChunk* NewChunk = GetWorld()->SpawnActor<AWorldChunk>(ChunkClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (NewChunk)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Spawning chunk at {%d,%d} world pos (%.1f, %.1f) - Active: %d"), ChunkXY.X, ChunkXY.Y, WorldX, WorldY, ActiveChunks.Num());

		ActiveChunks.Add(ChunkXY, NewChunk);
		NewChunk->SetWorldManager(this);
		NewChunk->SetRenderMode(RenderMode);
		NewChunk->InitializeChunk(ChunkSizeXY, ChunkHeightZ, VoxelScale, ChunkXY);

		const int32 DesiredLOD = ComputeLODForChunk(ChunkXY);
		NewChunk->SetCurrentLODLevel(DesiredLOD);

		if (DesiredLOD == 0)
		{
			NewChunk->CurrentGenPhase = EChunkGenPhase::Voxels;
			ChunkGenQueue.Add(ChunkXY);
			NewChunk->isQueuedForVoxelGen = true;
		}
		else
		{
			EnqueueLODMeshBuild(ChunkXY, DesiredLOD);
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

void AWorldManager::RegenerateChunk(const FIntPoint& Center, int32 OldLOD, int32 NewLOD)
{
	static const FIntPoint Offsets[5] =
	{
		FIntPoint(0, 0),
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	for (const FIntPoint& Offset : Offsets)
	{
		const FIntPoint ChunkXY = Center + Offset;

		AWorldChunk* Chunk = GetChunkAt(ChunkXY);
		if (!Chunk) continue;

		const int32 OldLODLevel = Chunk->GetCurrentLODLevel();
		const int32 DesiredLOD = ComputeLODForChunk(ChunkXY);

		if (Chunk->GetCurrentLODLevel() != DesiredLOD)
		{
			Chunk->SetCurrentLODLevel(DesiredLOD);

			if (OldLODLevel != DesiredLOD)
			{
				MarkChunkAndNeighborsDirty(ChunkXY);
			}
		}

		if (DesiredLOD == 0)
		{
			if (!Chunk->AreVoxelsGenerated())
			{
				if (!Chunk->isQueuedForVoxelGen)
				{
					Chunk->CurrentGenPhase = EChunkGenPhase::Voxels;
					ChunkGenQueue.Add(ChunkXY);
					Chunk->isQueuedForVoxelGen = true;
				}
				continue;
			}
			else if (!Chunk->isLOD0Built || Chunk->isLOD0SeamDirty)
			{
				if (!Chunk->isQueuedForVoxelGen)
				{
					Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
					ChunkGenQueue.Add(ChunkXY);
					Chunk->isQueuedForVoxelGen = true;
				}
			}
		}
		else
		{
			EnqueueLODMeshBuild(ChunkXY, DesiredLOD);
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

		if (!IsNeighborChunkLoaded(NeighborXY)) return false;

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
	TArray<FIntPoint> ActiveChunkKeys;
	ActiveChunks.GetKeys(ActiveChunkKeys);

	for (const FIntPoint& ChunkXY : ActiveChunkKeys)
	{
		AWorldChunk* Chunk = GetChunkAt(ChunkXY);
		if (!Chunk) continue;

		const int32 DesiredLOD = ComputeLODForChunk(ChunkXY);
		Chunk->SetCurrentLODLevel(DesiredLOD);

		if (DesiredLOD == 0)
		{
			if (!Chunk->AreVoxelsGenerated())
			{
				Chunk->CurrentGenPhase = EChunkGenPhase::Voxels;
			}
			else
			{
				Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
			}

			if (!Chunk->isQueuedForVoxelGen)
			{
				ChunkGenQueue.Add(ChunkXY);
				Chunk->isQueuedForVoxelGen = true;
			}
		}
		else if (DesiredLOD > 0)
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

void AWorldManager::MarkLOD0NeighborSeamDirty(const FIntPoint& Center)
{
	static const FIntPoint Neighbors[4] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	for (const FIntPoint& Offset : Neighbors)
	{
		const FIntPoint NeighborXY = Center + Offset;

		MarkLOD0Dirty(NeighborXY);
	}
}

void AWorldManager::MarkLOD0Dirty(const FIntPoint& ChunkXY)
{
	AWorldChunk* Chunk = GetChunkAt(ChunkXY);

	if (!Chunk) return;
	if (Chunk->GetCurrentLODLevel() != 0) return;
	if (!Chunk->AreVoxelsGenerated()) return;
	if (!Chunk->isLOD0Built) return;
	if (Chunk->isLOD0SeamDirty) return;

	Chunk->isLOD0SeamDirty = true;

	if (!Chunk->isQueuedForVoxelGen)
	{
		Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
		if (!ChunkGenQueue.Contains(ChunkXY))
		{
			ChunkGenQueue.Add(ChunkXY);
		}
		Chunk->isQueuedForVoxelGen = true;
	}
}

void AWorldManager::MarkChunkAndNeighborsDirty(const FIntPoint& ChunkXY)
{
	MarkLOD0Dirty(ChunkXY);
	MarkLOD0NeighborSeamDirty(ChunkXY);

	static const FIntPoint Neighbors[4] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	for (const FIntPoint& Offset : Neighbors)
	{
		const FIntPoint NeighborXY = ChunkXY + Offset;
		AWorldChunk* Neighbor = GetChunkAt(NeighborXY);
		if (!Neighbor) continue;

		const int32 CurrentLOD = Neighbor->GetCurrentLODLevel();
		if (CurrentLOD > 0)
		{
			EnqueueLODMeshBuild(NeighborXY, CurrentLOD);
		}
	}
}

bool AWorldManager::RemoveVoxel(const FVector& VoxelWorldLocation)
{
	FIntVector GlobalCoords = WorldPosToGlobalVoxel(VoxelWorldLocation);

	FIntPoint ChunkXY;
	FIntVector LocalXYZ;
	GlobalVoxelToChunkCoords(GlobalCoords.X, GlobalCoords.Y, GlobalCoords.Z, ChunkXY, LocalXYZ);

	AWorldChunk* Chunk = GetChunkAt(ChunkXY);

	if (!Chunk) return false;
	if (!Chunk->AreVoxelsGenerated()) return false;

	Chunk->SetVoxelLocal(LocalXYZ.X, LocalXYZ.Y, LocalXYZ.Z, false);
	Chunk->isLOD0SeamDirty = true;
	
	MarkLOD0NeighborSeamDirty(ChunkXY);

	if (!Chunk->isQueuedForVoxelGen)
	{
		Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
		ChunkGenQueue.Add(ChunkXY);
		Chunk->isQueuedForVoxelGen = true;
	}

	return true;
}

bool AWorldManager::AddVoxel(const FVector& VoxelWorldLocation)
{
	FIntVector GlobalCoords = WorldPosToGlobalVoxel(VoxelWorldLocation);

	FIntPoint ChunkXY;
	FIntVector LocalXYZ;
	GlobalVoxelToChunkCoords(GlobalCoords.X, GlobalCoords.Y, GlobalCoords.Z, ChunkXY, LocalXYZ);

	AWorldChunk* Chunk = GetChunkAt(ChunkXY);

	if (!Chunk) return false;
	if (!Chunk->AreVoxelsGenerated()) return false;

	Chunk->SetVoxelLocal(LocalXYZ.X, LocalXYZ.Y, LocalXYZ.Z, true);
	Chunk->isLOD0SeamDirty = true;

	MarkLOD0NeighborSeamDirty(ChunkXY);

	if (!Chunk->isQueuedForVoxelGen)
	{
		Chunk->CurrentGenPhase = EChunkGenPhase::MeshLOD0;
		ChunkGenQueue.Add(ChunkXY);
		Chunk->isQueuedForVoxelGen = true;
	}

	return true;
}
