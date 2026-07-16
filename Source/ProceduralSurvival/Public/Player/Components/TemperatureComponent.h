// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemperatureComponent.generated.h"

class ATemperatureManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExtremeCold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExtremeHeat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTemperatureNormalized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTemperatureChanged);

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
	
	// Called when the temperature component reaches the cold damage threshold temperature
	UPROPERTY(BlueprintAssignable)
	FOnExtremeCold OnExtremeCold;
	
	// Called when the temperature component reaches the heat damage threshold temperature
	UPROPERTY(BlueprintAssignable)
	FOnExtremeHeat OnExtremeHeat;
	
	// Called when the temperature component returns from extreme temperature to a safe temperature range
	UPROPERTY(BlueprintAssignable)
	FOnTemperatureNormalized OnTemperatureNormalized;
	
	// Called any time the temperature component changes significantly
	UPROPERTY(BlueprintAssignable)
	FOnTemperatureChanged OnTemperatureChanged;
	
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
	
	// Body temperature in Kelvin
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float BodyTemperature = 310.15f;
	
	// Rate at which heat is transferred between the environment and the body
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float HeatTransferRate = 1.5f;
	
	// The value used as the alpha value when lerping the Body Temp to the Environment Temp. 0 = no insulation (more Env. Temp influence), 1 = perfect insulation (less Env. Temp influence).
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float Insulation = 0.8f;
	
	// The value at which a player will start taking damage from extreme low temperatures
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float ColdDamageThresholdK = 308.0f;
	
	// The value at which a player will start taking damage from extreme high temperatures
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float HeatDamageThresholdK = 315.0f;
	
	bool bIsTemperatureExtreme = false;

private:
	ATemperatureManager* TemperatureManager;
	
	float CalculateEffectiveTemperature(float EnvironmentTemp) const;
	
};
