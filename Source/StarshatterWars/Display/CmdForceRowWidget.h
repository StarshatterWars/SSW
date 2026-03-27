/*	Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    UI
	FILE:         CmdForceRowWidget.h
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ListView row widget for CmdForceDlg.
	Displays a single UCmdForceListItem.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CmdForceRowWidget.generated.h"

class UTextBlock;
class USpacer;
class UCmdForceListItem;

UCLASS()
class STARSHATTERWARS_API UCmdForceRowWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UTextBlock* RowText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UTextBlock* ExpandText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	USpacer* IndentSpacer = nullptr;

protected:
	UPROPERTY(BlueprintReadOnly)
	UCmdForceListItem* CurrentItem = nullptr;
};