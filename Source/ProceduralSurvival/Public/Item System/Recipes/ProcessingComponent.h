// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RecipeData.h"
#include "Components/ActorComponent.h"
#include "Item System/InventoryComponent.h"
#include "ProcessingComponent.generated.h"


UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class PROCEDURALSURVIVAL_API UProcessingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UProcessingComponent();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(VisibleAnywhere)
	UInventoryComponent* InputInventory;
	
	UPROPERTY(VisibleAnywhere)	
	UInventoryComponent* OutputInventory;
	
	UPROPERTY(EditAnywhere)
	URecipeData* ActiveRecipe;
	
	void ProcessRecipe();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	
	
};
