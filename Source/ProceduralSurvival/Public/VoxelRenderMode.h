#pragma once

#include "CoreMinimal.h"

// Enum for voxel render modes
UENUM(BlueprintType)
enum class EVoxelRenderMode : uint8
{
	Cubes	UMETA(DisplayName = "Cubes"),
	MarchingCubes UMETA(DisplayName = "Smooth"),
	// Add other render modes as neededs
};
