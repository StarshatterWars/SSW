/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSelectDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Converted from FormWindow/AWEvent mapping to UBaseScreen (UUserWidget-derived).
    - Preserves original member names and intent where applicable.
    - Removes MemDebug and allocation tags.
    - Converts Print-style debugging to UE_LOG.
*/

#include "CampaignSelectDlg.h"

#include "GameStructs.h"

// Unreal:
#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

// Starshatter (ported core/gameplay):
#include "ConfirmDlg.h"
#include "MenuScreen.h"
#include "Starshatter.h"
#include "Campaign.h"
#include "CampaignSaveGame.h"
#include "CombatGroup.h"
#include "ShipDesign.h"
#include "PlayerCharacter.h"

#include "Game.h"
#include "DataLoader.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "ParseUtil.h"
#include "FormatUtil.h"

#include "TimerSubsystem.h"
#include "SSWGameInstance.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"

#include "CampaignSave.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"

// +--------------------------------------------------------------------+

UCampaignSelectDlg::UCampaignSelectDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // stars/select_msg initialized in NativeOnInitialized to ensure systems exist.
}

void UCampaignSelectDlg::NativePreConstruct()
{
    Super::NativePreConstruct();
    SetCampaignDDList();
}

void UCampaignSelectDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    stars = Starshatter::GetInstance();
    select_msg = Game::GetText("CmpSelectDlg.select_msg");

    RegisterControls();
}

void UCampaignSelectDlg::NativeConstruct()
{
    Super::NativeConstruct();

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] NativeConstruct: GameInstance is NULL"));
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] NativeConstruct: PlayerSubsystem is NULL"));
        return;
    }

    // Player save should already be loaded by Boot, but allow a safe fallback:
    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

   if (auto* StyleSS = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
    {
        StyleSS->ApplyMenuButtonStyle(CancelButton);
        StyleSS->ApplyMenuButtonStyle(PlayButton);
        StyleSS->ApplyMenuButtonStyle(RestartButton);
    }

    const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

    UStarshatterGameDataSubsystem* DataSubsystem =
        GI->GetSubsystem<UStarshatterGameDataSubsystem>();

    if (!DataSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] NativeConstruct: GameDataSubsystem is NULL"));
        return;
    }

    const TArray<FS_Campaign>& Campaigns = DataSubsystem->GetAllCampaigns();
    UE_LOG(LogTemp, Log, TEXT("[CampaignScreen] NativeConstruct: Campaign count = %d"), Campaigns.Num());

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("DYNAMIC CAMPAIGNS")));
    }

    // Buttons
    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonClicked);
        CancelButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonClicked);

        CancelButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonHovered);
        CancelButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonHovered);

        CancelButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonUnHovered);
        CancelButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonUnHovered);

        if (CancelButtonText)
        {
            CancelButtonText->SetText(FText::FromString(TEXT("CANCEL")));
        }
    }

    if (PlayButton)
    {
        PlayButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonClicked);
        PlayButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonClicked);

        PlayButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonHovered);
        PlayButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonHovered);

        PlayButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonUnHovered);
        PlayButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonUnHovered);

        if (PlayButtonText)
        {
            // Will be updated by UpdateCampaignButtons()
            PlayButtonText->SetText(FText::FromString(TEXT("START")));
        }
    }

    if (RestartButton)
    {
        RestartButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonClicked);
        RestartButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonClicked);

        RestartButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonHovered);
        RestartButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonHovered);

        RestartButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonUnHovered);
        RestartButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonUnHovered);

        if (RestartButtonText)
        {
            RestartButtonText->SetText(FText::FromString(TEXT("RESTART")));
        }
    }

    // Dropdown
    if (CampaignSelectDD)
    {
        CampaignSelectDD->OnSelectionChanged.RemoveDynamic(this, &UCampaignSelectDlg::OnSetSelected);
        CampaignSelectDD->OnSelectionChanged.AddDynamic(this, &UCampaignSelectDlg::OnSetSelected);
    }

    // Player name
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));
        UE_LOG(LogTemp, Log, TEXT("[CampaignScreen] Player Name: %s"), *PlayerInfo.Name);
    }

    // Restore selection: PlayerInfo.Campaign is ALWAYS 1-based campaign index
    int32 SelectedOptionIndex = 0;
    if (PlayerInfo.Campaign > 0)
    {
        const int32 Found = CampaignIndexByOptionIndex.IndexOfByKey(PlayerInfo.Campaign);
        if (Found != INDEX_NONE)
        {
            SelectedOptionIndex = Found;
        }
    }

    Selected = SelectedOptionIndex;

    PickedRowName = CampaignRowNamesByOptionIndex[Selected + 1];
   
    UE_LOG(LogTemp, Log, TEXT("[CampaignScreen] NativeConstruct: Selected=%d Row=%s"),
        Selected,
        *CampaignRowNamesByOptionIndex[Selected].ToString());
    
    CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    if (CampaignSelectDD && CampaignRowNamesByOptionIndex.IsValidIndex(SelectedOptionIndex))
    {
        // Programmatic selection; OnSetSelected should ignore Direct if needed
        CampaignSelectDD->SetSelectedIndex(SelectedOptionIndex);
    }

    // Update right panel and buttons
    SetSelectedData(Selected);
    UpdateCampaignButtons();
}

void UCampaignSelectDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    //ExecFrame(InDeltaTime);
}

void UCampaignSelectDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCampaignSelectDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}
// +--------------------------------------------------------------------+

void UCampaignSelectDlg::RegisterControls()
{
    // List selection:
    // UListView selection is UObject-driven. You should bind OnItemSelectionChanged and
    // map it to OnCampaignSelect(). For now, we keep the method and invoke it from your
    // entry widget / list item logic.
    //
    // Example (if you use UObject items):
    // lst_campaigns->OnItemSelectionChanged().AddUObject(this, &UCampaignSelectDlg::HandleSelectionChanged);

    ShowNewCampaigns();
}

UTexture2D* UCampaignSelectDlg::LoadTextureFromFile()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
    {
        return nullptr;
    }

    return GI->LoadPNGTextureFromFile(ImagePath);
}

FSlateBrush UCampaignSelectDlg::CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize)
{
    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = ImageSize;
    Brush.DrawAs = ESlateBrushDrawType::Image;
    return Brush;
}
void UCampaignSelectDlg::OnPlayButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[UCampaignSelectDlg] Player Button Clicked BEGIN"));
    PlayUISound(this, AcceptSound);

    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
        return;

    UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
        return;

    USSWGameInstance* GI = Cast<USSWGameInstance>(GIBase);
    if (!GI)
        return;

    UStarshatterGameDataSubsystem* DataSubsystem =
        GIBase->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
        return;

    if (PickedRowName.IsNone())
        return;

    const int32 CampaignIndex1Based =
        CampaignIndexByOptionIndex.IsValidIndex(Selected)
        ? CampaignIndexByOptionIndex[Selected]
        : (Selected + 1);

    const FS_Campaign* CampaignData =
        DataSubsystem->GetCampaignByIndex1Based(CampaignIndex1Based);

    if (!CampaignData)
        return;

    if (!Campaign::SelectFromData(*CampaignData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Campaign] Failed to activate runtime campaign '%s'"),
            *CampaignData->Name);
        return;
    }

    // CRITICAL FIX: START THE CAMPAIGN
    Campaign* CampaignPtr = Campaign::GetCampaign();
    if (CampaignPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Campaign] Starting campaign"));
        CampaignPtr->Start();
    }

    // SET ACTIVE CAMPAIGN INDEX FOR GAME DATA SUBSYSTEM
    DataSubsystem->CampaignIndex = CampaignIndex1Based - 1;

    // BUILD RUNTIME COMBAT ROSTER NOW THAT CAMPAIGN EXISTS
    UE_LOG(LogTemp, Warning, TEXT("[Campaign] Building combat roster from data tables"));
    DataSubsystem->BuildCombatRosterFromDataTables();

    GI->SelectedCampaignDisplayName =
        CampaignSelectDD ? CampaignSelectDD->GetSelectedOption() : TEXT("");

    GI->SelectedCampaignIndex = CampaignIndex1Based;
    GI->SelectedCampaignRowName = PickedRowName;

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    {
        FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
        PlayerInfo.Campaign = CampaignIndex1Based;
        PlayerInfo.CampaignRowName = PickedRowName;
        PlayerSS->SavePlayer(true);
    }

    const bool bHasSave = DoesSelectedCampaignSaveExist();

    if (bHasSave)
    {
        GI->LoadOrCreateSelectedCampaignSave();
    }
    else
    {
        GI->CreateNewCampaignSave(
            GI->SelectedCampaignIndex,
            GI->SelectedCampaignRowName,
            GI->SelectedCampaignDisplayName
        );
    }

    if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
    {
        Timer->SetCampaignSave(GI->CampaignSave);

        if (!bHasSave)
        {
            Timer->RestartCampaignClock(true);
        }
    }

    Mouse::Show(false);

    if (stars)
        stars->SetGameMode(EGameMode::CLOD);

    manager->ShowOperationsDlg();
}

