#pragma once

#include "CoreMinimal.h"
#include "Item System/Item Datas/ItemData.h"
#include "FItemAmount.generated.h"

USTRUCT(BlueprintType)
struct FItemAmount
{
	GENERATED_BODY();
	
public:
	UPROPERTY(EditAnywhere)
	UItemData* ItemData;
	
	UPROPERTY(EditAnywhere)
	int32 Quantity;
};
