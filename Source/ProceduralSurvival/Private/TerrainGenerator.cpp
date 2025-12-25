// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainGenerator.h"

static constexpr int32 SEED_CONTINENTS = 1337;
static constexpr int32 SEED_BIOMES = 7331;
static constexpr int32 SEED_PLAINS = 9001;
static constexpr int32 SEED_HILLS = 4242;
static constexpr int32 SEED_MOUNTAINS = 6666;
static constexpr int32 SEED_RIVERS = 12345;
static constexpr int32 SEED_SURFACE = 8888;

FBiomeWeights UTerrainGenerator::GetBiomeWeights(float X, float Y) const
{
	const FVector2D P = SeededCoords(X / BiomeScale, Y / BiomeScale, Seed + SEED_BIOMES);

	float warp = FMath::PerlinNoise2D(P * 0.5f) * 0.15f;

	const FVector2D Pw = FVector2D(P.X + warp, P.Y + warp);
	
	float v = FMath::PerlinNoise2D(Pw);

	float t = (v + 1.0f) * 0.5f;

	FBiomeWeights Weights;

	Weights.Plains = 1.0f - FMath::SmoothStep(PlainsEdge - BlendWidth, PlainsEdge + BlendWidth, t);
	Weights.Plains *= 1.1f;

	Weights.Mountains = FMath::SmoothStep(MountainsEdge - BlendWidth, MountainsEdge + BlendWidth, t);

	Weights.Hills = 1.0f - Weights.Plains - Weights.Mountains;
	Weights.Hills = FMath::Clamp(Weights.Hills, 0.0f, 1.0f);

	float sum = Weights.Plains + Weights.Hills + Weights.Mountains;

	if (sum > 0.0f)
	{
		Weights.Plains /= sum;
		Weights.Hills /= sum;
		Weights.Mountains /= sum;
	}

	return Weights;
}

float UTerrainGenerator::GetPlainsHeight(int X, int Y) const
{
	const FVector2D P = SeededCoords(X * PlainsFrequency, Y * PlainsFrequency, Seed + SEED_PLAINS);

	float n = 
		0.7f * FMath::PerlinNoise2D(P) +
		0.2f * FMath::PerlinNoise2D(P * 2.0f);

	return n * PlainsAmplitude + PlainsBaseHeight;
}

float UTerrainGenerator::GetHillsHeight(int X, int Y) const
{
	const FVector2D P = SeededCoords(X * HillsFrequency, Y * HillsFrequency, Seed + SEED_HILLS);

	float n = 
		0.6f * FMath::PerlinNoise2D(P) +
		0.3f * FMath::PerlinNoise2D(P * 2.0f) +
		0.1f * FMath::PerlinNoise2D(P * 4.0f);

	return n * HillsAmplitude + HillsBaseHeight;
}

float UTerrainGenerator::GetMountainsHeight(int X, int Y) const
{
	const FVector2D P = SeededCoords(X * MountainsFrequency, Y * MountainsFrequency, Seed + SEED_MOUNTAINS);

	float r = 1.0f - FMath::Abs(FMath::PerlinNoise2D(P));

	r *= r;

	float r2 = 1.0f - FMath::Abs(FMath::PerlinNoise2D(P * 3.0f));
	r2 = r2 * r2 * 0.5f;

	float height = (r + r2) * MountainsAmplitude + MountainsBaseHeight;

	return height;
}

float UTerrainGenerator::ApplyRivers(float X, float Y, float Height) const
{
	if (!EnableRivers) return Height;

	const FVector2D P = SeededCoords(X * RiverFrequency, Y * RiverFrequency, Seed + SEED_RIVERS);

	float RiverValue = FMath::Abs(FMath::PerlinNoise2D(P));

	if (RiverValue < RiverWidth)
	{
		float t = 1.0f - (RiverValue / RiverWidth);

		t = FMath::SmoothStep(0.0f, 1.0f, t * t);

		Height -= t * RiverDepth;
	}

	return Height;
}