void UCampaignSelectDlg::OnRestartButtonClicked()
{
    PlayUISound(this, AcceptSound);

    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
        return;

    UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
        return;

    USSWGameInstance* GI = Cast<USSWGameInstance>(GIBase);
    if (!GI)
        return;

    UStarshatterGameDataSubsystem* DataSubsystem =
        GIBase->GetSubsystem<UStarshatterGameDataSubsystem>();

    if (!DataSubsystem || PickedRowName.IsNone())
        return;

    const int32 CampaignIndex1Based =
        CampaignIndexByOptionIndex.IsValidIndex(Selected)
        ? CampaignIndexByOptionIndex[Selected]
        : (Selected + 1);

    const FS_Campaign* CampaignData =
        DataSubsystem->GetCampaignByIndex1Based(CampaignIndex1Based);

    if (!CampaignData)
        return;

    if (!Campaign::SelectFromData(*CampaignData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Campaign] Failed to restart campaign '%s'"),
            *CampaignData->Name);
        return;
    }

    // CRITICAL FIX: START THE CAMPAIGN
    Campaign* CampaignPtr = Campaign::GetCampaign();
    if (CampaignPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Campaign] Restarting campaign"));
        CampaignPtr->Start();
    }

    // =========================
    // Existing restart logic
    // =========================

    GI->SelectedCampaignDisplayName =
        CampaignSelectDD ? CampaignSelectDD->GetSelectedOption() : TEXT("");

    GI->SelectedCampaignIndex = CampaignIndex1Based;
    GI->SelectedCampaignRowName = PickedRowName;

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    {
        FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
        PlayerInfo.Campaign = CampaignIndex1Based;
        PlayerInfo.CampaignRowName = PickedRowName;
        PlayerSS->SavePlayer(true);
    }

    GI->CreateNewCampaignSave(
        GI->SelectedCampaignIndex,
        GI->SelectedCampaignRowName,
        GI->SelectedCampaignDisplayName
    );

    if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
    {
        Timer->SetCampaignSave(GI->CampaignSave);
        Timer->RestartCampaignClock(true);
    }

    manager->ShowOperationsDlg();
}

void UCampaignSelectDlg::OnPlayButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnPlayButtonUnHovered()
{
}

void UCampaignSelectDlg::OnRestartButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnRestartButtonUnHovered()
{
}

void UCampaignSelectDlg::OnCancelButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnCancelButtonUnHovered()
{
}

