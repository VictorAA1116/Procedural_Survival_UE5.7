// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/HealthComponent.h"

// Calls BaseResourceComponent's function to modify the current health value
void UHealthComponent::ApplyDamage(const int DamageAmount)
{
	// Negate damage amount
	ModifyCurrentValue(-DamageAmount);
}

// Calls BaseResourceComponent's function to modify the current health value
void UHealthComponent::Heal(const int HealAmount)
{
	ModifyCurrentValue(HealAmount);
}

// Relays parent class's event to the OnDeath event for this class
void UHealthComponent::OnResourceDepleted()
{
	OnDeath.Broadcast();
}

// Relays parent class's event to either the Heal or Damage event for this class
void UHealthComponent::OnResourceChanged(bool bPositiveChange)
{
	if (bPositiveChange)
	{
		OnHealed.Broadcast();
	}
	else
	{
		OnDamageTaken.Broadcast();
	}
}
