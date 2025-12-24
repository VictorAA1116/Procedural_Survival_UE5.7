// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TerrainGenerator.generated.h"

UENUM(BlueprintType)
enum class EBiomeType : uint8
{
	Plains UMETA(DisplayName = "Plains"),
	Hills UMETA(DisplayName = "Hills"),
	Mountains UMETA(DisplayName = "Mountain")
};

USTRUCT()
struct FBiomeWeights
{
	GENERATED_BODY()

	float Plains = 0.0f;
	float Hills = 0.0f;
	float Mountains = 0.0f;
};

UCLASS(Blueprintable, BlueprintType)
class PROCEDURALSURVIVAL_API UTerrainGenerator : public UObject
{
	GENERATED_BODY()
	
public:	
	// Surface noise to add small variations to the terrain surface
	UPROPERTY(EditAnywhere, Category = "Terrain | Surface Noise")
	float SurfaceNoiseAmplitude = 2.0f;
	
	// Noise frequency for continent shaping
	UPROPERTY(EditAnywhere, Category = "Terrain | Continents")
	float ContinentFrequency = 0.001f;

	// Amplitude for height variation of continent surfaces
	UPROPERTY(EditAnywhere, Category = "Terrain | Continents")
	float ContinentAmplitude = 20.0f;

	// Base height offset for continents
	UPROPERTY(EditAnywhere, Category = "Terrain | Continents")
	float ContinentBaseHeight = 30.0f;

	// How wide a biome is in voxels
	UPROPERTY(EditAnywhere, Category = "Terrain | Biomes")
	float BiomeScale = 2000.0f;

	// Frequency for plains biome generation (how often plains biomes spawn)
	UPROPERTY(EditAnywhere, Category = "Terrain | Plains")
	float PlainsFrequency = 0.006f;

	// Amplitude for plains biome height variations
	UPROPERTY(EditAnywhere, Category = "Terrain | Plains")
	float PlainsAmplitude = 15.0f;

	// Base height for plains biome
	UPROPERTY(EditAnywhere, Category = "Terrain | Plains")
	float PlainsBaseHeight = 10.0f;

	// Frequency for hill biome generation (how often hill biomes spawn)
	UPROPERTY(EditAnywhere, Category = "Terrain | Hills")
	float HillsFrequency = 0.008f;

	// Amplitude for hill biome height variations
	UPROPERTY(EditAnywhere, Category = "Terrain | Hills")
	float HillsAmplitude = 25.0f;

	// Frequency for mountain biome generation (how often mountain biomes spawn)
	UPROPERTY(EditAnywhere, Category = "Terrain | Mountains")
	float MountainsFrequency = 0.005f;

	// Amplitude for mountain biome height variations
	UPROPERTY(EditAnywhere, Category = "Terrain | Mountains")
	float MountainsAmplitude = 45.0f;

	// Boolean to enable or disable river generation
	UPROPERTY(EditAnywhere, Category = "Terrain | Rivers")
	bool EnableRivers = false;

	// Frequency of rivers in the terrain noise
	UPROPERTY(EditAnywhere, Category = "Terrain | Rivers")
	float RiverFrequency = 0.002f;

	// Width of rivers
	UPROPERTY(EditAnywhere, Category = "Terrain | Rivers")
	float RiverWidth = 0.1f;

	// Depth of rivers below the terrain surface in voxels
	UPROPERTY(EditAnywhere, Category = "Terrain | Rivers")
	float RiverDepth = 15.0f;

	float GetTerrainHeight(float X, float Y) const;
	float GetDensity(float X, float Y, float Z) const;
	FBiomeWeights GetBiomeWeights(float X, float Y) const;
	EBiomeType GetDominantBiome(float X, float Y) const;
	void InitializeSeed();

protected:
	

private:	
	UPROPERTY(EditAnywhere, Category = "Terrain | Seed")
	bool UseRandomSeed = false;

	UPROPERTY(EditAnywhere, Category = "Terrain | Seed")
	int Seed;

	FORCEINLINE float Hash2D(int32 X, int32 Y, int32 Salt) const;
	FORCEINLINE FVector2D SeededCoords(float X, float Y, int32 Salt) const;
	float GetPlainsHeight(int X, int Y) const;
	float GetHillsHeight(int X, int Y) const;
	float GetMountainsHeight(int X, int Y) const;
	float ApplyRivers(float X, float Y, float Height) const;
	void PickDominantBiomes(const FBiomeWeights& Weights, EBiomeType& OutBiome1, EBiomeType& OutBiome2, float& OutBlend) const;
};
