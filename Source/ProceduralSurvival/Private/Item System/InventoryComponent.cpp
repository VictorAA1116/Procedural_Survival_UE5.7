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

	InvItems.SetNum(NumInvSlots);
	HotbarItems.SetNum(NumHotbarSlots);
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

int32 UInventoryComponent::AddItem(UItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return 0;
	
	int32 NumSuccessfullyAdded = 0;
	
	for (FItemStack& Stack : InvItems)
	{
		if (Stack.ItemData == Item && Stack.Quantity < Item->MaxStackSize)
		{
			const int32 SpaceLeft = Item->MaxStackSize - Stack.Quantity;
			const int32 ToAdd = FMath::Min(SpaceLeft, Quantity);
			
			Stack.Quantity += ToAdd;
			NumSuccessfullyAdded += ToAdd;
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				FOnInventoryChanged.Broadcast();
				return NumSuccessfullyAdded;
			}
		}
		
		if (!Stack.IsValid())
		{
			int32 ToAdd = FMath::Min(Item->MaxStackSize, Quantity);
			Stack.ItemData = Item;
			Stack.Quantity = ToAdd;
			
			NumSuccessfullyAdded += ToAdd;
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				FOnInventoryChanged.Broadcast();
				return NumSuccessfullyAdded;
			}
		}
	}
	for (FItemStack& Stack : HotbarItems)
	{
		if (Stack.ItemData == Item && Stack.Quantity < Item->MaxStackSize)
		{
			const int32 SpaceLeft = Item->MaxStackSize - Stack.Quantity;
			const int32 ToAdd = FMath::Min(SpaceLeft, Quantity);
			
			Stack.Quantity += ToAdd;
			NumSuccessfullyAdded += ToAdd;
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				FOnInventoryChanged.Broadcast();
				return NumSuccessfullyAdded;
			}
		}
		
		if (!Stack.IsValid())
		{
			int32 ToAdd = FMath::Min(Item->MaxStackSize, Quantity);
			Stack.ItemData = Item;
			Stack.Quantity = ToAdd;
			
			NumSuccessfullyAdded += ToAdd;
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				FOnInventoryChanged.Broadcast();
				return NumSuccessfullyAdded;
			}
		}
	}
	
	return NumSuccessfullyAdded;
}

int32 UInventoryComponent::AddItemToHotbar(UItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return false;
	
	int32 NumSuccessfullyAdded = 0;
	
	for (FItemStack& Stack : HotbarItems)
	{
		if (Stack.ItemData == Item && Stack.Quantity < Item->MaxStackSize)
		{
			const int32 SpaceLeft = Item->MaxStackSize - Stack.Quantity;
			const int32 ToAdd = FMath::Min(SpaceLeft, Quantity);
			
			Stack.Quantity += ToAdd;
			NumSuccessfullyAdded += ToAdd;
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				FOnInventoryChanged.Broadcast();
				return NumSuccessfullyAdded;
			}
		}
		
		if (!Stack.IsValid())
		{
			int32 ToAdd = FMath::Min(Item->MaxStackSize, Quantity);
			Stack.ItemData = Item;
			Stack.Quantity = ToAdd;
			
			NumSuccessfullyAdded += ToAdd;
			Quantity -= ToAdd;
			
			if (Quantity <= 0)
			{
				FOnItemAdded.Broadcast();
				FOnInventoryChanged.Broadcast();
				return NumSuccessfullyAdded;
			}
		}
	}
	
	return NumSuccessfullyAdded;
}

int32 UInventoryComponent::RemoveItem(UItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return 0;
	
	int32 NumSuccessfullyRemoved = 0;
	
	for (int32 i = 0; i < InvItems.Num(); i++)
	{
		FItemStack& Stack = InvItems[i];
		
		if (Stack.ItemData == Item)
		{
			const int32 ToRemove = FMath::Min(Stack.Quantity, Quantity);
			
			Stack.Quantity -= ToRemove;
			Quantity -= ToRemove;
			NumSuccessfullyRemoved += ToRemove;
			
			if (Stack.Quantity <= 0)
			{
				Stack = FItemStack();
				FOnItemRemoved.Broadcast();
				FOnInventoryChanged.Broadcast();
			}
			
			if (Quantity <= 0) return NumSuccessfullyRemoved;
		}
	}
	
	return NumSuccessfullyRemoved;
}

