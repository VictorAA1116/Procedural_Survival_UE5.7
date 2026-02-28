#pragma once

#include "CoreMinimal.h"
#include "NoiseSettings.generated.h"

USTRUCT()
struct FNoiseSettings
{
	GENERATED_BODY();

	float Frequency = 0.01f;
	float Amplitude = 1.0f;
	int32 Octaves = 4;
	float Lacunarity = 2.0f;
	float Persistence = 0.5f;
};