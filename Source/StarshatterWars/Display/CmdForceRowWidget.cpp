/*	Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    UI
	FILE:         CmdForceRowWidget.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ListView row widget for CmdForceDlg.
*/

#include "CmdForceRowWidget.h"
#include "CmdForceListItem.h"

#include "Components/TextBlock.h"
#include "Components/Spacer.h"

void UCmdForceRowWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	CurrentItem = Cast<UCmdForceListItem>(ListItemObject);

	if (!CurrentItem)
	{
		if (RowText)
		{
			RowText->SetText(FText::GetEmpty());
		}

		if (ExpandText)
		{
			ExpandText->SetText(FText::GetEmpty());
		}

		if (IndentSpacer)
		{
			IndentSpacer->SetSize(FVector2D(0.0f, 1.0f));
		}

		return;
	}

	if (RowText)
	{
		RowText->SetText(FText::FromString(CurrentItem->DisplayText));
	}

	if (IndentSpacer)
	{
		const float IndentWidth = CurrentItem->IndentLevel * 24.0f;
		IndentSpacer->SetSize(FVector2D(IndentWidth, 1.0f));
	}

	if (ExpandText)
	{
		if (CurrentItem->IsGroup() && CurrentItem->bHasChildren)
		{
			ExpandText->SetText(FText::FromString(
				CurrentItem->bExpanded ? TEXT("-") : TEXT("+")));
		}
		else
		{
			ExpandText->SetText(FText::FromString(TEXT(" ")));
		}
	}
}