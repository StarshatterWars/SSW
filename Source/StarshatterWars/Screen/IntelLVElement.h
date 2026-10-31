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
#include "Components/ListView.h"
#include "IntelLVElement.generated.h"

class UTextBlock;
class UCheckBox;
class UBorder;


UCLASS()
class STARSHATTERWARS_API UIntelLVElement : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
	virtual void NativeOnEntryReleased() override;

private:

	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* NewsTitleText;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* NewsLocationText;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* NewsDateText;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* NewsSourceText;
	UPROPERTY(meta = (BindWidgetOptional)) UCheckBox* NewsVisited;

	UPROPERTY(meta = (BindWidgetOptional)) UBorder* SelectionBorder = nullptr;

protected:
	void ApplySelectionVisual();

private:
	UPROPERTY() UIntelListObject* IntelList;

	bool bRowSelected = false;
};
// Native list guarantees the code-built report entry in every campaign.
UCLASS()
class STARSHATTERWARS_API UIntelReportList : public UListView
{
    GENERATED_BODY()
public:
    UIntelReportList(const FObjectInitializer& ObjectInitializer);
};