void UCampaignSelectDlg::SetCampaignDDList()
{
    UStarshatterGameDataSubsystem* DataSubsystem =
        GetGameInstance() ? GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>() : nullptr;

    if (!CampaignSelectDD)
    {
        UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] SetCampaignDDList: CampaignSelectDD is NULL"));
        return;
    }

    if (!DataSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] SetCampaignDDList: GameDataSubsystem is NULL"));
        return;
    }

    CampaignSelectDD->ClearOptions();
    CampaignSelectDD->ClearSelection();

    CampaignRowNamesByOptionIndex.Reset();
    CampaignIndexByOptionIndex.Reset();

    const TArray<FS_Campaign>& Campaigns = DataSubsystem->GetAllCampaigns();

    for (const FS_Campaign& Row : Campaigns)
    {
        if (!Row.bAvailable)
        {
            continue;
        }

        // Assumes FS_Campaign preserves its source row name:
        CampaignRowNamesByOptionIndex.Add(Row.RowName);

        // Store 1-based stable campaign index:
        CampaignIndexByOptionIndex.Add(Row.Index + 1);

        CampaignSelectDD->AddOption(Row.Name);
    }

    UE_LOG(LogTemp, Log, TEXT("[CampaignScreen] SetCampaignDDList: Added %d campaigns"),
        CampaignRowNamesByOptionIndex.Num());
}

void UCampaignSelectDlg::SetSelectedData(int32 OptionIndex)
{
    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
    {
        UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] SetSelectedData: GameInstance is NULL"));
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem =
        GIBase->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] SetSelectedData: GameDataSubsystem is NULL"));
        return;
    }

    Selected = OptionIndex;

    if (!CampaignIndexByOptionIndex.IsValidIndex(Selected))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CampaignScreen] SetSelectedData: Invalid option index %d"), Selected);
        return;
    }

    const int32 CampaignIndex1Based = CampaignIndexByOptionIndex[Selected];

    const FS_Campaign* CampaignData = DataSubsystem->GetCampaignByIndex1Based(CampaignIndex1Based);
    if (!CampaignData)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignScreen] SetSelectedData: No campaign found for index %d"),
            CampaignIndex1Based);
        return;
    }

    if (!Campaign::SelectFromData(*CampaignData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignScreen] SetSelectedData: Failed to create runtime campaign from '%s'"),
            *CampaignData->Name);
        return;
    }

    Campaign* ActiveCampaign = Campaign::GetCampaign();
    if (ActiveCampaign)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[CampaignScreen] Active runtime campaign='%s' missionCount=%d"),
            ANSI_TO_TCHAR(ActiveCampaign->Name()),
            ActiveCampaign->GetMissionList().size());
    }

    TArray<FString> Orders = CampaignData->Orders;
    Orders.SetNum(4);

    if (CampaignImage)
    {
        UTexture2D* Texture = CampaignData->CampaignImage.LoadSynchronous();

        if (Texture)
        {
            CampaignImage->SetBrushFromTexture(Texture, true);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignScreen] Failed to load image for '%s'"),
                *CampaignData->Name);

            CampaignImage->SetBrush(FSlateBrush());
        }
    }

    if (CampaignNameText)
    {
        CampaignNameText->SetText(FText::FromString(CampaignData->Name));
    }

    if (CampaignStartTimeText)
    {
        CampaignStartTimeText->SetText(FText::FromString(CampaignData->Start));
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(CampaignData->Description));
    }

    if (SituationText)
    {
        SituationText->SetText(FText::FromString(CampaignData->Situation));
    }

    if (Orders1Text)
    {
        Orders1Text->SetText(FText::FromString(Orders[0]));
    }

    if (Orders2Text)
    {
        Orders2Text->SetText(FText::FromString(Orders[1]));
    }

    if (Orders3Text)
    {
        Orders3Text->SetText(FText::FromString(Orders[2]));
    }

    if (Orders4Text)
    {
        Orders4Text->SetText(FText::FromString(Orders[3]));
    }

    if (LocationSystemText)
    {
        const FString LocationText = CampaignData->System + TEXT("/") + CampaignData->Region;
        LocationSystemText->SetText(FText::FromString(LocationText));
    }

    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    UE_LOG(LogTemp, Log,
        TEXT("[CampaignScreen] SetSelectedData: Selected=%d Campaign=%s Row=%s Index=%d"),
        Selected,
        *CampaignData->Name,
        *PickedRowName.ToString(),
        CampaignIndex1Based);
}

