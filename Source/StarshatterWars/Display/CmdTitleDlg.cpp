/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmdTitleDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdTitleDlg implementation.
*/

#include "CmdTitleDlg.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

// If you have a CampaignSubsystem class, include it; otherwise remove.
// #include "CampaignSubsystem.h"

UCmdTitleDlg::UCmdTitleDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdTitleDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // If you have a CampaignSubsystem, grab it here. If not, remove.
    // CampaignSubsystem = UGameplayStatics::GetGameInstance(this)
    //     ? UGameplayStatics::GetGameInstance(this)->GetSubsystem<UCampaignSubsystem>()
    //     : nullptr;

    // If your campaign object is owned by the subsystem, resolve it here.
    // Campaign = CampaignSubsystem ? CampaignSubsystem->GetCampaign() : nullptr;

    ShowTime = 0.0f;
    bFinished = false;
}

void UCmdTitleDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Title card screens typically want focus so Enter/Escape works uniformly:
    SetDialogInputEnabled(true);
    bIsFocusable = true;

    // If you want to force keyboard focus:
    // UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void UCmdTitleDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ExecFrame(InDeltaTime);
}

void UCmdTitleDlg::BindFormWidgets()
{
    // Bind legacy FORM ID 200 to the UMG image.
    BindImage(200, TitleImage);

    // If you also have common Apply/Cancel buttons in this screen,
    // they can be bound in the widget designer via BindWidgetOptional
    // and will automatically be used by UBaseScreen::HandleAccept/Cancel.
}

FString UCmdTitleDlg::GetLegacyFormText() const
{
    // Optional: return the raw .frm text here if you have it embedded.
    // Returning empty means "no parse/apply defaults".
    return FString();
}

void UCmdTitleDlg::SetTitleTexture(UTexture2D* InTexture)
{
    if (!TitleImage)
        return;

    TitleImage->SetBrushFromTexture(InTexture, /*bMatchSize*/ true);
}

void UCmdTitleDlg::OnTitleFinished()
{

}

void UCmdTitleDlg::ExecFrame(float DeltaTime)
{
    if (bFinished)
        return;

    ShowTime += DeltaTime;

    // Optional auto-finish behavior:
    if (AutoFinishAfterSeconds > 0.0f && ShowTime >= AutoFinishAfterSeconds)
    {
        bFinished = true;
        OnTitleFinished();
    }
}
