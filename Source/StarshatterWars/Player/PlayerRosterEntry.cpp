/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.
    All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         PlayerRosterEntry.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UPlayerRosterEntry row widget implementation
*/

#include "PlayerRosterEntry.h"

#include "Components/TextBlock.h"

#include "PlayerRosterItem.h"
#include "PlayerCharacter.h"

UPlayerRosterEntry::UPlayerRosterEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPlayerRosterEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    UPlayerRosterItem* item = Cast<UPlayerRosterItem>(ListItemObject);
    PlayerCharacter* player = item ? item->GetPlayer() : nullptr;

    RefreshFromPlayer(player);
}

void UPlayerRosterEntry::RefreshFromPlayer(PlayerCharacter* player)
{
    if (!player)
    {
        if (txt_name)  txt_name->SetText(FText::GetEmpty());
        if (txt_rank)  txt_rank->SetText(FText::GetEmpty());
        if (txt_stats) txt_stats->SetText(FText::GetEmpty());
        return;
    }

    // Name() and Rank() in your legacy code may be const char* OR Rank may be int.
    // DO NOT use StringCast on ints. Convert explicitly.

    if (txt_name)
        txt_name->SetText(FText::FromString(*player->Name()));

    if (txt_rank)
    {
        // If Rank() returns const char*:
        // txt_rank->SetText(FText::FromString(ANSI_TO_TCHAR(player->Rank())));

        // If Rank() returns int:
        const int32 RankVal = (int32)player->GetRank();
        txt_rank->SetText(FText::AsNumber(RankVal));
    }

    if (txt_stats)
    {
        // Example: "M 12  K 5  D 1"
        const int32 Missions = (int32)player->Missions();
        const int32 Kills = (int32)player->Kills();
        const int32 Deaths = (int32)player->Deaths();

        const FString Stats = FString::Printf(TEXT("M %d  K %d  D %d"), Missions, Kills, Deaths);
        txt_stats->SetText(FText::FromString(Stats));
    }
}
