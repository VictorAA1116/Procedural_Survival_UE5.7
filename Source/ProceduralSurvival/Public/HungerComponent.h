// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseResourceComponent.h"
#include "HungerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStarvation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHungerDrained);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHungerRefilled);

UCLASS( ClassGroup=(Survival), meta=(BlueprintSpawnableComponent) )
class PROCEDURALSURVIVAL_API UHungerComponent : public UBaseResourceComponent
{
	GENERATED_BODY()
	
public:
	// Called when hunger value is at or below the min value
	UPROPERTY(BlueprintAssignable)
	FOnStarvation OnStarvation;
	
	// Called when the hunger value is decreased by any amount
	UPROPERTY(BlueprintAssignable)
	FOnHungerDrained OnHungerDrained;
	
	// Called when the hunger value is increased by any amount
	UPROPERTY(BlueprintAssignable)
	FOnHungerRefilled OnHungerRefilled;
	
	// Decreases the current hunger by the Drain Amount. Only positive numbers should be used
	UFUNCTION(BlueprintCallable)
	void DrainHunger(const int DrainAmount);
	
	// Increases the current hunger by the Refill Amount. Only positive numbers should be used
	UFUNCTION(BlueprintCallable)
	void RefillHunger(const int RefillAmount);

protected:
	
	virtual void OnResourceDepleted() override;
	virtual void OnResourceChanged(bool bPositiveChange) override;
	
};
