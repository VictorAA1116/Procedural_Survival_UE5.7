// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemperatureManager.generated.h"

class ATimeOfDayManager;
class UTempModifierComponent;

UCLASS()
class PROCEDURALSURVIVAL_API ATemperatureManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATemperatureManager();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	float GetTemperatureAtLocation(FVector Location) const;
	
	void RegisterModifier(UTempModifierComponent* Modifier);
	void UnregisterModifier(UTempModifierComponent* Modifier);
	
	void SetTimeOfDayManager(ATimeOfDayManager* InTODManager);

protected:
	UPROPERTY(EditAnywhere, Category = "Temperature")
	UCurveFloat* TemperatureOverTimeOfDay;
	
	UPROPERTY(EditAnywhere, Category = "Temperature")
	ATimeOfDayManager* TimeOfDayManager;

private:	
	UPROPERTY()
	TArray<UTempModifierComponent*> Modifiers;
	
	void SetGlobalTemperature() const;
	
	mutable float GlobalTemperatureKelvin;
};
