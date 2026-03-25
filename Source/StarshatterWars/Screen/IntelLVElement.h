// +----------------------------------------------------------------------+
// | UIntelLVElement                                                      |
// +----------------------------------------------------------------------+
// | PURPOSE:                                                             |
// |   Visual representation of an intel list row.                        |
// +----------------------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "IntelListObject.h"
#include "IntelLVElement.generated.h"

class UTextBlock;
class UCheckBox;

UCLASS()
class STARSHATTERWARS_API UIntelLVElement : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

private:

	UPROPERTY(meta = (BindWidget)) UTextBlock* NewsTitleText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NewsLocationText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NewsDateText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NewsSourceText;
	UPROPERTY(meta = (BindWidget)) UCheckBox* NewsVisited;

private:
	UPROPERTY() UIntelListObject* IntelList;
};