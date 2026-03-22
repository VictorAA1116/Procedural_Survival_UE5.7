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

	if (!TemperatureManager) return;
	
	FVector Location = GetOwner()->GetActorLocation();
	float EnvironmentTemp = TemperatureManager->GetTemperatureAtLocation(Location);
	
	float EffectiveTemp = CalculateEffectiveTemperature(EnvironmentTemp);
	
	BodyTemperature = FMath::FInterpTo(BodyTemperature, EffectiveTemp, DeltaTime, HeatTransferRate);
}

float UTemperatureComponent::CalculateEffectiveTemperature(float EnvironmentTemp) const
{
	return FMath::Lerp(EnvironmentTemp, BodyTemperature, Insulation);
}

