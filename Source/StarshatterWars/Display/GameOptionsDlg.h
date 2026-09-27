/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           GameOptionsDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UGameOptionsDlg
    - Gameplay options subpage hosted by UOptionsScreen.
    - AutoVBox runtime layout (RootCanvas -> AutoVBox).
    - Reads/writes legacy globals: PlayerCharacter + Ship + HUDView.
    - Apply/Cancel orchestrated by UOptionsScreen, but page supports standalone Apply/Cancel.

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

// Needed for ESelectInfo::Type
#include "Components/ComboBoxString.h"

#include "GameOptionsDlg.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;
class UVerticalBox;

class UOptionsScreen;

UCLASS()
class STARSHATTERWARS_API UGameOptionsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UGameOptionsDlg(const FObjectInitializer& ObjectInitializer);

    void Show();
    virtual void ExecFrame(double DeltaTime) override;

    void Apply();
    void Cancel();

    bool IsDirty() const { return bDirty; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

private:
    void BindDelegates();

    void RefreshFromModel();
    void PushToModel();

    void BuildGameRows();
    void BuildListsIfNeeded();

    void BuildFlightModelListIfNeeded();
    void BuildLandingModelListIfNeeded();
    void BuildFlyingStartListIfNeeded();
    void BuildAIDifficultyListIfNeeded();
    void BuildHudModeListIfNeeded();
    void BuildHudColorListIfNeeded();
    void BuildFriendlyFireListIfNeeded();
    void BuildGridModeListIfNeeded();
    void BuildGunsightListIfNeeded();

private:
    bool bClosed = true;
    bool bDelegatesBound = false;

    UPROPERTY(Transient)
    bool bDirty = false;

    // Snapshot (pulled on RefreshFromModel)
    int32 FlightModelIndex = 0;
    int32 LandingModelIndex = 0;
    int32 FlyingStartIndex = 0;
    int32 AIDifficultyIndex = 0;
    int32 HudModeIndex = 0;
    int32 HudColorIndex = 0;
    int32 FriendlyFireIndex = 0;
    int32 GridModeIndex = 0;
    int32 GunsightIndex = 0;

protected:
    // Optional description label (if present in BP)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DescriptionText;

    // Combos (bound from BP; moved into AutoVBox rows at runtime)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> FlightModelCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> FlyingStartCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> LandingsCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> AIDifficultyCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> HudModeCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> HudColorCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> FriendlyFireCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> GridModeCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> GunsightCombo;

    // Apply/Cancel
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ApplyBtn;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelBtn;

    // Tabs (optional)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> VidTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AudTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CtlTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OptTabButton; // this page
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ModTabButton;

private:
    // Apply/Cancel
    UFUNCTION() void OnApplyClicked();
    UFUNCTION() void OnCancelClicked();

    // Tabs
    UFUNCTION() void OnAudioClicked();
    UFUNCTION() void OnVideoClicked();
    UFUNCTION() void OnControlsClicked();
    UFUNCTION() void OnGameClicked();
    UFUNCTION() void OnModClicked();

    // Combo handlers
    UFUNCTION() void OnFlightModelChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnFlyingStartChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnLandingsChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnAIDifficultyChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnHudModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnHudColorChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnFriendlyFireChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnGridModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnGunsightChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
};