void UTerrainGenerator::PickDominantBiomes(const FBiomeWeights& Weights, EBiomeType& OutBiome1, EBiomeType& OutBiome2, float& OutBlend) const
{
	struct FBiomeWeightPair
	{
		EBiomeType Biome;
		float Weight;
	};

	TArray<FBiomeWeightPair> BiomeWeightPairs;
	BiomeWeightPairs.Add({ EBiomeType::Plains, Weights.Plains });
	BiomeWeightPairs.Add({ EBiomeType::Hills, Weights.Hills });
	BiomeWeightPairs.Add({ EBiomeType::Mountains, Weights.Mountains });

	BiomeWeightPairs.Sort([](const FBiomeWeightPair& A, const FBiomeWeightPair& B)
	{
		return A.Weight > B.Weight;
	});

	OutBiome1 = BiomeWeightPairs[0].Biome;
	OutBiome2 = BiomeWeightPairs[1].Biome;
	OutBlend = BiomeWeightPairs[1].Weight;
}

float UTerrainGenerator::GetTerrainHeight(float X, float Y) const
{
	const FVector2D P = SeededCoords(X * ContinentFrequency, Y * ContinentFrequency, Seed + SEED_CONTINENTS);

	float continents = FMath::PerlinNoise2D(P);
	continents = continents * ContinentAmplitude + ContinentBaseHeight;

	FBiomeWeights Weight = GetBiomeWeights(X, Y);

	EBiomeType PrimaryBiome;
	EBiomeType SecondaryBiome;

	float Blend;
	PickDominantBiomes(Weight, PrimaryBiome, SecondaryBiome, Blend);

	auto getBiomeHeight = [&](EBiomeType Biome) -> float
	{
		switch (Biome)
		{
			case EBiomeType::Plains:
				return GetPlainsHeight(X, Y);
			case EBiomeType::Hills:
				return GetHillsHeight(X, Y);
			case EBiomeType::Mountains:
				return GetMountainsHeight(X, Y);
			default:
				return 0.0f;
		}
	};

	float h0 = getBiomeHeight(PrimaryBiome);
	float h1 = getBiomeHeight(SecondaryBiome);

	float biomeHeight = FMath::Lerp(h0, h1, Blend);

	float Height = continents + biomeHeight;

	Height = ApplyRivers(X, Y, Height);

	float surfaceNoise = FMath::PerlinNoise2D(SeededCoords(X * 0.1f, Y * 0.1f, Seed + SEED_SURFACE)) * SurfaceNoiseAmplitude;
	Height += surfaceNoise;
	
	return Height;
}

float UTerrainGenerator::GetDensity(float X, float Y, float Z) const
{
	float Height = GetTerrainHeight(X, Y);
	return Height - Z;
}

EBiomeType UTerrainGenerator::GetDominantBiome(float X, float Y) const
{
	const FBiomeWeights Weights = GetBiomeWeights(X, Y);

	EBiomeType PrimaryBiome;
	EBiomeType SecondaryBiome;

	float blend = 0.0f;
	PickDominantBiomes(Weights, PrimaryBiome, SecondaryBiome, blend);
	return PrimaryBiome;
}

void UTerrainGenerator::InitializeSeed()
{
	if (UseRandomSeed)
	{
		Seed = FMath::Rand();
	}
}

FORCEINLINE uint32 UTerrainGenerator::Hash1D(int32 V) const
{
	uint32 X = (uint32)V;
	X ^= X >> 16;
	X *= 0x7feb352d;
	X ^= X >> 15;
	X *= 0x846ca68b;
	X ^= X >> 16;
	return X;
}

FORCEINLINE FVector2D UTerrainGenerator::SeededCoords(float X, float Y, int32 Salt) const
{
	const uint32 H1 = Hash1D(Salt);
	const uint32 H2 = Hash1D(Salt ^ 0x9E3779B9);

	constexpr float OffsetScale = 512.0f;

	const float Ox = ((H1 / (float)UINT32_MAX) * 2.0f - 1.0f) * OffsetScale;
	const float Oy = ((H2 / (float)UINT32_MAX) * 2.0f - 1.0f) * OffsetScale;

	return FVector2D(X + Ox, Y + Oy);
}