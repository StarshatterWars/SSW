#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MissionListObject.h"
#include "MissionLVElement.generated.h"

class UTextBlock;
class UBorder;

UCLASS()
class STARSHATTERWARS_API UMissionLVElement : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MissionName = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MissionType = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MissionStatus = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MissionTime = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* SelectionBorder = nullptr;

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
	virtual void NativeOnEntryReleased() override;

	void ApplySelectionVisual();

protected:
	UPROPERTY()
	UMissionListObject* MissionList = nullptr;

	bool bRowSelected = false;
};