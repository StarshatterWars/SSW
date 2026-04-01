/*  Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponStationLVElement.h

    OVERVIEW
    ========
    ListView entry widget for displaying a single
    station and its selected weapon.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MissionWeaponStationLVElement.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;
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
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnEntryReleased() override;

protected:
    void ApplySelectionVisual();
    void ApplyTextRules();

protected:
    UPROPERTY(meta = (BindWidget))
    UBorder* SelectionBorder = nullptr;

    UPROPERTY(meta = (BindWidget))
    USizeBox* StationSizeBox = nullptr;

    UPROPERTY(meta = (BindWidget))
    USizeBox* WeaponSizeBox = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StationText = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* WeaponText = nullptr;

    UPROPERTY(Transient)
    UMissionWeaponStationRowObject* StationItem = nullptr;

    bool bRowSelected = false;
};