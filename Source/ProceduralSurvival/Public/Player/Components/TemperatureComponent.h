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
	
	UFUNCTION( BlueprintCallable, Category = "Temperature" )
	float GetBodyTemperatureKelvin() const { return BodyTemperature; }
	
	UFUNCTION( BlueprintCallable, Category = "Temperature" )
	float GetBodyTemperatureCelsius() const { return BodyTemperature - 273.15f; }
	
	UFUNCTION( BlueprintCallable, Category = "Temperature" )
	float GetBodyTemperatureFahrenheit() const { return GetBodyTemperatureCelsius() * 9.0f/5.0f + 32.0f; }
	
	UFUNCTION( BlueprintCallable, Category = "Temperature" )
	float GetAmbientTemperatureKelvin() const;
	
	UFUNCTION( BlueprintCallable, Category = "Temperature" )
	float GetAmbientTemperatureCelsius() const;
	
	UFUNCTION( BlueprintCallable, Category = "Temperature" )
	float GetAmbientTemperatureFahrenheit() const;
	

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