void UCampaignSelectDlg::OnSetSelected(FString SelectedItem, ESelectInfo::Type Type)
{
    // Ignore programmatic SetSelectedIndex calls
    if (Type == ESelectInfo::Direct)
        return;

    if (!CampaignSelectDD)
        return;

    const int32 NewIndex = CampaignSelectDD->FindOptionIndex(SelectedItem);
    if (NewIndex == INDEX_NONE)
        return;

    Selected = NewIndex;

    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(NewIndex)
        ? CampaignRowNamesByOptionIndex[NewIndex]
        : NAME_None;

    SetSelectedData(NewIndex);
    UpdateCampaignButtons();
}

void UCampaignSelectDlg::GetCampaignImageFile(int32 OptionIndex)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI || !GI->CampaignData.IsValidIndex(OptionIndex))
    {
        ImagePath.Empty();
        return;
    }

    // This is UI folder convention based on dropdown ordering (legacy)
    // If you prefer folder to follow campaign index (1-based), switch to CampaignIndexByOptionIndex[OptionIndex].
    ImagePath = FPaths::ProjectContentDir() + TEXT("UI/Campaigns/0");
    ImagePath.Append(FString::FromInt(OptionIndex + 1));
    ImagePath.Append(TEXT("/"));
    ImagePath.Append(GI->CampaignData[OptionIndex].MainImage);
    ImagePath.Append(TEXT(".png"));

    UE_LOG(LogTemp, Log, TEXT("Campaign Image: %s"), *ImagePath);
}



void UCampaignSelectDlg::PlayUISound(UObject* WorldContext, USoundBase* UISound)
{
    if (UISound)
    {
        UGameplayStatics::PlaySound2D(WorldContext, UISound);
    }
}

bool UCampaignSelectDlg::DoesSelectedCampaignSaveExist() const
{
    const USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return false;

    if (PickedRowName.IsNone())
        return false;

    const FString GameSlot = UCampaignSave::MakeSlotNameFromRowName(PickedRowName);
    constexpr int32 UserIndex = 0;
    return UGameplayStatics::DoesSaveGameExist(GameSlot, UserIndex);
}

void UCampaignSelectDlg::UpdateCampaignButtons()
{
    const bool bHasSave = DoesSelectedCampaignSaveExist();

    if (PlayButton)
    {
        PlayButton->SetIsEnabled(true);
    }

    if (PlayButtonText)
    {
        PlayButtonText->SetText(FText::FromString(bHasSave ? TEXT("CONTINUE") : TEXT("START")));
    }

    if (RestartButton)
    {
        RestartButton->SetIsEnabled(bHasSave);
    }
}


// +--------------------------------------------------------------------+

