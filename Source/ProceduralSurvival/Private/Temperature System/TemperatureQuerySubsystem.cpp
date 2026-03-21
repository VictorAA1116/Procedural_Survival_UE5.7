// Fill out your copyright notice in the Description page of Project Settings.

#include "Temperature System/TemperatureQuerySubsystem.h"
#include "Temperature System/TemperatureSubsystem.h"
#include "Temperature System/TempModifierInterface.h"
#include "Kismet/GameplayStatics.h"

void UTemperatureQuerySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	TemperatureSubsystem = GetWorld()->GetSubsystem<UTemperatureSubsystem>();
}

float UTemperatureQuerySubsystem::GetTemperatureAtLocation(FVector Location) const
{
	if (!TemperatureSubsystem) return 0.0f;
	
	TemperatureSubsystem->SetTempFromTOD();
	float Temperature = TemperatureSubsystem->GetGlobalTempKelvin();
	
	TArray<AActor*> Actors;
	GatherModifiersAtLocation(Location, Actors);
	
	// Sort by priority
	Actors.Sort([](AActor& A, AActor& B)
	{
		const ITempModifierInterface* TempModA = Cast<ITempModifierInterface>(&A);	
		const ITempModifierInterface* TempModB = Cast<ITempModifierInterface>(&B);
		
		if (!TempModA || !TempModB) return false;
		
		return TempModA && TempModB && TempModA->GetPriority() > TempModB->GetPriority();
	});
	
	for (AActor* Actor : Actors)
	{
		if (const ITempModifierInterface* Modifier = Cast<ITempModifierInterface>(Actor))
		{
			Temperature = Modifier->ApplyTemperature(Temperature, Location);
		}
	}
	
	return Temperature;
}

void UTemperatureQuerySubsystem::GatherModifiersAtLocation(FVector Location, TArray<AActor*>& OutActors) const
{
	if (!TemperatureSubsystem) return;
	
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UTempModifierInterface::StaticClass(), AllActors);
	
	for (AActor* Actor : AllActors)
	{
		if (!Actor) continue;
		
		// Check if the actor is a volume
		if (AVolume* Volume = Cast<AVolume>(Actor))
		{
			if (Volume->EncompassesPoint(Location))
			{
				OutActors.Add(Actor);
			}
		}
		else
		{
			const float Distance = FVector::Dist(Location, Actor->GetActorLocation());
			
			const float MaxRange = 500.0f;
			
			if (Distance <= MaxRange)
			{
				OutActors.Add(Actor);
			}
		}
	}
}
