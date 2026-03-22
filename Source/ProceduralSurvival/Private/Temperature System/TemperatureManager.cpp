// Fill out your copyright notice in the Description page of Project Settings.


#include "Temperature System/TemperatureManager.h"

#include "TimeOfDayManager.h"

// Sets default values
ATemperatureManager::ATemperatureManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ATemperatureManager::BeginPlay()
{
	Super::BeginPlay();
}

void ATemperatureManager::SetGlobalTemperature() const
{
	if (!TemperatureOverTimeOfDay || !TimeOfDayManager)
	{
		GlobalTemperatureKelvin = 0.0f;
		return;
	}
	
	const float Time = TimeOfDayManager->GetTimeNormalized();
	
	GlobalTemperatureKelvin = TemperatureOverTimeOfDay->GetFloatValue(Time);
}

float ATemperatureManager::GetTemperatureAtLocation(FVector Location) const
{
	SetGlobalTemperature();
	float Temperature = GlobalTemperatureKelvin;
	
	for (UTempModifierComponent* Modifier : Modifiers)
	{
		if (!Modifier) continue;
		
		if (Modifier->IsPointIside(Location))
		{
			Temperature = Modifier->ApplyTemperature(Temperature);
		}
	}
	
	return Temperature;
}

void ATemperatureManager::RegisterModifier(UTempModifierComponent* Modifier)
{
	Modifiers.AddUnique(Modifier);
}

void ATemperatureManager::UnregisterModifier(UTempModifierComponent* Modifier)
{
	Modifiers.Remove(Modifier);
}