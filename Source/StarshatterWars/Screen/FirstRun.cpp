/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           FirstRun.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    - Uses UStarshatterPlayerSubsystem as the sole save authority
    - Does NOT perform campaign initialization
    - Does NOT dictate UI close/routing behavior
=============================================================================*/

#include "FirstRun.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"

#include "Misc/DateTime.h"

#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"

#include "StarshatterPlayerSubsystem.h"
#include "GameStructs.h"

void UFirstRun::NativeConstruct()
{
    Super::NativeConstruct();

    if (btn_apply)
    {
        btn_apply->OnClicked.AddDynamic(this, &UFirstRun::OnApplyClicked);
    }

    if (btn_cancel)
    {
        btn_cancel->OnClicked.AddDynamic(this, &UFirstRun::OnCancelClicked);
    }

    if (FirstRunTitle)
    {
        FirstRunTitle->SetText(FText::FromString(TEXT("NEW PLAYER")));
    }

    if (FirstRunPrompt)
    {
        FirstRunPrompt->SetText(FText::FromString(
            TEXT("Create a new player account.\n"
                "Enter your name in the box provided.\n"
                "The user name may be a nickname, callsign, or last name.")));
    }
}

void UFirstRun::OnApplyClicked()
{
    const FString PlayerName =
        PlayerNameBox ? PlayerNameBox->GetText().ToString() : TEXT("DefaultPlayer");

    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    UStarshatterPlayerSubsystem* PlayerSS =
        GI->GetSubsystem<UStarshatterPlayerSubsystem>();

    if (!PlayerSS)
        return;

    // Reset to struct defaults, then initialize
    FS_PlayerGameInfo& Info = PlayerSS->GetMutablePlayerInfo();
    Info = FS_PlayerGameInfo();

    Info.Name = PlayerName;
    Info.Campaign = -1;

    const FDateTime Now = FDateTime::Now();
    Info.CreateTime = Now.ToUnixTimestamp();

    Info.GameTime = 0;
    Info.CampaignTime = 0;

    Info.PlayerForce = -1;
    Info.PlayerFleet = -1;
    Info.PlayerWing = -1;
    Info.PlayerCarrier = -1;
    Info.PlayerBattleGroup = -1;
    Info.PlayerDesronGroup = -1;
    Info.PlayerSquadron = -1;

    Info.PlayerShip = TEXT("Unassigned");
    Info.PlayerSystem = TEXT("Borova");
    Info.PlayerRegion = TEXT("Borova");
    Info.CampaignRowName = FName(TEXT("Operation Live Fire"));

    // Persist via subsystem ONLY
    PlayerSS->SavePlayer(true);

    // UI close / routing intentionally left to host screen
}

void UFirstRun::OnCancelClicked()
{
    if (GEngine)
    {
        GEngine->ForceGarbageCollection();
    }

    APlayerController* Player = GetOwningPlayer();
    UKismetSystemLibrary::QuitGame(this, Player, EQuitPreference::Quit, true);
}
