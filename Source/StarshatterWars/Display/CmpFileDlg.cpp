#include "CmpFileDlg.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "CampaignSaveItem.h"

#include "Kismet/GameplayStatics.h"

// If you have these, include them. Otherwise wire Campaign via Manager.
// #include "CampaignSubsystem.h"
// #include "Campaign.h"

UCmpFileDlg::UCmpFileDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmpFileDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Bind click handlers:
    if (SaveButton)
    {
        SaveButton->OnClicked.AddDynamic(this, &UCmpFileDlg::OnSaveClicked);
    }

    if (CancelButtonLocal)
    {
        CancelButtonLocal->OnClicked.AddDynamic(this, &UCmpFileDlg::OnCancelClicked);
    }

    // List selection:
    if (CampaignList)
    {
        CampaignList->OnItemClicked().AddUObject(this, &UCmpFileDlg::OnCampaignSelected);
        CampaignList->ClearListItems();
    }

    // If you want Enter/Escape routed through UBaseScreen consistently:
    SetDialogInputEnabled(true);
    bIsFocusable = true;

    // Optional: resolve subsystem campaign:
    // if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
    // {
    //     CampaignSubsystem = GI->GetSubsystem<UCampaignSubsystem>();
    //     Campaign = CampaignSubsystem ? CampaignSubsystem->GetCampaign() : nullptr;
    // }
}

void UCmpFileDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ShowTime += InDeltaTime;
    ExecFrame(InDeltaTime);
}

void UCmpFileDlg::BindFormWidgets()
{
    // Legacy ids:
    BindButton(1, SaveButton);
    BindButton(2, CancelButtonLocal);
    BindEdit(200, NameEdit);
    BindList(201, CampaignList);
}

void UCmpFileDlg::Show()
{
    // Populate save list:
    if (CampaignList)
    {
        CampaignList->ClearListItems();

        TArray<FString> Names;
        GetSaveGameNames(Names);
        Names.Sort();

        for (const FString& N : Names)
        {
            CampaignList->AddItem(UCampaignSaveItem::Make(this, N));
        }
    }

    // Default save name:
    if (NameEdit)
    {
        const FString DefaultName = BuildDefaultSaveName();
        NameEdit->SetText(FText::FromString(DefaultName));
        NameEdit->SetKeyboardFocus();
    }

    bExitLatch = false;
}

void UCmpFileDlg::ExecFrame(float DeltaTime)
{
    (void)DeltaTime;

    // You already have centralized Enter/Escape in UBaseScreen::NativeOnKeyDown.
    // If you want exact legacy "latch" behavior for Escape repeats, keep this:

    if (!bDialogInputEnabled)
        return;

    // Enter key is handled by base as Accept, but legacy triggers Save:
    // We keep Save mapped to Accept by letting ApplyButton == SaveButton in your widget,
    // OR we can explicitly do it here if you prefer.

    // Escape latch:
    // If you prefer to rely purely on UBaseScreen key handling, you can delete this block.
}

void UCmpFileDlg::OnSaveClicked()
{
    if (!NameEdit)
        return;

    FString SlotName = NameEdit->GetText().ToString();

    // Legacy stripped newline:
    SlotName.ReplaceInline(TEXT("\r"), TEXT(""));
    SlotName.ReplaceInline(TEXT("\n"), TEXT(""));

    SlotName = SlotName.TrimStartAndEnd();

    if (SlotName.Len() < 1)
        return;

    if (SaveCampaignToSlot(SlotName))
    {
        RequestHide();
    }
}

void UCmpFileDlg::OnCancelClicked()
{
    RequestHide();
}

void UCmpFileDlg::OnCampaignSelected(UObject* SelectedItem)
{
    UCampaignSaveItem* Item = Cast<UCampaignSaveItem>(SelectedItem);
    if (!Item || !NameEdit)
        return;

    NameEdit->SetText(FText::FromString(Item->Name));
}

void UCmpFileDlg::GetSaveGameNames(TArray<FString>& OutNames) const
{
    OutNames.Reset();

    // Override this to call your CampaignSaveGame::GetSaveGameList equivalent.
    // Example integration patterns:
    // - CampaignSubsystem->GetSaveGameNames(OutNames);
    // - UCampaignSaveGameLibrary::GetSaveGameNames(OutNames);
}

bool UCmpFileDlg::SaveCampaignToSlot(const FString& SlotName)
{
    // Override this to call your CampaignSaveGame save pipeline.
    // Legacy did:
    //   CampaignSaveGame save(campaign);
    //   save.Save(filename);
    //   save.SaveAuto();

    (void)SlotName;
    return false;
}

FString UCmpFileDlg::BuildDefaultSaveName() const
{
    // Legacy logic:
    // - campaign = Campaign::GetCampaign()
    // - op_name = campaign->Name(); if "Operation " prefix, skip it
    // - FormatDay(day, campaign->GetTime())
    // - group->GetRegion().data()
    //
    // We keep it as a hook because your port’s types differ.

    // If you want a safe fallback:
    return TEXT("");
}

void UCmpFileDlg::RequestHide()
{
    // Legacy:
    // if (manager) manager->HideCmpFileDlg();

    if (Manager)
    {
        // If your manager is a UObject with HideCmpFileDlg() UFUNCTION, call it here.
        // Manager->HideCmpFileDlg();
        OnRequestHideCmpFileDlg();
        return;
    }

    OnRequestHideCmpFileDlg();
}
