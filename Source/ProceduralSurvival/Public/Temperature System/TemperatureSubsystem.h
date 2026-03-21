// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TemperatureSubsystem.generated.h"

class ATimeOfDayManager;

UCLASS()
class PROCEDURALSURVIVAL_API UTemperatureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// Kelvin
	float GetGlobalTempKelvin() const;
	
	// Fahrenheit 
	float GetGlobalTempFahrenheit() const;
	
	// Celsius
	float GetGlobalTempCelsius() const;
	
	void SetTimeOfDayManager(ATimeOfDayManager* InTODManager);
	
	void SetTempFromTOD();
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Temperature")
	UCurveFloat* TemperatureOverTimeOfDay;

private:
	UPROPERTY()
	ATimeOfDayManager* TimeOfDayManager;
	
	float TemperatureK;
};
