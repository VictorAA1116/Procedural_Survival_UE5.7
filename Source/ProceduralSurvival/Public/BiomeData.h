#pragma once

#include "CoreMinimal.h"
#include "BiomeData.Generated.h"

USTRUCT()
struct FBiomeData
{
	GENERATED_BODY();

	EBiomeType BiomeType;
	float Weight;
};

USTRUCT()
struct FBiomeWeights
{
	GENERATED_BODY();

	float Plains = 0.0f;
	float Hills = 0.0f;
	float Mountains = 0.0f;
};

UENUM(BlueprintType)
enum class EBiomeType : uint8
{
	Plains UMETA(DisplayName = "Plains"),
	Hills UMETA(DisplayName = "Hills"),
	Mountains UMETA(DisplayName = "Mountain")
};