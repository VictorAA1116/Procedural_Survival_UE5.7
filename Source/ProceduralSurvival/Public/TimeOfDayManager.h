// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeOfDayManager.generated.h"

class ADirectionalLight;

UCLASS()
class PROCEDURALSURVIVAL_API ATimeOfDayManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATimeOfDayManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// In-game time in hours (0.0 - 24.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay")
	float TimeOfDayHours = 12.0f;

	// How many in-game minutes pass per real-time second
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay")
	float TimeScale = 10.0f;

	// Bool to control whether time progresses
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay")
	bool AdvanceTime = true;

	// Directional light representing the sun
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay | Lights")
	TObjectPtr<ADirectionalLight> SunLight;

	// Directional light representing the moon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay | Lights")
	TObjectPtr<ADirectionalLight> MoonLight;

	// Appearance Tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay | Appearance")
	float SunMaxIntensityLux = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay | Appearance")
	float MoonMaxIntensityLux = 0.25f;

	// Controls how soft the transition is around sunrise/sunset (in degrees)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay | Appearance")
	float TwilightDegrees = 6.0f;

	// Direction of sunrise on yaw axis
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeOfDay | Appearance")
	float SunYawDegrees = 0.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	void UpdateSunAndMoon();
	static float SmoothStep(float Edge0, float Edge1, float X);
};
