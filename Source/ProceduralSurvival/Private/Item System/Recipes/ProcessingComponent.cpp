// Fill out your copyright notice in the Description page of Project Settings.


#include "Item System/Recipes/ProcessingComponent.h"


// Sets default values for this component's properties
UProcessingComponent::UProcessingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UProcessingComponent::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<UInventoryComponent*> Inventories;
	GetOwner()->GetComponents<UInventoryComponent>(Inventories);
	
	if (Inventories.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Inventory components were found on %s, processing will not work"), *GetOwner()->GetName());
	}
	
	for (UInventoryComponent* Inventory : Inventories)
	{
		if (Inventory->ComponentHasTag("Input"))
		{
			InputInventory = Inventory;
			InputInventory->FOnInventoryChanged.AddDynamic(this, &UProcessingComponent::TrySetActiveRecipe);
		}
		else if (Inventory->ComponentHasTag("Output"))
		{
			OutputInventory = Inventory;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UProcessingComponent::BeginPlay(): Inventory component %s on actor %s has no tag, it will be ignored."), *Inventory->GetName(), *GetOwner()->GetName());
		}
	}
}


// Called every frame
void UProcessingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UProcessingComponent::TrySetActiveRecipe()
{
	if (InputInventory->GetFullInventory().Num() < 0 || PossibleRecipes.Num() <= 0) return;
	
	// Check if the input slots match any of the possible recipes, if so set that recipe as the active recipe
	for (URecipeData* Recipe : PossibleRecipes)
	{
		for (FItemAmount& Input : Recipe->Inputs)
		{
			if (InputInventory->ContainsItem(Input.ItemData, Input.Quantity))
			{
				ActiveRecipe = Recipe;
				return;
			}
		}
	}
	
	// If no matches, set ActiveRecipe to null
	ActiveRecipe = nullptr;
}

void UProcessingComponent::ProcessRecipe()
{
	if (!ActiveRecipe || !InputInventory || !OutputInventory) return;
	
	for (const FItemAmount& Input : ActiveRecipe->Inputs)
	{
		if (!InputInventory->RemoveItem(Input.ItemData, Input.Quantity))
		{
			return; // Not enough input items, abort processing
		}
	}
	
	for (const FItemAmount& Output : ActiveRecipe->Outputs)
	{
		OutputInventory->AddItem(Output.ItemData, Output.Quantity);
	}
}

