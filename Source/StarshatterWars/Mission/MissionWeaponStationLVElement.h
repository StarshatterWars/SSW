/*  Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponStationLVElement.h

    OVERVIEW
    ========
    Interactive station row widget for runtime MissionLoad editing.

    Displays:
      - Station label
      - Current weapon text
      - Combo box of allowed weapons
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "MissionWeaponStationLVElement.generated.h"

class UBorder;
class UComboBoxString;
class USizeBox;
class UTextBlock;
class UObject;
class UMissionWeaponDlg;
class UMissionWeaponStationRowObject;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponStationLVElement
    : public UUserWidget
    , public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    void SetOwningWeaponDlg(UMissionWeaponDlg* InDlg) { OwningWeaponDlg = InDlg; }

protected:
    UFUNCTION()
    void HandleWeaponSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    void ApplyTextRules();
    void ApplySelectionVisual();

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* RowBorder = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* StationSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* WeaponSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* StationText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* WeaponText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UComboBoxString* WeaponCombo = nullptr;

    UPROPERTY()
    UMissionWeaponStationRowObject* StationItem = nullptr;

    UPROPERTY()
    UMissionWeaponDlg* OwningWeaponDlg = nullptr;

    bool bRowSelected = false;
    bool bUpdatingCombo = false;
};