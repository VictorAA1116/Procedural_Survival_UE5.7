// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeOfDayManager.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Timespan.h"
#include "Temperature System/TemperatureManager.h"

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
	
	SetTime(StartTime);
	
	if (ATemperatureManager* TemperatureManager = Cast<ATemperatureManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATemperatureManager::StaticClass())))
	{
		TemperatureManager->SetTimeOfDayManager(this);
	}
}

// Called every frame
void ATimeOfDayManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AdvanceTime)
	{
		const float HoursPerSecond = TimeScale / 60.0f;
		CurrentTime = FMath::Fmod(CurrentTime + HoursPerSecond * DeltaTime, DayLength);

		if (CurrentTime < 0.0f)
		{
			CurrentTime += DayLength;
		}

		UpdateTime();
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
	const float DayAlpha = FMath::Fmod(CurrentTime / DayLength, 1.0f);
	const float SunPitch = 90.0f - DayAlpha * 360.0f;

	const float Elevation = FMath::Sin(FMath::DegreesToRadians(SunPitch));
	const float TwilightThreshold = FMath::Sin(FMath::DegreesToRadians(TwilightDegrees));
	const float NightLightFactor = SmoothStep(-TwilightThreshold, TwilightThreshold, Elevation);

	const float DayLightFactor = 1.0f - NightLightFactor;

	if (SunLight)
	{
		SunLight->SetActorRotation(FRotator(SunPitch, 0.0f, SunRollDegrees));

		if (ULightComponent* SunLightComponent = SunLight->GetLightComponent())
		{
			SunLightComponent->SetIntensity(SunMaxIntensityLux * DayLightFactor);
			SunLightComponent->SetVisibility(DayLightFactor > KINDA_SMALL_NUMBER);
		}
	}

	if (MoonLight)
	{
		const float MoonPitch = SunPitch + 180.0f;
		MoonLight->SetActorRotation(FRotator(MoonPitch, 0.0f, SunRollDegrees));

		if (ULightComponent* MoonLightComponent = MoonLight->GetLightComponent())
		{
			MoonLightComponent->SetIntensity(MoonMaxIntensityLux * NightLightFactor);
			MoonLightComponent->SetVisibility(NightLightFactor > KINDA_SMALL_NUMBER);
		}
	}
}

void ATimeOfDayManager::UpdateTime()
{
	const double WrappedHours = FMath::Fmod(CurrentTime, DayLength);
	const double ClampedHours = WrappedHours < 0.0f ? WrappedHours + DayLength : WrappedHours;
	
	TimeOfDay = FTimespan::FromHours(ClampedHours);
}

void ATimeOfDayManager::SetTime(float NewTimeInHours)
{
	CurrentTime = FMath::Fmod(NewTimeInHours, DayLength);

	if (CurrentTime < 0.0f)
	{
		CurrentTime += DayLength;
	}

	UpdateTime();
	UpdateSunAndMoon();
}

FString ATimeOfDayManager::GetTime(bool Use24HFormat, bool IncludeSeconds) const
{
	const int32 Hours24 = TimeOfDay.GetHours();
	const int32 Minutes = TimeOfDay.GetMinutes();
	const int32 Seconds = TimeOfDay.GetSeconds();

	int32 DisplayHours = Hours24;
	FString Suffix;

	if (!Use24HFormat)
	{
		Suffix = (Hours24 >= 12) ? TEXT(" PM") : TEXT(" AM");
		DisplayHours = Hours24 % 12;

		if (DisplayHours == 0)
		{
			DisplayHours = 12;
		}
	}

	const FString TimeString = IncludeSeconds ?
		FString::Printf(TEXT("%02d:%02d:%02d%s"), DisplayHours, Minutes, Seconds, *Suffix) :
		FString::Printf(TEXT("%02d:%02d%s"), DisplayHours, Minutes, *Suffix);

	return TimeString;
}

float ATimeOfDayManager::GetTimeNormalized() const
{
	const float NormalizedTime = CurrentTime / DayLength;
	
	return NormalizedTime;
}
