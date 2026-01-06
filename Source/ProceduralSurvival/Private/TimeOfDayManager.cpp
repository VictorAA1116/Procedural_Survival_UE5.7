// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeOfDayManager.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Math/UnrealMathUtility.h"

// Sets default values
ATimeOfDayManager::ATimeOfDayManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATimeOfDayManager::BeginPlay()
{
	Super::BeginPlay();
	UpdateSunAndMoon();
}

// Called every frame
void ATimeOfDayManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AdvanceTime)
	{
		const float HoursPerSecond = TimeScale / 60.0f;
		TimeOfDayHours = FMath::Fmod(TimeOfDayHours + HoursPerSecond * DeltaTime, 24.0f);

		if (TimeOfDayHours < 0.0f)
		{
			TimeOfDayHours += 24.0f;
		}

		UpdateSunAndMoon();
	}
}

float ATimeOfDayManager::SmoothStep(float Edge0, float Edge1, float X)
{
	const float Time = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.0f, 1.0f);
	return Time * Time * (3.0f - 2.0f * Time);
}

void ATimeOfDayManager::UpdateSunAndMoon()
{
	const float DayAlpha = FMath::Fmod(TimeOfDayHours / 24.0f, 1.0f);
	const float SunPitch = DayAlpha * 360.0f - 90.0f;

	const float Elevation = FMath::Sin(FMath::DegreesToRadians(SunPitch));
	const float TwilightThreshold = FMath::Sin(FMath::DegreesToRadians(TwilightDegrees));
	const float NightLightFactor = SmoothStep(-TwilightThreshold, TwilightThreshold, Elevation);

	const float DayLightFactor = 1.0f - NightLightFactor;

	if (SunLight)
	{
		SunLight->SetActorRotation(FRotator(SunPitch, SunYawDegrees, 0.0f));

		if (ULightComponent* SunLightComponent = SunLight->GetLightComponent())
		{
			SunLightComponent->SetIntensity(SunMaxIntensityLux * DayLightFactor);
			SunLightComponent->SetVisibility(DayLightFactor > KINDA_SMALL_NUMBER);
		}
	}

	if (MoonLight)
	{
		const float MoonPitch = SunPitch + 180.0f;
		MoonLight->SetActorRotation(FRotator(MoonPitch, SunYawDegrees, 0.0f));

		if (ULightComponent* MoonLightComponent = MoonLight->GetLightComponent())
		{
			MoonLightComponent->SetIntensity(MoonMaxIntensityLux * NightLightFactor);
			MoonLightComponent->SetVisibility(NightLightFactor > KINDA_SMALL_NUMBER);
		}
	}
}

