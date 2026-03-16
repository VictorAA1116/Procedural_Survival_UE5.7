// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseResourceComponent.generated.h"

UCLASS()
class PROCEDURALSURVIVAL_API UBaseResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBaseResourceComponent();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Resource Values
	
	// The current value of the resource, can be increased and decreased at runtime based on functions, events, or timers
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Resource")
	int CurrentValue;
	
	// The maximum value that this resource can hold
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Resource")
	int MaxValue = 100;
	
	// The minimum value that this resource can hold. In most cases, this should be left at 0
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Resource")
	int MinValue = 0;
	
	// Regeneration
	
	// The time in seconds between increasing the resource's current value. Values below 0 disable regeneration
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Resource|Regeneration")
	float RegenRate = -1;
	
	// The amount that the current value of the resource will regenerate by on every regeneration interval
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Resource|Regeneration")
	int RegenAmount = 0;
	
	// Depletion
	
	// The time in seconds between decreasing the resource's current value. Values below 0 disable depletion
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Resource|Depletion")
	float DepleteRate = -1;
	
	// The amount that the current value of the resource will deplete by on every depletion interval
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Resource|Depletion")
	int DepleteAmount = 0;
	
	// Virtual functions for children to link their events to
	virtual void OnResourceDepleted();
	virtual void OnResourceChanged(const bool bPositiveChange);

private:
	FTimerHandle RegenTimerHandle;
	FTimerHandle DepleteTimerHandle;
	
	void RegenTick();
	void DepleteTick();
	
	bool bWasDepleted = false;
	
public:	
	// Modifies the current value of the resource by the Delta Amount
	UFUNCTION(BlueprintCallable, Category="Resource")
	void ModifyCurrentValue(const int DeltaAmount);
	
	// Modifies the regeneration values and restarts the timer
	UFUNCTION(BlueprintCallable, Category="Resource")
	void ModifyRegeneration(const int NewRegenAmount, const float NewRegenRate);
	
	// Modifies the depletion values and restarts the timer
	UFUNCTION(BlueprintCallable, Category="Resource")
	void ModifyDepletion(const int NewDepleteAmount, const float NewDepleteRate);
	
	// Starts the timer for regenerating the resource
	UFUNCTION(BlueprintCallable, Category="Resource|Regeneration")
	void StartRegenTimer();
	
	// Stops the timer for regenerating the resource
	UFUNCTION(BlueprintCallable, Category="Resource|Regeneration")
	void StopRegenTimer();
	
	// Starts the timer for depleting the resource
	UFUNCTION(BlueprintCallable, Category="Resource|Depletion")
	void StartDepletionTimer();
	
	// Stops the timer for depleting the resource
	UFUNCTION(BlueprintCallable, Category="Resource|Depletion")
	void StopDepletionTimer();
	
	// Returns the percentage of the current value relative to the max value as a float
	UFUNCTION(BlueprintCallable, Category="Resource")
	float GetPercent() const;
	
};
