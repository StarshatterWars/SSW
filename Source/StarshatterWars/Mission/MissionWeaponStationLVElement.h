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

class UTextBlock;
class UMissionWeaponStationRowObject;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponStationLVElement
    : public UUserWidget
    , public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StationText = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* WeaponText = nullptr;
};