// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/TemperatureComponent.h"
#include "Temperature System/TemperatureManager.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UTemperatureComponent::UTemperatureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTemperatureComponent::BeginPlay()
{
	Super::BeginPlay();

	TemperatureManager = Cast<ATemperatureManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATemperatureManager::StaticClass()));
}


// Called every frame
void UTemperatureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	float InitialBodyTemp = BodyTemperature;
	
	float EnvironmentTemp = GetAmbientTemperatureKelvin();
	
	float EffectiveTemp = CalculateEffectiveTemperature(EnvironmentTemp);
	
	BodyTemperature = FMath::FInterpTo(BodyTemperature, EffectiveTemp, DeltaTime, HeatTransferRate);

	if (InitialBodyTemp < BodyTemperature - 0.1f || InitialBodyTemp > BodyTemperature + 0.1f)
	{
		OnTemperatureChanged.Broadcast();
	}
	
	if (BodyTemperature <= ColdDamageThresholdK && !bIsTemperatureExtreme)
	{
		OnExtremeCold.Broadcast();
		bIsTemperatureExtreme = true;
	}
	else if (BodyTemperature >= HeatDamageThresholdK && !bIsTemperatureExtreme)
	{
		OnExtremeHeat.Broadcast();
		bIsTemperatureExtreme = true;
	}
	else if (BodyTemperature > ColdDamageThresholdK && BodyTemperature < HeatDamageThresholdK && bIsTemperatureExtreme)
	{
		OnTemperatureNormalized.Broadcast();
		bIsTemperatureExtreme = false;
	}
}

float UTemperatureComponent::GetAmbientTemperatureKelvin() const
{
	if (!TemperatureManager) return 0.0f;
	
	return TemperatureManager->GetTemperatureAtLocation(GetOwner()->GetActorLocation());
}

float UTemperatureComponent::GetAmbientTemperatureCelsius() const
{
	return GetAmbientTemperatureKelvin() - 273.15f;
}

float UTemperatureComponent::GetAmbientTemperatureFahrenheit() const
{
	return GetAmbientTemperatureCelsius() * 9.0f / 5.0f + 32.0f;
}

float UTemperatureComponent::CalculateEffectiveTemperature(float EnvironmentTemp) const
{
	return FMath::Lerp(EnvironmentTemp, BodyTemperature, Insulation);
}

