/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpFileDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpFileDlg
    - Unreal port of legacy CmpFileDlg (campaign save dialog).
    - Uses UBaseScreen FORM ID binding and unified Enter/Escape handling.
    - Populates a ListView with save-game names; selecting one fills the EditBox.
    - Save writes the current campaign to the provided slot name.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmpFileDlg.generated.h"

class UButton;
class UEditableTextBox;
class UListView;

class UCmpnScreen;
class UCampaignSubsystem;
class Campaign;
class UCampaignSaveItem;

UCLASS()
class STARSHATTERWARS_API UCmpFileDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmpFileDlg(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // Legacy-equivalent API
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() override;

    /** Call after construct/add-to-viewport: populates list + sets default save name */
    UFUNCTION(BlueprintCallable, Category = "CmpFileDlg")
    void Show();

    /** Legacy ExecFrame equivalent */
    virtual void ExecFrame(float DeltaTime);

    /** Legacy manager hook (CmpnScreen*) */
    void SetManager(UCmpnScreen* InManager) { Manager = InManager; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // ----------------------------------------------------------------
    // Widgets (IDs match legacy)
    // ----------------------------------------------------------------

    /** id 1 */
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* SaveButton = nullptr;

    /** id 2 */
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* CancelButtonLocal = nullptr; // avoid name collision with UBaseScreen::CancelButton optional

    /** id 200 */
    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* NameEdit = nullptr;

    /** id 201 - Unreal replacement for ListBox */
    UPROPERTY(meta = (BindWidgetOptional))
    UListView* CampaignList = nullptr;

protected:
    // ----------------------------------------------------------------
    // Event handlers (legacy OnSave/OnCancel/OnCampaign)
    // ----------------------------------------------------------------
    UFUNCTION()
    void OnSaveClicked();

    UFUNCTION()
    void OnCancelClicked();

    UFUNCTION()
    void OnCampaignSelected(UObject* SelectedItem);

protected:
    // ----------------------------------------------------------------
    // Save system hooks (override to integrate your CampaignSaveGame code)
    // ----------------------------------------------------------------
    virtual void GetSaveGameNames(TArray<FString>& OutNames) const;

    /** Returns true if saved successfully */
    virtual bool SaveCampaignToSlot(const FString& SlotName);

    /** Provides default save name like "OpName Day (Region)" */
    virtual FString BuildDefaultSaveName() const;

protected:
    // ----------------------------------------------------------------
    // Manager routing (legacy HideCmpFileDlg)
    // ----------------------------------------------------------------
    void RequestHide();

    UFUNCTION(BlueprintImplementableEvent, Category = "CmpFileDlg")
    void OnRequestHideCmpFileDlg();

protected:
    // ----------------------------------------------------------------
    // State
    // ----------------------------------------------------------------
    UCmpnScreen* Manager = nullptr;
    UCampaignSubsystem* CampaignSubsystem = nullptr;
    Campaign* Campaign = nullptr;

    bool bExitLatch = false;

    UPROPERTY(Transient)
    float ShowTime = 0.0f;
};
