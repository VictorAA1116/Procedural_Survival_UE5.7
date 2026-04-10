// Fill out your copyright notice in the Description page of Project Settings.

#include "Item System/InventoryComponent.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Items.SetNum(NumSlots);
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UInventoryComponent::AddItem(UItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return false;
	
	for (FItemStack& Stack : Items)
	{
		if (Stack.ItemData == Item && Stack.Quantity < Item->MaxStackSize)
		{
			const int32 SpaceLeft = Item->MaxStackSize - Stack.Quantity;
			const int32 ToAdd = FMath::Min(SpaceLeft, Quantity);
			
			Stack.Quantity += ToAdd;
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				return true;
			}
		}
		
		if (!Stack.IsValid())
		{
			int32 ToAdd = FMath::Min(Item->MaxStackSize, Quantity);
			Stack.ItemData = Item;
			Stack.Quantity = ToAdd;
			
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				return true;
			}
		}
	}
	
	return false;
}

bool UInventoryComponent::RemoveItem(UItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return false;
	
	for (int32 i = Items.Num() - 1; i >= 0; i--)
	{
		FItemStack& Stack = Items[i];
		
		if (Stack.ItemData == Item)
		{
			const int32 ToRemove = FMath::Min(Stack.Quantity, Quantity);
			
			Stack.Quantity -= ToRemove;
			Quantity -= ToRemove;
			
			if (Stack.Quantity <= 0)
			{
				Stack = FItemStack();
				FOnItemRemoved.Broadcast();
			}
			
			if (Quantity <= 0) return true;
		}
	}
	
	return false;
}

bool UInventoryComponent::AddItemAtIndex(UItemData* Item, int32 Quantity, int32 Index)
{
	if (!Item || Quantity <= 0 || !Items.IsValidIndex(Index)) return false;
	
	if (Items[Index].ItemData == Item)
	{
		int32 SpaceLeft = Item->MaxStackSize - Items[Index].Quantity;
		int32 ToAdd = FMath::Min(SpaceLeft, Quantity);
		
		Items[Index].Quantity += ToAdd;
		Quantity -= ToAdd;
		
		if (Quantity <= 0) return true;
	}
	else if (Items[Index].Quantity == 0 && Quantity <= Item->MaxStackSize)
	{
		FItemStack NewStack;
		NewStack.ItemData = Item;
		NewStack.Quantity = Quantity;
		
		Items[Index] = NewStack;
		Quantity -= Items[Index].Quantity;
		
		if (Quantity <= 0) return true;
	}
	
	return false;
}

bool UInventoryComponent::RemoveItemAtIndex(UItemData* Item, int32 Quantity, int32 Index)
{
	if (!Item || Quantity <= 0 || !Items.IsValidIndex(Index)) return false;
	
	Items[Index] = FItemStack();
	return true;
}

int32 UInventoryComponent::GetNumSlots() const
{
	return NumSlots;
}

FItemStack UInventoryComponent::GetItemAtIndex(int32 Index) const
{
	if (Items.IsValidIndex(Index))
	{
		return Items[Index];
	}
	
	return FItemStack(); // Return empty if index is not valid
}

bool UInventoryComponent::IsSlotEmpty(int32 Index) const
{
	if (!Items.IsValidIndex(Index)) return true;
	
	// If item is valid, return not empty
	return !Items[Index].IsValid();
}

