// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item System/Item Datas/ItemData.h"
#include "FoodItemData.generated.h"

/**
 * 
 */
UCLASS()
class PROCEDURALSURVIVAL_API UFoodItemData : public UItemData
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	float HungerRestoration;
	
};
