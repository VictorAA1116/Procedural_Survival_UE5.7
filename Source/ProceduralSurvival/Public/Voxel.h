#pragma once

#include "CoreMinimal.h"
#include "Voxel.generated.h"

USTRUCT(BlueprintType)
struct FVoxel
{
    GENERATED_BODY()

    UPROPERTY()
    bool isSolid = false;

    UPROPERTY()
    float density = 1.0f;

    UPROPERTY()
    uint8 materialID = 0;
};