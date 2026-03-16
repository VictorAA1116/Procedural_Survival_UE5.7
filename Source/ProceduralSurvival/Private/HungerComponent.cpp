// Fill out your copyright notice in the Description page of Project Settings.

#include "HungerComponent.h"

// Calls BaseResourceComponent's function to modify the current hunger value
void UHungerComponent::DrainHunger(const int DrainAmount)
{
	// Negate deplete amount
	ModifyCurrentValue(-DrainAmount);
}

// Calls BaseResourceComponent's function to modify the current hunger value
void UHungerComponent::RefillHunger(const int RefillAmount)
{
	ModifyCurrentValue(RefillAmount);
}

void UHungerComponent::OnResourceDepleted()
{
	OnStarvation.Broadcast();
}

void UHungerComponent::OnResourceChanged(bool bPositiveChange)
{
	if (bPositiveChange)
	{
		OnHungerRefilled.Broadcast();
	}
	else
	{
		OnHungerDrained.Broadcast();
	}
}