void UCampaignSelectDlg::ExecFrame(double DeltaTime)
{
    if (Keyboard::KeyDown(VK_RETURN)) {
        if (btn_accept && btn_accept->GetIsEnabled()) {
            OnAccept();
        }
    }

    AutoThreadSync a(sync);

    if (loaded) {
        loaded = false;

        if (btn_cancel)
            btn_cancel->SetIsEnabled(true);

        if (description && btn_accept) {
            if (campaign) {
                Campaign::SelectCampaign(campaign->Name());

                if (load_index >= 0) {
                    // In classic UI, this updated the ListBox image at load_index.
                    // With UListView, images are part of the item object / entry widget.
                    // Keep the image-copy logic for the legacy Bitmap list:
                    if (load_index >= 0 && load_index < images.size()) {
                        images[load_index]->CopyBitmap(*campaign->GetImage(1));
                    }

                    description->SetText(FText::FromString(
                        UTF8_TO_TCHAR(
                            (Text("<font Limerick12><color ffffff>") +
                                campaign->Name() +
                                Text("<font Verdana>\n\n") +
                                Text("<color ffff80>") +
                                Game::GetText("CmpSelectDlg.scenario") +
                                Text("<color ffffff>\n\t") +
                                campaign->Description()).data()
                        )
                    ));
                }
                else {
                    char time_buf[32];
                    char score_buf[32];

                    double t = campaign->GetLoadTime() - campaign->GetStartTime();
                    FormatDayTime(time_buf, t);

                    sprintf_s(score_buf, "%d", campaign->GetPlayerTeamScore());

                    Text desc = Text("<font Limerick12><color ffffff>") +
                        campaign->Name() +
                        Text("<font Verdana>\n\n") +
                        Text("<color ffff80>") +
                        Game::GetText("CmpSelectDlg.scenario") +
                        Text("<color ffffff>\n\t") +
                        campaign->Description() +
                        Text("\n\n<color ffff80>") +
                        Game::GetText("CmpSelectDlg.campaign-time") +
                        Text("<color ffffff>\n\t") +
                        time_buf +
                        Text("\n\n<color ffff80>") +
                        Game::GetText("CmpSelectDlg.assignment") +
                        Text("<color ffffff>\n\t");

                    if (campaign->GetPlayerGroup())
                        desc += campaign->GetPlayerGroup()->GetDescription();
                    else
                        desc += "n/a";

                    desc += Text("\n\n<color ffff80>") +
                        Game::GetText("CmpSelectDlg.team-score") +
                        Text("<color ffffff>\n\t") +
                        score_buf;

                    description->SetText(FText::FromString(UTF8_TO_TCHAR(desc.data())));
                }

                btn_accept->SetIsEnabled(true);

                if (btn_delete)
                    btn_delete->SetIsEnabled(show_saved);
            }
            else {
                description->SetText(FText::FromString(UTF8_TO_TCHAR(select_msg.data())));
                btn_accept->SetIsEnabled(true);
            }
        }
    }
}

bool UCampaignSelectDlg::CanClose()
{
    AutoThreadSync a(sync);
    return !loading;
}

// +--------------------------------------------------------------------+

void UCampaignSelectDlg::ShowNewCampaigns()
{
    AutoThreadSync a(sync);

    if (loading && description) {
        description->SetText(FText::FromString(UTF8_TO_TCHAR(Game::GetText("CmpSelectDlg.already-loading").data())));
        // Button::PlaySound(Button::SND_REJECT); // classic UI sound; hook into your UE sound layer
        return;
    }

    // UMG button visual state is handled by styles; we keep logical intent only.

    if (btn_delete)
        btn_delete->SetIsEnabled(false);

    if (lst_campaigns) {
        images.destroy();

        // UListView population is UObject-driven; you will create item objects for each entry.
        // We keep legacy Bitmap generation here for later entry widgets to reference.

        PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();
        if (!player)
            return;

        ListIter<Campaign> iter = Campaign::GetAllCampaigns();
        while (++iter) {
            Campaign* c = iter.value();

            if (c->GetCampaignId() < Campaign::SINGLE_MISSIONS) {
                Bitmap* bmp = new Bitmap;
                bmp->CopyBitmap(*c->GetImage(0));
                images.append(bmp);

                // ListView item creation is TODO: create a UObject item holding name + bmp index.
                // Example: UCampaignSelectItem* Item = NewObject<UCampaignSelectItem>(this); ...
                // lst_campaigns->AddItem(Item);

                // FULL GAME CRITERIA (based on player record):
                const int cid = c->GetCampaignId();
                const bool locked_full =
                    (cid > 2 && cid < 10 && !player->HasCompletedCampaign(cid - 1));

                const bool locked_extra =
                    (cid >= 10 && cid < 30 && (cid % 10) != 0 && !player->HasCompletedCampaign(cid - 1));

                if (locked_full || locked_extra) {
                    const int n = images.size() - 1;
                    images[n]->CopyBitmap(*c->GetImage(2));
                }
            }
        }
    }

    if (description)
        description->SetText(FText::FromString(UTF8_TO_TCHAR(select_msg.data())));

    if (btn_accept)
        btn_accept->SetIsEnabled(false);

    show_saved = false;
}

