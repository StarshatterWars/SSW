/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSelectDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Code-built campaign selection screen.
    Replaces Blueprint widget dependencies for stability.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Text.h"
#include "List.h"
#include "GameStructs.h"
#include "CampaignSelectDlg.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UComboBoxString;
class UMenuScreen;
class UCanvasPanel;
class UBorder;
class UHorizontalBox;
class UVerticalBox;
class UWidget;
class USoundBase;

class Campaign;
class Starshatter;

template<typename T> class List;
class Bitmap;
class ThreadSync;

UCLASS()
class STARSHATTERWARS_API UCampaignSelectDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCampaignSelectDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    virtual void SetMenuManager(UMenuScreen* InManager);
    virtual void InitializeDlg(UMenuScreen* InManager);

    virtual void RegisterControls();
    virtual void ExecFrame(double DeltaTime);
    virtual bool CanClose();

    void ShowDlg();
    void HideDlg();

    UFUNCTION() virtual void OnCampaignSelect();
    UFUNCTION() virtual void OnNew();
    UFUNCTION() virtual void OnSaved();
    UFUNCTION() virtual void OnDelete();
    UFUNCTION() virtual void OnConfirmDelete();
    UFUNCTION() virtual void OnAccept();

    virtual uint32 LoadProc();

protected:
    virtual void StartLoadProc();
    virtual void StopLoadProc();
    virtual void ShowNewCampaigns();
    virtual void ShowSavedCampaigns();

protected:
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

protected:
    void BuildWidgetTreeIfNeeded();
    void HookupEvents();
    void PopulateCampaignDropdown();
    void RefreshFromSelection();
    void RefreshUIFromSubsystem();
    void UpdateCampaignButtons();
    bool DoesSelectedCampaignSaveExist() const;
    UTexture2D* LoadCampaignTexture(int32 CampaignIndex1Based) const;

    void StartSelectedCampaignFlow(bool bRestart);
    void FinishSelectedCampaignFlow(bool bRestart);
    void TryFinishCampaignLoadTransition();

    UTextBlock* CreateText(
        const FName Name,
        const FString& InText,
        int32 FontSize = 16,
        const FLinearColor& Color = FLinearColor::White,
        ETextJustify::Type Justification = ETextJustify::Left,
        bool bWrap = false);

    UBorder* CreatePanelBorder(const FName Name, const FLinearColor& Color);

    UWidget* CreateMenuButton(
        const FName Name,
        TObjectPtr<UButton>& OutButton,
        TObjectPtr<UTextBlock>& OutText,
        const FString& Label);

    void ApplyButtonStyle(UButton* Button) const;
    void PlayUISound(UObject* WorldContext, USoundBase* UISound);

    UFUNCTION() void OnPlayButtonClicked();
    UFUNCTION() void OnPlayButtonHovered();
    UFUNCTION() void OnPlayButtonUnHovered();

    UFUNCTION() void OnRestartButtonClicked();
    UFUNCTION() void OnRestartButtonHovered();
    UFUNCTION() void OnRestartButtonUnHovered();

    UFUNCTION() void OnCancelButtonClicked();
    UFUNCTION() void OnCancelButtonHovered();
    UFUNCTION() void OnCancelButtonUnHovered();

    UFUNCTION() void OnSetSelected(FString SelectedItem, ESelectInfo::Type Type);

protected:
    FTimerHandle CampaignLoadFinishTimer;

protected:
    UPROPERTY(Transient) TObjectPtr<UMenuScreen> manager = nullptr;
    UPROPERTY(Transient) TObjectPtr<UImage> BackgroundImage = nullptr;
    UPROPERTY(Transient) TObjectPtr<UBorder> MainBorder = nullptr;

    UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerNameText = nullptr;

    UPROPERTY(Transient) TObjectPtr<UComboBoxString> CampaignSelectDD = nullptr;
    UPROPERTY(Transient) TObjectPtr<UImage> CampaignImage = nullptr;

    UPROPERTY(Transient) TObjectPtr<UTextBlock> CampaignNameText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DescriptionText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SituationText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Orders1Text = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Orders2Text = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Orders3Text = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Orders4Text = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LocationSystemText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CampaignStartTimeText = nullptr;

    UPROPERTY(Transient) TObjectPtr<UButton> PlayButton = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> RestartButton = nullptr;

    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayButtonText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RestartButtonText = nullptr;

    UPROPERTY(EditAnywhere, Category = "UI Sound")
    TObjectPtr<USoundBase> HoverSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "UI Sound")
    TObjectPtr<USoundBase> AcceptSound = nullptr;

protected:
    Starshatter* stars = nullptr;
    Campaign* campaign = nullptr;
    int selected_mission = 0;

    void* hproc = nullptr;
    ThreadSync sync;

    bool loading = false;
    bool loaded = false;

    Text load_file;
    int load_index = -1;
    bool show_saved = false;

    List<Bitmap> images;
    Text select_msg;

    int32 Selected = INDEX_NONE;
    FName PickedRowName = NAME_None;
    TArray<FName> CampaignRowNamesByOptionIndex;
    TArray<int32> CampaignIndexByOptionIndex;
};