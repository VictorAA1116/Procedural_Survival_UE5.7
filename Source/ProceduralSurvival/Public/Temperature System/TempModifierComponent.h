// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "TempModifierComponent.generated.h"

class ATemperatureManager;

UENUM(BlueprintType)
enum class ETempBlendMode : uint8
{
	Override,
	Additive,
	Blend
};

UCLASS( ClassGroup=(Temperature), meta=(BlueprintSpawnableComponent) )
class PROCEDURALSURVIVAL_API UTempModifierComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTempModifierComponent();
	
	// Called when the game starts
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	bool IsPointInside(FVector Point) const;
	
	float ApplyTemperature(float CurrentTemp) const;

protected:
	
	// Box component used to define the bounds of the temperature modifier volume. The player must be inside this box to be affected by the temperature changes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta=(AllowPrivateAccess=true))
	UBoxComponent* BoxComponent;
	
	// The method that the Temperature Value will be applied with. Additive will add the Temperature Value to the ambient temp. Blend will blend between the ambient temp and the Temperature Value based on the Blend Alpha. Override will simply ignore ambient temperatures and apply the Temperature Value exactly.
	UPROPERTY(EditAnywhere, Category = "Temperature")
	ETempBlendMode TemperatureBlendMode = ETempBlendMode::Override;
	
	// The Temperature Value in Kelvin that this volume applies to the player when they are inside it. The way this value is applied depends on the selected Blend Mode.
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float TemperatureValue = 0.0f;
	
	// The alpha used when blending between the Temperature Value and the ambient temp while using the "Blend" Blend Mode.
	UPROPERTY(EditAnywhere, Category = "Temperature")
	float BlendAlpha = 0.5f;

private:	
	ATemperatureManager* TemperatureManager;
		
};
