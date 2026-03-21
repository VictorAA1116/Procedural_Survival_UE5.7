// Fill out your copyright notice in the Description page of Project Settings.

#include "Temperature System/TemperatureVolume.h"

ATemperatureVolume::ATemperatureVolume()
{
	bColored = true;
}

float ATemperatureVolume::ApplyTemperature(float CurrentTemp, FVector Location) const
{
	switch (BlendMode)
	{
		case ETempBlendMode::Override:
			return TemperatureValue;
		
		case ETempBlendMode::Additive:
			return CurrentTemp + TemperatureValue;
		
		case ETempBlendMode::Blend:
			return FMath::Lerp(CurrentTemp, TemperatureValue, BlendAlpha);
		
		default:
			return CurrentTemp;
	}
}
