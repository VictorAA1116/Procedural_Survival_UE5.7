// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"

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

void UHealthComponent::OnResourceDepleted()
{
	OnDeath.Broadcast();
}

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
