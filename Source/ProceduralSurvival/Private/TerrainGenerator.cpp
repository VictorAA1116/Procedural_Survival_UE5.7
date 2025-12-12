// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainGenerator.h"

FBiomeWeights UTerrainGenerator::GetBiomeWeights(float X, float Y) const
{
	float bx = X / BiomeScale;
	float by = Y / BiomeScale;

	float warp = FMath::PerlinNoise2D(FVector2D(bx * 0.5f, by * 0.5f)) * 0.15f;

	bx += warp;
	by += warp;
	
	float v = FMath::PerlinNoise2D(FVector2D(bx, by));

	float t = (v + 1.0f) * 0.5f;

	float PlainsEdge = 0.33f;
	float MountainsEdge = 0.66f;

	FBiomeWeights Weights;

	Weights.Plains = 1.0 - FMath::SmoothStep(0.25f, PlainsEdge, t);

	Weights.Hills = FMath::SmoothStep(0.30f, PlainsEdge, t) * (1.0f - FMath::SmoothStep(MountainsEdge, 0.75f, t));

	Weights.Mountains = FMath::SmoothStep(MountainsEdge, 0.85f, t);

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
	float Height = FMath::PerlinNoise2D(FVector2D(X, Y) * 0.005f);
	return Height * 3.0f + 20.0f;
}

float UTerrainGenerator::GetHillsHeight(int X, int Y) const
{
	float nx = X * HillsFrequency;
	float ny = Y * HillsFrequency;

	float n = 
		0.6f * FMath::PerlinNoise2D(FVector2D(nx, ny)) +
		0.3f * FMath::PerlinNoise2D(FVector2D(2 * nx, 2 * ny)) +
		0.1f * FMath::PerlinNoise2D(FVector2D(4 * nx, 4 * ny));

	return n * HillsAmplitude + 25.0f;
}

float UTerrainGenerator::GetMountainsHeight(int X, int Y) const
{
	float nx = X * MountainsFrequency;
	float ny = Y * MountainsFrequency;

	float r = 1.0f - FMath::Abs(FMath::PerlinNoise2D(FVector2D(nx, ny)));

	r = r * r;

	float r2 = 1.0f - FMath::Abs(FMath::PerlinNoise2D(FVector2D(2 * nx, 2 * ny)));
	r2 = r2 * r2 * 0.5f;

	float height = (r + r2) * MountainsAmplitude + 40.0f;

	return height;
}

float UTerrainGenerator::ApplyRivers(float X, float Y, float Height) const
{
	if (!EnableRivers) return Height;

	float RiverValue = FMath::Abs(FMath::PerlinNoise2D(FVector2D(X, Y) * RiverFrequency));

	if (RiverValue < RiverWidth)
	{
		float t = 1.0f - (RiverValue / RiverWidth);

		t = t * t;
		t = FMath::SmoothStep(0.0f, 1.0f, t);

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
	float continents = FMath::PerlinNoise2D(FVector2D(X, Y) * ContinentFrequency);
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

	float surfaceNoise = FMath::PerlinNoise2D(FVector2D(X, Y) * 0.1f) * SurfaceNoiseAmplitude;
	Height += surfaceNoise;
	
	return Height;
}

float UTerrainGenerator::GetDensity(float X, float Y, float Z) const
{
	float Height = GetTerrainHeight(X, Y);
	return Height - Z;
}

EBiomeType UTerrainGenerator::GetBiomeAt(float X, float Y) const
{
	FBiomeWeights Weights = GetBiomeWeights(X, Y);
	EBiomeType Biome1;
	EBiomeType Biome2;
	float Blend;
	PickDominantBiomes(Weights, Biome1, Biome2, Blend);
	return Biome2;
}