// +--------------------------------------------------------------------+

void UCampaignSelectDlg::ShowSavedCampaigns()
{
    AutoThreadSync a(sync);

    if (loading && description) {
        description->SetText(FText::FromString(UTF8_TO_TCHAR(Game::GetText("CmpSelectDlg.already-loading").data())));
        // Button::PlaySound(Button::SND_REJECT);
        return;
    }

    if (btn_delete)
        btn_delete->SetIsEnabled(false);

    if (lst_campaigns) {
        // UListView population is UObject-driven. Build save list here:
        List<Text> save_list;

        CampaignSaveGame::GetSaveGameList(save_list);
        save_list.sort();

        // TODO: create UObjects for each save entry and set as list items.
        // for (int i=0; i<save_list.size(); ++i) { ... }

        save_list.destroy();
    }

    if (description)
        description->SetText(FText::FromString(UTF8_TO_TCHAR(select_msg.data())));

    if (btn_accept)
        btn_accept->SetIsEnabled(false);

    show_saved = true;
}

// +--------------------------------------------------------------------+

void UCampaignSelectDlg::OnCampaignSelect()
{
    if (description && lst_campaigns) {
        AutoThreadSync a(sync);

        if (loading) {
            description->SetText(FText::FromString(UTF8_TO_TCHAR(Game::GetText("CmpSelectDlg.already-loading").data())));
            // Button::PlaySound(Button::SND_REJECT);
            return;
        }

        load_index = -1;
        load_file = "";

        PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();
        if (!player)
            return;

        // NOTE:
        // In classic, selection came from ListBox indices.
        // In UE, you must fetch the selected UObject item from UListView and map it:
        //
        // UObject* Sel = lst_campaigns->GetSelectedItem();
        // Determine if it's a "new campaign" entry (index) or "save file" entry (Text).
        //
        // For now, preserve behavior by requiring external code to set load_index/load_file
        // prior to calling StartLoadProc().

        if (btn_accept)
            btn_accept->SetIsEnabled(false);
    }

    if (!loading && (load_index >= 0 || load_file.length() > 0)) {
        if (btn_cancel)
            btn_cancel->SetIsEnabled(false);

        StartLoadProc();
    }
}

// +--------------------------------------------------------------------+

void UCampaignSelectDlg::RefreshUIFromSubsystem()
{

}

void UCampaignSelectDlg::OnNew()
{
    ShowNewCampaigns();
}

void UCampaignSelectDlg::OnSaved()
{
    ShowSavedCampaigns();
}

void UCampaignSelectDlg::OnDelete()
{
    load_file = "";

    // In UE, selection comes from UListView selected item.
    // This port keeps the deletion logic but requires load_file be set from selection mapping.

    if (load_file.length()) {
        // Confirm dialog flow:
        // ConfirmDlg* confirm = manager->GetConfirmDlg();
        // confirm->SetMessage(...); confirm->SetTitle(...); manager->ShowConfirmDlg();
        // Else: OnConfirmDelete();
        UE_LOG(LogTemp, Verbose, TEXT("CampaignSelectDlg: Request delete for save '%s'"), UTF8_TO_TCHAR(load_file.data()));
    }

    ShowSavedCampaigns();
}

void UCampaignSelectDlg::OnConfirmDelete()
{
    if (load_file.length()) {
        CampaignSaveGame::Delete(load_file);
    }

    ShowSavedCampaigns();
}

// +--------------------------------------------------------------------+

