// Fill out your copyright notice in the Description page of Project Settings.

#include "StaminaComponent.h"

// Calls BaseResourceComponent's function to modify the current stamina value
void UStaminaComponent::DrainStamina(const int DrainAmount)
{
	// Negate drain amount
	ModifyCurrentValue(-DrainAmount);
}

// Calls BaseResourceComponent's function to modify the current stamina value
void UStaminaComponent::RefillStamina(const int RefillAmount)
{
	ModifyCurrentValue(RefillAmount);
}

// Relays parent class's event to the OnStaminaDrained event for this class
void UStaminaComponent::OnResourceDepleted()
{
	if (bCanSprint)
	{
		bCanSprint = false;
	}
	
	OnStaminaDrained.Broadcast();
}


// Relays parent class's event to either the Refilled or Drained event for this class
void UStaminaComponent::OnResourceChanged(const bool bPositiveChange)
{
	if (bPositiveChange)
	{
		if (!bCanSprint)
		{
			bCanSprint = true;
		}
		
		OnStaminaRefilled.Broadcast();
	}
	else
	{
		OnStaminaDrained.Broadcast();
	}
}