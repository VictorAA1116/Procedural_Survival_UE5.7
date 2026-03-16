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
}


// Called every frame
void UBaseResourceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// To be called by children when they deem appropriate
void UBaseResourceComponent::ModifyValue(const int DeltaAmount)
{
	// Modifies the CurrentValue by adding the Delta Amount and clamping to Min and Max Values
	CurrentValue = FMath::Clamp(CurrentValue + DeltaAmount, MinValue, MaxValue);
}


// Returns the current value as a float percentage of the max value
float UBaseResourceComponent::GetPercent() const
{
	// Cast to float to increase precision
	return CurrentValue / static_cast<float>(MaxValue);
}

