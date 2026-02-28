// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BiomeData.h"
#include "NoiseSettings.h"
#include "UObject/Object.h"
#include "PlanetDefinition.generated.h"

UCLASS(BlueprintType)
class PROCEDURALSURVIVAL_API UPlanetDefinition : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	int32 Seed;

	UPROPERTY(EditAnywhere)
	float PlanetRadius = 100000.0f;

	UPROPERTY(EditAnywhere)
	float HeightScale = 2000.0f;

	UPROPERTY(EditAnywhere)
	FNoiseSettings TerrainNoise;

	UPROPERTY(EditAnywhere)
	FNoiseSettings BiomeNoise;

	UPROPERTY(EditAnywhere)
	FNoiseSettings CaveNoise;

	float GetHeight(FVector2D WorldXZ) const;

	float GetBaseDensity(FVector WorldPos) const;

	float GetFinalDensity(FVector WorldPos) const;

	FBiomeData GetBiome(FVector2D WorldXZ) const;

private:


protected:


};