int32 UInventoryComponent::AddItemAtIndex(UItemData* Item, int32 Quantity, int32 Index)
{
	if (!Item || Quantity <= 0) return 0;
	
	int32 NumSuccessfullyAdded = 0;
	
	if (!InvItems.IsValidIndex(Index) && Index < NumInvSlots + NumHotbarSlots)
	{
		int HotbarIndex = Index - NumInvSlots;
		
		if (HotbarItems[HotbarIndex].ItemData == Item)
		{
			int32 SpaceLeft = Item->MaxStackSize - HotbarItems[HotbarIndex].Quantity;
			int32 ToAdd = FMath::Min(SpaceLeft, Quantity);
		
			HotbarItems[HotbarIndex].Quantity += ToAdd;
			Quantity -= ToAdd;
			NumSuccessfullyAdded += ToAdd;
		
			if (Quantity <= 0) return NumSuccessfullyAdded;
		}
		else if (HotbarItems[HotbarIndex].Quantity == 0 && Quantity <= Item->MaxStackSize)
		{
			FItemStack NewStack;
			NewStack.ItemData = Item;
			NewStack.Quantity = Quantity;
		
			HotbarItems[HotbarIndex] = NewStack;
			Quantity -= HotbarItems[HotbarIndex].Quantity;
			NumSuccessfullyAdded += HotbarItems[HotbarIndex].Quantity;
		
			if (Quantity <= 0) return NumSuccessfullyAdded;
		}
	}
	else
	{
		if (InvItems[Index].ItemData == Item)
		{
			int32 SpaceLeft = Item->MaxStackSize - InvItems[Index].Quantity;
			int32 ToAdd = FMath::Min(SpaceLeft, Quantity);
		
			InvItems[Index].Quantity += ToAdd;
			Quantity -= ToAdd;
			NumSuccessfullyAdded += ToAdd;
		
			if (Quantity <= 0) return NumSuccessfullyAdded;
		}
		else if (InvItems[Index].Quantity == 0 && Quantity <= Item->MaxStackSize)
		{
			FItemStack NewStack;
			NewStack.ItemData = Item;
			NewStack.Quantity = Quantity;
		
			InvItems[Index] = NewStack;
			Quantity -= InvItems[Index].Quantity;
			NumSuccessfullyAdded += InvItems[Index].Quantity;
		
			if (Quantity <= 0) return NumSuccessfullyAdded;
		}
	}
	
	return NumSuccessfullyAdded;
}

int32 UInventoryComponent::RemoveItemAtIndex(UItemData* Item, int32 Quantity, int32 Index)
{
	if (!Item || Quantity <= 0) return 0;
	
	int32 NumSuccessfullyRemoved = 0;
	
	if (InvItems.IsValidIndex(Index))
	{
		if (InvItems[Index].ItemData != Item) return 0;
		
		int32 ToRemove = FMath::Min(Quantity, InvItems[Index].Quantity);
		
		InvItems[Index].Quantity -= ToRemove;
		NumSuccessfullyRemoved += ToRemove;
		
		if (InvItems[Index].Quantity <= 0)
		{
			InvItems[Index] = FItemStack();
			return NumSuccessfullyRemoved;
		}
	}
	else if (Index < NumInvSlots + NumHotbarSlots)
	{
		int HotbarIndex = Index - NumInvSlots;
		
		if (HotbarItems[HotbarIndex].ItemData != Item) return 0;
		
		int32 ToRemove = FMath::Min(Quantity, HotbarItems[HotbarIndex].Quantity);
		
		HotbarItems[HotbarIndex].Quantity -= ToRemove;
		NumSuccessfullyRemoved += ToRemove;
		
		if (HotbarItems[HotbarIndex].Quantity <= 0)
		{
			HotbarItems[HotbarIndex] = FItemStack();
			return NumSuccessfullyRemoved;
		}
	}
	
	return NumSuccessfullyRemoved;
}

int32 UInventoryComponent::RemoveItemFromHotbar(UItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return 0;
	
	int32 NumSuccessfullyRemoved = 0;
	
	for (int32 i = 0; i < HotbarItems.Num(); i++)
	{
		FItemStack& Stack = HotbarItems[i];
		
		if (Stack.ItemData == Item)
		{
			const int32 ToRemove = FMath::Min(Stack.Quantity, Quantity);
			
			Stack.Quantity -= ToRemove;
			Quantity -= ToRemove;
			NumSuccessfullyRemoved += ToRemove;
			
			if (Stack.Quantity <= 0)
			{
				Stack = FItemStack();
				FOnItemRemoved.Broadcast();
				FOnInventoryChanged.Broadcast();
			}
			
			if (Quantity <= 0) return NumSuccessfullyRemoved;
		}
	}
	
	return NumSuccessfullyRemoved;
}

FItemStack UInventoryComponent::GetItemAtIndex(int32 Index) const
{
	if (InvItems.IsValidIndex(Index))
	{
		return InvItems[Index]; // Return item from main inventory
	}
	else if (Index >= NumInvSlots && Index < NumInvSlots + NumHotbarSlots)
	{
		return HotbarItems[Index - NumInvSlots]; // Return item from hotbar
	}
	
	return FItemStack(); // Return empty if index is not valid
}

bool UInventoryComponent::IsSlotEmpty(int32 Index) const
{
	if (Index >= NumInvSlots && !HotbarItems.IsValidIndex(Index - NumInvSlots)) return true;
	if (Index < NumInvSlots && !InvItems.IsValidIndex(Index)) return true;
	
	// If item is valid, return not empty
	return Index >= NumInvSlots ? !HotbarItems[Index - NumInvSlots].IsValid() : !InvItems[Index].IsValid();
}

bool UInventoryComponent::ContainsItem(UItemData* Item, const int32 Quantity) const
{
	if (!Item || Quantity <= 0) return false;
	
	int32 TotalQuantity = 0;
	
	for (const FItemStack& Stack : InvItems)
	{
		if (Stack.ItemData == Item)
		{
			TotalQuantity += Stack.Quantity;
			if (TotalQuantity >= Quantity) return true;
		}
	}
	
	for (const FItemStack& Stack : HotbarItems)
	{
		if (Stack.ItemData == Item)
		{
			TotalQuantity += Stack.Quantity;
			if (TotalQuantity >= Quantity) return true;
		}
	}
	
	return false;
}

