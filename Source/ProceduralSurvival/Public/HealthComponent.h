// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseResourceComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDamageTaken);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealed);

UCLASS( ClassGroup=(Survival), meta=(BlueprintSpawnableComponent) )
class PROCEDURALSURVIVAL_API UHealthComponent : public UBaseResourceComponent
{
	GENERATED_BODY()
	
public:
	// Event called when health is at or below the min value
	UPROPERTY(BlueprintAssignable)
	FOnDeath OnDeath;
	
	// Called when the health value is decreased by any amount
	UPROPERTY(BlueprintAssignable)
	FOnDamageTaken OnDamageTaken;
	
	// Called when the health value is increased by any amount
	UPROPERTY(BlueprintAssignable)
	FOnHealed OnHealed;
	
	// Decreases the current health by the Damage Amount. Only positive numbers should be used.
	UFUNCTION(BlueprintCallable)
	void ApplyDamage(const int DamageAmount);
	
	// Increases the current health by the Heal Amount. Only positive numbers should be used.
	UFUNCTION(BlueprintCallable)
	void Heal(const int HealAmount);
	
protected:
	
	virtual void OnResourceDepleted() override;
	virtual void OnResourceChanged(bool bPositiveChange) override;
	
};
