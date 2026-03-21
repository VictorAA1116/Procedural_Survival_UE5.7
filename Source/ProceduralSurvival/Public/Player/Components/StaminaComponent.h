// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseResourceComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDrained);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaRefilled);

UCLASS( ClassGroup=(Survival), meta=(BlueprintSpawnableComponent) )
class PROCEDURALSURVIVAL_API UStaminaComponent : public UBaseResourceComponent
{
	GENERATED_BODY()
	
public:
	// Called when the stamina value is at or below the min value
	UPROPERTY(BlueprintAssignable)
	FOnStaminaExhausted OnStaminaExhausted;
	
	// Called when the stamina value is decreased by any amount
	UPROPERTY(BlueprintAssignable)
	FOnStaminaDrained OnStaminaDrained;
	
	// Called when the stamina value is increased by any amount
	UPROPERTY(BlueprintAssignable)
	FOnStaminaRefilled OnStaminaRefilled;
	
	// Decreases the stamina value by Drain Amount. Only positive numbers should be used.
	UFUNCTION(BlueprintCallable)
	void DrainStamina(const int DrainAmount);
	
	// Increases the stamina value by Refill Amount. Only positive numbers should be used.
	UFUNCTION(BlueprintCallable)
	void RefillStamina(const int RefillAmount);
	
	// Whether the owner of this component is allowed to sprint or not. This is automatically updated
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanSprint = true;
	
protected:
	
	virtual void OnResourceDepleted() override;
	virtual void OnResourceChanged(const bool bPositiveChange) override;
	
};
