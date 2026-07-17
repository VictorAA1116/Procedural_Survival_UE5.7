// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FItemStack.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemRemoved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

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
	
	// Called any time an item is either added or removed from this inventory component
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged FOnInventoryChanged;
	
	// Attempts to add an item to the inventory, returns the number of items successfully added.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int AddItem(UItemData* Item, int32 Quantity);
	
	// Attempts to remove an item from the inventory, returns the number of items successfully removed.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int RemoveItem(UItemData* Item, int32 Quantity);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemAtIndex(UItemData* Item, int32 Quantity, int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAtIndex(UItemData* Item, int32 Quantity, int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetNumSlots() const { return NumInvSlots; }
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetNumHotbarSlots() const { return NumHotbarSlots; }
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemStack GetItemAtIndex(int32 Index) const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsSlotEmpty(int32 Index) const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FItemStack> GetFullInventory() { return InvItems; }
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FItemStack> GetHotbarItems() { return HotbarItems; }
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ContainsItem(UItemData* Item, const int32 Quantity = 1) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 NumInvColumns = 1;

protected:
	
	UPROPERTY(EditAnywhere)
	int32 NumInvSlots = 20;
	
	UPROPERTY(EditAnywhere)
	int32 NumHotbarSlots = 5;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FItemStack> InvItems;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FItemStack> HotbarItems;

private:
	
	
};
