/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.
    All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         PlayerRosterEntry.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UPlayerRosterEntry (UE port)
    - Row widget for Player roster ListView
    - Implements IUserObjectListEntry so NativeOnListItemObjectSet() is valid
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"   // IMPORTANT
#include "PlayerRosterEntry.generated.h"

class UTextBlock;
class UPlayerRosterItem;
class PlayerCharacter;

// +--------------------------------------------------------------------+

UCLASS()
class STARSHATTERWARS_API UPlayerRosterEntry : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    UPlayerRosterEntry(const FObjectInitializer& ObjectInitializer);

protected:
    // IUserObjectListEntry:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
    // Bind these in your entry widget BP (or construct in C++ if you prefer).
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* txt_name = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* txt_rank = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* txt_stats = nullptr; // e.g. missions/kills, optional

private:
    void RefreshFromPlayer(PlayerCharacter* player);
};
