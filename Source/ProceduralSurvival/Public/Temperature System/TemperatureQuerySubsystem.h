// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TemperatureQuerySubsystem.generated.h"

class UTemperatureSubsystem;
class ITempModifierInterface;

/**
 * 
 */
UCLASS()
class PROCEDURALSURVIVAL_API UTemperatureQuerySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	float GetTemperatureAtLocation(FVector Location) const;
	
protected:
	UPROPERTY()
	UTemperatureSubsystem* TemperatureSubsystem;
	
	void GatherModifiersAtLocation(FVector Location, TArray<AActor*>& OutActors) const;
	
private:
	
	
};
