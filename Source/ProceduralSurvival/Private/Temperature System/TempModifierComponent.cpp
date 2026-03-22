// Fill out your copyright notice in the Description page of Project Settings.


#include "Temperature System/TempModifierComponent.h"

#include "StateTreeTypes.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Temperature System/TemperatureManager.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UTempModifierComponent::UTempModifierComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UTempModifierComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find Temperature Manager
	TemperatureManager = Cast<ATemperatureManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATemperatureManager::StaticClass()));
	
	// Register self as a modifier with the Temperature Manager
	if (TemperatureManager)
	{
		TemperatureManager->RegisterModifier(this);
	}
	
	if (!BoxComponent)
	{
		BoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
		if (!BoxComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("No BoxComponent found"));
		}
	}
}

void UTempModifierComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister self on EndPlay
	if (TemperatureManager)
	{
		TemperatureManager->UnregisterModifier(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

bool UTempModifierComponent::IsPointInside(FVector Point) const
{
	if (!BoxComponent) return false;
	
	return BoxComponent->Bounds.GetBox().IsInside(Point);
}

float UTempModifierComponent::ApplyTemperature(float CurrentTemp) const
{
	switch (TemperatureBlendMode)
	{
		case ETempBlendMode::Override:
			return TemperatureValue;
		
		case ETempBlendMode::Additive:
			return CurrentTemp + TemperatureValue;
		
		case ETempBlendMode::Blend:
			return FMath::Lerp(CurrentTemp, TemperatureValue, BlendAlpha);
	}
	
	return CurrentTemp;
}