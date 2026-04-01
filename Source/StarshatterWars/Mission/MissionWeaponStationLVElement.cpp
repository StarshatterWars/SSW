/*  Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponStationLVElement.cpp

    OVERVIEW
    ========
    ListView entry widget for displaying a single
    station and its selected weapon.
*/

#include "MissionWeaponStationLVElement.h"
#include "MissionWeaponStationRowObject.h"

#include "Components/TextBlock.h"

void UMissionWeaponStationLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    UMissionWeaponStationRowObject* Row =
        Cast<UMissionWeaponStationRowObject>(ListItemObject);

    if (!Row)
    {
        return;
    }

    if (StationText)
    {
        StationText->SetText(FText::FromString(Row->GetStationLabel()));
    }

    if (WeaponText)
    {
        WeaponText->SetText(FText::FromString(Row->GetWeaponName()));
    }
}