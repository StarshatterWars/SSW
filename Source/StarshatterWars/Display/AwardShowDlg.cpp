/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         AwardShowDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UAwardShowDlg implementation (Unreal port)
*/

#include "AwardShowDlg.h"
#include "AwardPresentation.h"
#include "Components/CanvasPanel.h"
#include "MenuScreen.h"
#include "AwardInfoRegistry.h"

// UMG:
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"

// Input:
#include "Input/Reply.h"
#include "InputCoreTypes.h"

// Starshatter core:
#include "PlayerCharacter.h"

// If you have specific manager widget type (MenuScreen), include it and cast Manager accordingly.
// #include "MenuScreen.h"

UAwardShowDlg::UAwardShowDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UAwardShowDlg::RebuildWidget()
{
    EnsureUi();
    return Super::RebuildWidget();
}

void UAwardShowDlg::EnsureUi()
{
    if(bModernUiBuilt)return;
    if(!WidgetTree)return;
    if(!RootCanvas)
    {
        RootCanvas=Cast<UCanvasPanel>(GetRootWidget());
        if(!RootCanvas){RootCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("RootCanvas"));WidgetTree->RootWidget=RootCanvas;}
    }
    auto* Box=EnsureAutoVerticalBoxWithOffsets(FMargin(24));if(!Box)return;
    if(RootCanvas)for(auto* Child:RootCanvas->GetAllChildren())if(Child!=Box)Child->SetVisibility(ESlateVisibility::Collapsed);
    AwardPresentation::Build(WidgetTree,Box,lbl_name,lbl_info,img_rank,btn_close);
    bModernUiBuilt=true;BindFormWidgets();btn_close->OnClicked.AddUniqueDynamic(this,&UAwardShowDlg::OnCloseClicked);
}

void UAwardShowDlg::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureUi();

    if (btn_close)
    {
        btn_close->OnClicked.AddUniqueDynamic(this, &UAwardShowDlg::OnCloseClicked);
    }

    // Latch to prevent immediate close on first input frame.
    bExitLatch = true;
}

FReply UAwardShowDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept ||
        Key == EKeys::Escape || Key == EKeys::Virtual_Back)
    {
        bool bHandled = false;
        UpdateExitLatchFromInput(InKeyEvent, bHandled);
        if (bHandled)
            return FReply::Handled();
    }

    // Any other key releases latch (simple approximation of legacy key-up gating).
    bExitLatch = false;

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAwardShowDlg::UpdateExitLatchFromInput(const FKeyEvent& InKeyEvent, bool& bOutHandled)
{
    bOutHandled = false;

    const FKey Key = InKeyEvent.GetKey();

    const bool bIsAccept = (Key == EKeys::Enter || Key == EKeys::Virtual_Accept);
    const bool bIsBack = (Key == EKeys::Escape || Key == EKeys::Virtual_Back);

    if (!bIsAccept && !bIsBack)
        return;

    if (bExitLatch)
    {
        // First press while latched: consume and release.
        bExitLatch = false;
        bOutHandled = true;
        return;
    }

    OnCloseClicked();
    bOutHandled = true;
}

// --------------------------------------------------------------------
// UBaseScreen overrides
// --------------------------------------------------------------------

void UAwardShowDlg::BindFormWidgets()
{
    BindLabel(203, lbl_name);
    BindLabel(201, lbl_info);
    BindImage(202, img_rank);
    BindButton(1, btn_close);
}

FString UAwardShowDlg::GetLegacyFormText() const
{
    // You did not provide AwardShowDlg.frm in the snippet.
    // Return empty for now so BaseScreen does not attempt to parse.
    // If you paste the FORM, I’ll embed it exactly like AwardDlg.
    return FString();
}

// --------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------

void UAwardShowDlg::SetManager(UBaseScreen* InManager)
{
    Manager = InManager;
}

void UAwardShowDlg::ShowDialog()
{
    EnsureUi();
    SetVisibility(ESlateVisibility::Visible);

    ShowAward();

    bExitLatch = true;
}

void UAwardShowDlg::SetRank(int32 InRank)
{
    Rank = InRank;
    Medal = -1;
}

void UAwardShowDlg::SetMedal(int32 InMedal)
{
    Rank = -1;
    Medal = InMedal;
}

void UAwardShowDlg::ShowAward()
{
    if (Rank >= 0)
    {
        if (lbl_name)
        {
            FString TitleText = FString(TEXT("Rank of ")) + PlayerCharacter::RankName(Rank);
            lbl_name->SetText(FText::FromString(TitleText));
        }

        if (lbl_info)
        {
            // Assuming RankDescription returns FString/Text-compatible.
            lbl_info->SetText(FText::FromString(PlayerCharacter::RankDescription(Rank)));
        }

        if (img_rank)
        {
            if(const auto* Row=UAwardInfoRegistry::FindRank(Rank))
                if(auto* Texture=Row->LargeImage.LoadSynchronous())img_rank->SetBrushFromTexture(Texture);
            img_rank->SetVisibility(ESlateVisibility::Visible);
        }
    }
    else if (Medal >= 0)
    {
        if (lbl_name)
            lbl_name->SetText(FText::FromString(PlayerCharacter::MedalName(Medal)));

        if (lbl_info)
            lbl_info->SetText(FText::FromString(PlayerCharacter::MedalDescription(Medal)));

        if (img_rank)
        {
            if(const auto* Row=UAwardInfoRegistry::FindMedal(Medal))
                if(auto* Texture=Row->LargeImage.LoadSynchronous())img_rank->SetBrushFromTexture(Texture);
            img_rank->SetVisibility(ESlateVisibility::Visible);
        }
    }
    else
    {
        if (lbl_name) lbl_name->SetText(FText::GetEmpty());
        if (lbl_info) lbl_info->SetText(FText::GetEmpty());
        if (img_rank) img_rank->SetVisibility(ESlateVisibility::Hidden);
    }
}

// --------------------------------------------------------------------
// Actions
// --------------------------------------------------------------------

void UAwardShowDlg::OnCloseClicked()
{
    if(auto* P=PlayerCharacter::GetCurrentPlayer())P->ClearShowAward();
    if(auto* Menu=GetMenuManager()){Menu->ShowPlayerDlg();return;}
    SetVisibility(ESlateVisibility::Hidden);
}
