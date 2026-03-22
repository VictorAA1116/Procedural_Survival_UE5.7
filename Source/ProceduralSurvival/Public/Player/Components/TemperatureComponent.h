// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemperatureComponent.generated.h"

class ATemperatureManager;

UCLASS( ClassGroup=(Survival), meta=(BlueprintSpawnableComponent) )
class PROCEDURALSURVIVAL_API UTemperatureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTemperatureComponent();
	
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	float GetBodyTemperature() const { return BodyTemperature; }

protected:
	
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float BodyTemperature = 310.15f;
	
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float HeatTransferRate = 1.5f;
	
	// The value used as the alpha value when lerping the Body Temp to the Environment Temp. 0 = no insulation (more Env. Temp influence), 1 = perfect insulation (less Env. Temp influence).
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float Insulation = 0.5f;

private:
	ATemperatureManager* TemperatureManager;
	
	float CalculateEffectiveTemperature(float EnvironmentTemp) const;
	
};
