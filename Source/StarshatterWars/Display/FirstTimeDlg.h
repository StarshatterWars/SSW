/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025–2026.

    DIALOG:      FirstTimeDlg
    FILE:        FirstTimeDlg.h
    AUTHOR:      Carlos Bott

    OVERVIEW
    ========
    First-time player setup dialog.

    Authoritative-only implementation:
      - Reads/writes FS_PlayerGameInfo via UStarshatterPlayerSubsystem
      - Saves via UStarshatterPlayerSubsystem::SavePlayer(true)
      - Does NOT touch legacy PlayerCharacter or player.cfg
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "GameStructs.h"                 // FS_PlayerGameInfo + EEMPIRE_NAME
#include "Components/ComboBoxString.h"
#include "FirstTimeDlg.generated.h"

// UMG forward declarations:
class UButton;
class UEditableTextBox;
class UComboBoxString;

class UStarshatterPlayerSubsystem;

UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UFirstTimeDlg : public UBaseScreen
{
    GENERATED_BODY()
    friend class FFirstRunPresentationTest;

public:
    UFirstTimeDlg(const FObjectInitializer& ObjectInitializer);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnAcceptClicked();

    UFUNCTION()
    void OnCancelClicked();

private:
    UPROPERTY(Transient) TObjectPtr<class UImage> FactionLogo;
    void BuildModernDialog();
    UFUNCTION() void OnEmpireLogoChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    bool bModernDialogBuilt = false;

    // Helpers
    UStarshatterPlayerSubsystem* GetPlayerSubsystem() const;

    void PopulateDefaultsIfNeeded();
    void PopulateEmpireOptionsIfNeeded();

    // Apply UI -> PlayerInfo (authoritative)
    void ApplyUiToPlayerInfo(FS_PlayerGameInfo& Info) const;

    // Optional: ensure a save exists on first run
    void EnsureMinimalFirstRunDefaults(FS_PlayerGameInfo& Info) const;

    // Map empire combobox index -> EEMPIRE_NAME
    EEMPIRE_NAME EmpireFromComboIndex(int32 Index) const;

private:
    // ------------------------------------------------------------
    // BindWidget controls (must match names in WBP_FirstTimeDlg)
    // ------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEditableTextBox> NameEdit;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEditableTextBox> CallsignEdit;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UComboBoxString> EmpireCombo;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UComboBoxString> PlayStyleCombo;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UComboBoxString> ExperienceCombo;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> AcceptBtn;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> CancelBtn;
};
