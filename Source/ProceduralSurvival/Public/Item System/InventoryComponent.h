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
	// Sets default values for this component's properties.
	UInventoryComponent();
	
	// Called when the game starts.
	virtual void BeginPlay() override;
	
	// Called every frame.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Called any time an item is added to this inventory component.
	UPROPERTY(BlueprintAssignable)
	FOnItemAdded FOnItemAdded;
	
	// Called any time an item is removed from this inventory component.
	UPROPERTY(BlueprintAssignable)
	FOnItemRemoved FOnItemRemoved;
	
	// Called any time an item is either added or removed from this inventory component.
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged FOnInventoryChanged;
	
	// Attempts to add an item to the inventory, including the hotbar, returns the number of items successfully added.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemData* Item, int32 Quantity);
	
	// Attempts to add an item to the hotbar, excluding the rest of the inventory, returns the number of items successfully added.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItemToHotbar(UItemData* Item, int32 Quantity);
	
	// Attempts to remove an item from the inventory, returns the number of items successfully removed.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(UItemData* Item, int32 Quantity);
	
	// Attempts to add an item at the specified index, returns the number of items successfully added.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItemAtIndex(UItemData* Item, int32 Quantity, int32 Index);
	
	// Attempts to remove an item at the specified index, returns the number of items successfully removed.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItemAtIndex(UItemData* Item, int32 Quantity, int32 Index);
	
	// Attempts to remove an item from the hotbar, excluding the rest of the inventory, returns the number of items successfully removed.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItemFromHotbar(UItemData* Item, int32 Quantity);
	
	// Returns the number of total slots in the inventory, not including the hotbar slots.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetNumSlots() const { return NumInvSlots; }
	
	// Returns the number of total slots in the hotbar.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetNumHotbarSlots() const { return NumHotbarSlots; }
	
	// Returns the ItemStack struct at the specified index.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemStack GetItemAtIndex(int32 Index) const;
	
	// Returns true if the slot at the specified index is empty, false otherwise.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsSlotEmpty(int32 Index) const;
	
	// Returns the full array of ItemStacks that this inventory contains, not including the hotbar.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FItemStack> GetFullInventory() { return InvItems; }
	
	// Returns the full array of ItemStacks that this Hotbar contains.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FItemStack> GetHotbarItems() { return HotbarItems; }
	
	// Returns true if the inventory contains the amount specified of the item specified.
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
