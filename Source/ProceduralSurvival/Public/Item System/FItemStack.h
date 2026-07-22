#pragma once

#include "CoreMinimal.h"
#include "Item Datas/ItemData.h"
#include "FItemStack.generated.h"

USTRUCT(BlueprintType)
struct FItemStack
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UItemData* ItemData = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;
	
	bool IsValid() const
	{
		return ItemData != nullptr && Quantity > 0;
	}
};
