// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseResourceComponent.h"

// Sets default values for this component's properties
UBaseResourceComponent::UBaseResourceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UBaseResourceComponent::BeginPlay()
{
	Super::BeginPlay();
	
	StartDepletionTimer();
	StartRegenTimer();
}

// Called every frame
void UBaseResourceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// Clears any previous timer for regeneration in the handle and starts a new timer if the rate is a positive value
void UBaseResourceComponent::StartRegenTimer()
{
	StopRegenTimer();
	
	if (RegenRate < 0) return;
	
	GetWorld()->GetTimerManager().SetTimer(
		RegenTimerHandle,
		this,
		&UBaseResourceComponent::RegenTick,
		RegenRate,
		true
	);
}

// Called by the regeneration timer
void UBaseResourceComponent::RegenTick()
{
	if (CurrentValue >= MaxValue) return;
	
	ModifyCurrentValue(RegenAmount);
}

// Stops and clears the timer for regeneration
void UBaseResourceComponent::StopRegenTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(RegenTimerHandle);
}

// Updates the values of RegenAmount and RegenRate and restarts the timer with these new values
void UBaseResourceComponent::ModifyRegeneration(const int NewRegenAmount, const float NewRegenRate)
{
	RegenAmount = NewRegenAmount;
	RegenRate = NewRegenRate;
	
	StartRegenTimer();
}

// Clears any previous depletion timer in the handle and starts a new timer if the rate is a positive value
void UBaseResourceComponent::StartDepletionTimer()
{
	StopDepletionTimer();
	
	if (DepleteRate < 0) return;
	
	GetWorld()->GetTimerManager().SetTimer(
		DepleteTimerHandle,
		this,
		&UBaseResourceComponent::DepleteTick,
		DepleteRate,
		true
	);
}

// Called by the depletion timer
void UBaseResourceComponent::DepleteTick()
{
	if (CurrentValue <= MinValue) return;
	
	ModifyCurrentValue(DepleteAmount);
}

// Stops and clears the timer for depletion
void UBaseResourceComponent::StopDepletionTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(DepleteTimerHandle);
}

// Updates the values of DepleteAmount and DepleteRate and restarts the timer with these new values
void UBaseResourceComponent::ModifyDepletion(const int NewDepleteAmount, const float NewDepleteRate)
{
	DepleteAmount = NewDepleteAmount;
	DepleteRate = NewDepleteRate;
	
	StartDepletionTimer();
}

// To be called by children when they deem appropriate
void UBaseResourceComponent::ModifyCurrentValue(const int DeltaAmount)
{
	// Modifies the CurrentValue by adding the Delta Amount and clamping to Min and Max Values
	CurrentValue = FMath::Clamp(CurrentValue + DeltaAmount, MinValue, MaxValue);
	
	// Broadcast Event
	OnResourceChanged(DeltaAmount > 0);
	
	// Call virtual function for children to override
	if (!bWasDepleted && CurrentValue <= MinValue)
	{
		bWasDepleted = true;
		OnResourceDepleted();
	}
	
	if (bWasDepleted && CurrentValue > MinValue)
	{
		bWasDepleted = false;
	}
}

// Returns the current value as a float percentage of the max value
float UBaseResourceComponent::GetPercent() const
{
	// Cast to float to increase precision
	return CurrentValue / static_cast<float>(MaxValue);
}

// Called when the resource is fully depleted
void UBaseResourceComponent::OnResourceDepleted()
{
	// To be overridden by children
}

// Called any time the resource's current value changes
void UBaseResourceComponent::OnResourceChanged(const bool bPositiveChange)
{
	// To be overridden by children
}

