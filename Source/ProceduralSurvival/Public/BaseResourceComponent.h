// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseResourceComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROCEDURALSURVIVAL_API UBaseResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBaseResourceComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Resource)
	int CurrentValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Resource)
	int MaxValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Resource)
	int MinValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Resource)
	float RegenRate;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Resource)
	float DepleteRate;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, Category=Resource)
	void ModifyValue(const int DeltaAmount);
	
	UFUNCTION(BlueprintCallable, Category=Resource)
	float GetPercent() const;
	
};
