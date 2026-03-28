// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FItemStack.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemRemoved);

UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class PROCEDURALSURVIVAL_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInventoryComponent();
	
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Called any time an item is added to this inventory component
	UPROPERTY(BlueprintAssignable)
	FOnItemAdded FOnItemAdded;
	
	// Called any time an item is removed from this inventory component
	UPROPERTY(BlueprintAssignable)
	FOnItemRemoved FOnItemRemoved;
	
	// Attempts to add an item to the inventory, returns true if successful and false if not.
	bool AddItem(UItemData* Item, int32 Quantity);
	
	// Attempts to remove an item from the inventory, returns true if successful and false if not.
	bool RemoveItem(UItemData* Item, int32 Quantity);

protected:
	UPROPERTY(EditAnywhere)
	int32 NumSlots = 20;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FItemStack> Items;

private:
	
	
};
