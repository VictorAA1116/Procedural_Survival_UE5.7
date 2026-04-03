// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FItemAmount.h"
#include "Engine/DataAsset.h"
#include "RecipeData.generated.h"

/**
 * 
 */
UCLASS()
class PROCEDURALSURVIVAL_API URecipeData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	TArray<FItemAmount> Inputs;
	
	UPROPERTY(EditAnywhere)
	TArray<FItemAmount> Outputs;
	
	UPROPERTY(EditAnywhere)
	float ProcessingTime;
};
