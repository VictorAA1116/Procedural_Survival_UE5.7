// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TempModifierInterface.h"
#include "GameFramework/Volume.h"
#include "TemperatureVolume.generated.h"

UENUM()
enum class ETempBlendMode : uint8
{
	Override,
	Additive,
	Blend
};

/**
 * 
 */
UCLASS()
class PROCEDURALSURVIVAL_API ATemperatureVolume : public AVolume, public ITempModifierInterface
{
	GENERATED_BODY()
	
public:
	ATemperatureVolume();
	
	virtual float ApplyTemperature(float CurrentTemp, FVector Location) const override;
	virtual int32 GetPriority() const override { return Priority; }
	
protected:
	
	// The method that the Temperature Value will be applied with. Additive will add the Temperature Value to the ambient temp. Blend will blend between the ambient temp and the Temperature Value based on the Blend Alpha. Override will simply ignore ambient temperatures and apply the Temperature Value exactly.
	UPROPERTY(EditAnywhere, Category = "Temperature")
	ETempBlendMode BlendMode = ETempBlendMode::Override;
	
	// The Temperature Value in Kelvin that this volume applies to the player when they are inside it. The way this value is applied depends on the selected Blend Mode.
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float TemperatureValue = 0.0f;
	
	// The alpha used when blending between the Temperature Value and the ambient temp while using the "Blend" Blend Mode.
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float BlendAlpha = 0.5f;
	
	// Higher numbers = higher priority. Used to prevent conflicts with multiple volumes in the same space.
	UPROPERTY(EditAnywhere, Category = "Temperature")
	int32 Priority = 0;
	
private:
	
	
};
