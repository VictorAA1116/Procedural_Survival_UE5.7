// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TempModifierInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable)
class UTempModifierInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROCEDURALSURVIVAL_API ITempModifierInterface
{
	GENERATED_BODY()

public:
	virtual float ApplyTemperature(float CurrentTemp, FVector Location) const = 0;
	virtual int32 GetPriority() const = 0;
};