void UCampaignSelectDlg::OnAccept()
{
    AutoThreadSync a(sync);

    if (loading)
        return;

    // if this is to be a new campaign, re-instantiate the campaign object
    // NOTE: classic used btn_new->GetButtonState(). In UE, track a bool or enum state.
    // We preserve original intent by using show_saved as a proxy:
    if (!show_saved)
        Campaign::GetCampaign()->Load();
    else
        Game::ResetGameTime();

    Mouse::Show(false);
    if (stars)
        stars->SetGameMode(EGameMode::CLOD);
}

void UCampaignSelectDlg::OnCancelButtonClicked()
{
    //RefreshUIFromSubsystem();

    if (manager)
        manager->ShowMenuDlg();
    else
        HideDlg();
}

// +--------------------------------------------------------------------+
// Thread proc helpers (ported)
// +--------------------------------------------------------------------+

static uint32 CampaignSelectDlgLoadProc(void* link)
{
    UCampaignSelectDlg* dlg = static_cast<UCampaignSelectDlg*>(link);

    if (dlg)
        return dlg->LoadProc();

    return 0;
}

void UCampaignSelectDlg::StartLoadProc()
{
    // NOTE:
    // This is a minimal, legacy-style thread port. For production UE:
    // - Prefer Async(EAsyncExecution::ThreadPool, ...) or UE::Tasks.
    // - Avoid raw Win32 thread handles in cross-platform builds.

    if (hproc != nullptr) {
        // If you keep a platform thread handle wrapper, check and close it here.
        return;
    }

    campaign = 0;
    loading = true;
    loaded = false;

    if (description)
        description->SetText(FText::FromString(UTF8_TO_TCHAR(Game::GetText("CmpSelectDlg.loading").data())));

    // Placeholder: no raw CreateThread in this port layer.
    // Hook this to UE async when you are ready; for now, run synchronously:
    const uint32 Result = LoadProc();
    UE_LOG(LogTemp, Verbose, TEXT("CampaignSelectDlg: LoadProc result=%u"), Result);
}

void UCampaignSelectDlg::StopLoadProc()
{
    // Placeholder: if you implement an async task/thread, signal stop/join here.
    hproc = nullptr;
}

uint32 UCampaignSelectDlg::LoadProc()
{
    Campaign* c = 0;

    // NEW CAMPAIGN:
    if (load_index >= 0) {
        List<Campaign>& list = Campaign::GetAllCampaigns();

        if (load_index < list.size()) {
            c = list[load_index];
            c->Load();
        }
    }

    // SAVED CAMPAIGN:
    else {
        CampaignSaveGame savegame;
        savegame.Load(load_file);
        c = savegame.GetCampaign();
    }

    sync.acquire();

    loading = false;
    loaded = true;
    campaign = c;

    sync.release();

    return 0;
}

// --------------------------------------------------------------------
// UBaseScreen overrides
// --------------------------------------------------------------------

void UCampaignSelectDlg::BindFormWidgets()
{
    // Map FORM ids to widgets (optional – only if present in your UMG):
    BindButton(100, btn_new);
    BindButton(101, btn_saved);
    BindButton(102, btn_delete);
    BindButton(1, btn_accept);
    BindButton(2, btn_cancel);

    BindList(201, lst_campaigns);
    BindText(200, description);

    BindLabel(10, lbl_title);
    BindLabel(901, lbl_hdr_campaign);
    BindLabel(902, lbl_hdr_desc);

    // Backgrounds are typically Images:
    BindImage(9991, bg_9991);
    BindImage(9992, bg_9992);
}

FString UCampaignSelectDlg::GetLegacyFormText() const
{
    // Keep the original FORM as a raw string, or load it from an asset/datatable later.
    // Returning empty disables auto-application.
    return FString();
}

void UCampaignSelectDlg::ShowDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    RefreshUIFromSubsystem();
}

void UCampaignSelectDlg::HideDlg()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

