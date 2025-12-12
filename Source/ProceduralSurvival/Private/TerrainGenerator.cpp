// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainGenerator.h"

EBiomeType UTerrainGenerator::GetBiomeAt(float X, float Y) const
{
	float BiomeValue = FMath::PerlinNoise2D(FVector2D(X, Y) * BiomeFrequency);

	if (BiomeValue < -0.2f)
	{
		return EBiomeType::Plains;
	}
	else if (BiomeValue < 0.3f)
	{
		return EBiomeType::Hills;
	}
	else
	{
		return EBiomeType::Mountains;
	}
}

float UTerrainGenerator::GetPlainsHeight(int X, int Y) const
{
	float Height = FMath::PerlinNoise2D(FVector2D(X, Y) * 0.005f);
	return Height * 3.0f + 20.0f;
}

float UTerrainGenerator::GetHillsHeight(int X, int Y) const
{
	float Height = FMath::PerlinNoise2D(FVector2D(X, Y) * HillsFrequency);
	return Height * HillsAmplitude + 25.0f;
}

float UTerrainGenerator::GetMountainsHeight(int X, int Y) const
{
	float Height = FMath::Abs(FMath::PerlinNoise2D(FVector2D(X, Y) * MountainsFrequency));
	Height = Height * Height;
	return Height * MountainsAmplitude + 40.0f;
}

float UTerrainGenerator::ApplyRivers(float X, float Y, float Height) const
{
	if (!EnableRivers) return Height;

	float RiverValue = FMath::Abs(FMath::PerlinNoise2D(FVector2D(X, Y) * RiverFrequency));

	if (RiverValue < RiverWidth)
	{
		float DepthFactor = (RiverWidth - RiverValue) / RiverWidth;
		Height -= DepthFactor * RiverDepth;
	}

	return Height;
}

float UTerrainGenerator::GetTerrainHeight(float X, float Y) const
{
	float continents = FMath::PerlinNoise2D(FVector2D(X, Y) * ContinentFrequency);
	continents = continents * ContinentAmplitude + ContinentBaseHeight;

	EBiomeType Biome = GetBiomeAt(X, Y);

	float Height = continents;

	switch (Biome)
	{
		case EBiomeType::Plains:
			Height += GetPlainsHeight(X, Y);
			break;
		case EBiomeType::Hills:
			Height += GetHillsHeight(X, Y);
			break;
		case EBiomeType::Mountains:
			Height += GetMountainsHeight(X, Y);
			break;
	}

	Height = ApplyRivers(X, Y, Height);
	
	return Height;
}

float UTerrainGenerator::GetDensity(float X, float Y, float Z) const
{
	float Height = GetTerrainHeight(X, Y);
	return Height - Z;
}

