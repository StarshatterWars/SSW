/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         DialogButtonRail.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation of reusable left-side button rail panel.

    Builds a vertical layout with:
    - top dynamic buttons
    - spacer to push bottom buttons
    - optional Accept / Cancel buttons

    Ensures consistent UI behavior across multiple dialogs.
*/

#include "DialogButtonRail.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Spacer.h"

#include "MenuButton.h"

// +--------------------------------------------------------------------+

void UDialogButtonRail::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRoot();
    RebuildRail();
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::BuildRoot()
{
    if (!WidgetTree || RootBorder)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("RailRootBorder"));

    ButtonContainer = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(),
        TEXT("ButtonContainer"));

    RootBorder->SetContent(ButtonContainer);

    // Padding inside panel
    RootBorder->SetPadding(FMargin(8.f, 8.f));

    // ------------------------------------------------------------
    // APPLY PANEL BRUSH (Option 2)
    // ------------------------------------------------------------

    if (PanelBrush.GetResourceObject())
    {
        RootBorder->SetBrush(PanelBrush);
    }
    else
    {
        // fallback color (dark blue-gray)
        RootBorder->SetBrushColor(FLinearColor(0.08f, 0.10f, 0.14f, 1.0f));
    }

    WidgetTree->RootWidget = RootBorder;
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::SetMenuButtonClass(TSubclassOf<UMenuButton> InClass)
{
    MenuButtonClass = InClass;
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::SetTopButtons(const TArray<FDialogButtonSpec>& InButtons)
{
    TopButtons = InButtons;
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::SetShowBottomButtons(bool bInShowAccept, bool bInShowCancel)
{
    bShowAccept = bInShowAccept;
    bShowCancel = bInShowCancel;
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::SetButtonSize(float InWidth, float InHeight)
{
    ButtonWidth = InWidth;
    ButtonHeight = InHeight;
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::ClearRail()
{
    ButtonMap.Empty();
    AcceptButton = nullptr;
    CancelButton = nullptr;

    if (ButtonContainer)
    {
        ButtonContainer->ClearChildren();
    }
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::RebuildRail()
{
    if (!ButtonContainer || !MenuButtonClass)
    {
        return;
    }

    ClearRail();

    for (const FDialogButtonSpec& Spec : TopButtons)
    {
        UMenuButton* Btn = CreateButton(Spec);
        AddTopButton(Btn);
    }

    FillSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());

    if (UVerticalBoxSlot* SpacerSlot = ButtonContainer->AddChildToVerticalBox(FillSpacer))
    {
        SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    if (bShowAccept)
    {
        AcceptButton = CreateButton({ TEXT("Accept"), TEXT("ACCEPT") });
        AddBottomButton(AcceptButton);
    }

    if (bShowCancel)
    {
        CancelButton = CreateButton({ TEXT("Cancel"), TEXT("CANCEL") });
        AddBottomButton(CancelButton);
    }
}

// +--------------------------------------------------------------------+

UMenuButton* UDialogButtonRail::CreateButton(const FDialogButtonSpec& Spec)
{
    if (!WidgetTree || !MenuButtonClass)
    {
        return nullptr;
    }

    UMenuButton* Btn =
        WidgetTree->ConstructWidget<UMenuButton>(MenuButtonClass, Spec.Id);

    if (!Btn)
    {
        return nullptr;
    }

    Btn->SetMenuOption(Spec.Label);
    Btn->SetButtonText(FText::FromString(Spec.Label));
    Btn->SetButtonSize(ButtonWidth, ButtonHeight);

    BindButton(Btn, Spec.Id);

    return Btn;
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::AddTopButton(UMenuButton* Button)
{
    if (!ButtonContainer || !Button)
    {
        return;
    }

    if (UVerticalBoxSlot* VBoxSlot = ButtonContainer->AddChildToVerticalBox(Button))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, ButtonSpacing));
        VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        VBoxSlot->SetHorizontalAlignment(HAlign_Left);
    }
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::AddBottomButton(UMenuButton* Button)
{
    if (!ButtonContainer || !Button)
    {
        return;
    }

    if (UVerticalBoxSlot* BottomSlot = ButtonContainer->AddChildToVerticalBox(Button))
    {
        BottomSlot->SetPadding(FMargin(0.f, 0.f, 0.f, ButtonSpacing));
        BottomSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BottomSlot->SetHorizontalAlignment(HAlign_Left);
    }
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::BindButton(UMenuButton* Button, FName Id)
{
    if (!Button)
    {
        return;
    }

    ButtonMap.Add(Button, Id);
    Button->OnSelected.AddUniqueDynamic(this, &UDialogButtonRail::HandleButtonSelected);
}

// +--------------------------------------------------------------------+

void UDialogButtonRail::HandleButtonSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    if (const FName* Found = ButtonMap.Find(SelectedButton))
    {
        OnButtonClicked.Broadcast(*Found);
    }
}