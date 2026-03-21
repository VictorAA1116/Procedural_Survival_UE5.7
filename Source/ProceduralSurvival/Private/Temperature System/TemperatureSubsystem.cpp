// Fill out your copyright notice in the Description page of Project Settings.

#include "Temperature System/TemperatureSubsystem.h"

#include "TimeOfDayManager.h"

void UTemperatureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTemperatureSubsystem::SetTimeOfDayManager(ATimeOfDayManager* InTODManager)
{
	TimeOfDayManager = InTODManager;
}

void UTemperatureSubsystem::SetTempFromTOD()
{
	if (!TimeOfDayManager || !TemperatureOverTimeOfDay)
	{
		TemperatureK = 0.0f;
		return;
	}
	
	const float TimeNormalized = TimeOfDayManager->GetTimeNormalized();
	
	TemperatureK = TemperatureOverTimeOfDay->GetFloatValue(TimeNormalized);
}

// Getter for global Kelvin Temp
float UTemperatureSubsystem::GetGlobalTempKelvin() const
{
	return TemperatureK;
}

// Convert internal Kelvin Temp to Celsius
float UTemperatureSubsystem::GetGlobalTempCelsius() const
{
	const float TempCelsius = TemperatureK - 273.15f;
	
	return TempCelsius;
}

// Convert internal Kelvin Temp to Fahrenheit
float UTemperatureSubsystem::GetGlobalTempFahrenheit() const
{
	const float TempFahrenheit = (TemperatureK - 273.15f) * 9.0f / 5.0f + 32.0f;
	
	return TempFahrenheit;
}
