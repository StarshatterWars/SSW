
#include "MenuScreen.h"
#include "HUDView.h"
#include "RuntimeFeaturePolicy.h"
#include "OfflineCutsceneExport.h"
#include "PlayerCharacter.h"

// UE:
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// Dialogs:
#include "MenuDlg.h"
#include "ModEditorDlg.h"
#include "StationServices.h"
#include "Sim.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "FreeFlightHullRules.h"
#include "MissionHullAccessRules.h"
#include "TrainingShuttleRules.h"
#include "FreeFlightAssignment.h"
#include "StarshatterGameDataSubsystem.h"
#include "LocalGameStateSubsystem.h"
#include "DynamicMissionAI.h"
#include "ExitDlg.h"
#include "ConfirmDlg.h"
#include "FirstTimeDlg.h"
#include "PlayerDlg.h"
#include "AwardShowDlg.h"
#include "MissionSelectDlg.h"
#include "MissionsPanelDriver.h"
#include "MultiplayerDlg.h"
#include "CustomMissionDlg.h"
#include "CmdMissionsDlg.h"
#include "CampaignSelectDlg.h"
#include "MissionEditorDlg.h"
#include "MissionElementDlg.h"
#include "MissionBriefingDlg.h"
#include "AgentCatalogSubsystem.h"
#include "AgentMissionRules.h"
#include "Campaign.h"
#include "Mission.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PlayerCharacter.h"
#include "SSWGameInstance.h"
#include "SSWRuntimeSubsystem.h"
#include "MissionDebriefDlg.h"
#include "MissionEventDlg.h"
#include "MissionEditorNavDlg.h"
#include "OptionsScreen.h"
#include "LoadDlg.h"
#include "CmpLoadDlg.h"
#include "CmdDlg.h"
#include "CmpnScreen.h"
#include "TacRefDlg.h"
#include "Keyboard.h"

#include "StarshatterPlayerSubsystem.h"
#include "CampaignPresentationRules.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameStructs.h"
#include "StarshatterAssetRegistrySubsystem.h"

// ------------------------------------------------------------

void UMenuScreen::InitializeMenu(UGameInstance* InGI)
{
    if (!InGI) return;

    UStarshatterAssetRegistrySubsystem* Assets = InGI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();
    if (!Assets) return;

    if (!OptionsScreenClass)
        OptionsScreenClass = Assets->GetWidgetClass(TEXT("UI.OptionsScreenClass"), true);

    if (!CampaignSelectScreenClass)
        CampaignSelectScreenClass = Assets->GetWidgetClass(TEXT("UI.CampaignSelectScreenClass"), true);

    if (!MissionSelectScreenClass)
        MissionSelectScreenClass = Assets->GetWidgetClass(TEXT("UI.MissionSelectScreenClass"), true);

    if (!OperationsScreenClass)
        OperationsScreenClass = Assets->GetWidgetClass(TEXT("UI.OperationscreenClass"), true);

    if (!MissionScreenClass)
        MissionScreenClass = Assets->GetWidgetClass(TEXT("UI.MissionScreenClass"), true);

    if (!FirstTimeDlgClass)
        FirstTimeDlgClass = Assets->GetWidgetClass(TEXT("UI.FirstTimeDlgClass"), true);

    if (!ExitDlgClass)
        ExitDlgClass = Assets->GetWidgetClass(TEXT("UI.ExitDlgClass"), true);

    if (!PlayerDlgClass)
        PlayerDlgClass = Assets->GetWidgetClass(TEXT("UI.PlayerLogbookScreenClass"), true);

    if (!TacRefDlgClass)
        TacRefDlgClass = Assets->GetWidgetClass(TEXT("UI.TacRefScreenClass"), true);

    if (!CmpLoadDlgClass)
        CmpLoadDlgClass = Assets->GetWidgetClass(TEXT("UI.CampaignLoadClass"), true);

    if (!CmpnScreenClass)
        CmpnScreenClass = Assets->GetWidgetClass(TEXT("UI.CampaignScreenClass"), true);

    // Mission Editor is supplied by WBP_MissionEditorScreen, derived from UMissionEditorDlg.
    // Prefer the registry entry rather than an old Blueprint default.
    if (TSubclassOf<UMissionEditorDlg> RegisteredEditor =
            ResolveWidgetOrLog<UMissionEditorDlg>(Assets, TEXT("UI.MissionEditorClass")))
    {
        if (MsnEditDlg && MsnEditDlg->GetClass() != RegisteredEditor.Get())
        {
            MsnEditDlg->RemoveFromParent();
            MsnEditDlg = nullptr;
        }
        MsnEditDlgClass = RegisteredEditor;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] Initialize: MenuDlgClass=%s Options=%s FirstTime=%s Exit=%s CmpLoad=%s CmpnScreen=%s"),
        *GetNameSafe(MenuDlgClass.Get()),
        *GetNameSafe(OptionsScreenClass.Get()),
        *GetNameSafe(FirstTimeDlgClass.Get()),
        *GetNameSafe(ExitDlgClass.Get()),
        *GetNameSafe(CmpLoadDlgClass.Get()),
        *GetNameSafe(CmpnScreenClass.Get()));
}

static void ApplyUIFocus(APlayerController* PC, UUserWidget* FocusWidget)
{
    if (!PC || !FocusWidget)
        return;

    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;

    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(FocusWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);

    PC->SetInputMode(Mode);
}

UMenuScreen::UMenuScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMenuScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        InitializeMenu(GI);
    }
}

template<typename TDialog>
TDialog* UMenuScreen::EnsureDialog(TSubclassOf<TDialog> ClassToSpawn, TObjectPtr<TDialog>& Storage)
{
    if (Storage)
        return Storage.Get();

    if (!ClassToSpawn)
        return nullptr;

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureDialog: OwningPlayer is null (%s)"), *GetName());
        return nullptr;
    }

    TDialog* Created = CreateWidget<TDialog>(PC, ClassToSpawn);
    if (!Created)
        return nullptr;

    Created->SetMenuManager(this);
    Created->AddToViewport(10);
    Created->SetVisibility(ESlateVisibility::Hidden);
    Created->SetIsEnabled(false);

    Storage = Created;
    return Created;
}

void UMenuScreen::ShowDialog(UBaseScreen* Dialog, int32 ZOrder)
{
    if (!Dialog)
    {
        return;
    }

    if (Dialog->IsInViewport())
        Dialog->RemoveFromParent();

    Dialog->AddToViewport(ZOrder);
    Dialog->SetMenuManager(this);
    Dialog->SetVisibility(ESlateVisibility::Visible);
    Dialog->SetDialogInputEnabled(true);

    CurrentDialog = Dialog;

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(Dialog->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
    }
}

void UMenuScreen::HideDialog(UBaseScreen* Dialog)
{
    if (!Dialog)
        return;

    Dialog->SetDialogInputEnabled(false);
    Dialog->SetVisibility(ESlateVisibility::Collapsed);

    if (CurrentDialog == Dialog)
        CurrentDialog = nullptr;
}

void UMenuScreen::HideAll()
{
    HideDialog(MenuDlg);
    if(ModEditorDlg)ModEditorDlg->EndEditingSession();
    HideDialog(ModEditorDlg);
    HideDialog(ExitDlg);
    HideDialog(ConfirmDlg);

    HideDialog(FirstTimeDlg);
    HideDialog(PlayerDlg);
    HideDialog(AwardDlg);

    HideDialog(MissionsPanelDriver);
    HideDialog(MultiplayerMissionDlg);
    HideDialog(CustomMissionDlg);
    HideDialog(MissionSelectDlg);
    HideDialog(CustomMissionDlg);
    HideDialog(CmdMissionsDlg);
    HideDialog(CmdDlg);
    HideDialog(CmpnScreen);
    HideDialog(CmpSelectDlg);

    HideDialog(MissionBriefingDlg);
    HideDialog(MissionDebriefDlg);

    HideDialog(MsnEditDlg);
    HideDialog(MsnElemDlg);
    HideDialog(MsnEventDlg);
    HideDialog(MsnEditNavDlg);

    HideDialog(LoadDlg);
    HideDialog(CmpLoadDlg);
    HideDialog(TacRefDlg);

    HideDialog(OptionsScreen);

    CurrentDialog = nullptr;
}

void UMenuScreen::Setup()
{
    EnsureDialog<UMenuDlg>(MenuDlgClass, MenuDlg);
    EnsureDialog<UExitDlg>(ExitDlgClass, ExitDlg);

    EnsureDialog<UOptionsScreen>(OptionsScreenClass, OptionsScreen);
    EnsureDialog<UConfirmDlg>(ConfirmDlgClass, ConfirmDlg);

    EnsureDialog<UFirstTimeDlg>(FirstTimeDlgClass, FirstTimeDlg);
    EnsureDialog<UPlayerDlg>(PlayerDlgClass, PlayerDlg);
    EnsureDialog<UAwardShowDlg>(AwardDlgClass, AwardDlg);

    EnsureDialog<UMissionBriefingDlg>(MissionScreenClass, MissionBriefingDlg);

    EnsureDialog<UMissionEditorDlg>(MsnEditDlgClass, MsnEditDlg);
    EnsureDialog<UMissionElementDlg>(MsnElemDlgClass, MsnElemDlg);
    EnsureDialog<UMissionEventDlg>(MsnEventDlgClass, MsnEventDlg);
    EnsureDialog<UMissionEditorNavDlg>(MsnEditNavDlgClass, MsnEditNavDlg);

    EnsureDialog<ULoadDlg>(LoadDlgClass, LoadDlg);
    EnsureDialog<UCmpLoadDlg>(CmpLoadDlgClass, CmpLoadDlg);


    ShowMenuDlg();
}

void UMenuScreen::TearDown()
{
    HideAll();

    auto Destroy = [](UBaseScreen*& W)
        {
            if (W)
            {
                if (W->IsInViewport())
                    W->RemoveFromParent();
                W = nullptr;
            }
        };

    Destroy(reinterpret_cast<UBaseScreen*&>(MenuDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(ModEditorDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(ExitDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(ConfirmDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(FirstTimeDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(PlayerDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(AwardDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(MissionBriefingDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MissionDebriefDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MissionSelectDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CustomMissionDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmpSelectDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmdMissionsDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmdDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmpnScreen));

    Destroy(reinterpret_cast<UBaseScreen*&>(MsnEditDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MsnElemDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MsnEventDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MsnEditNavDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(LoadDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmpLoadDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(TacRefDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(OptionsScreen));

    CurrentDialog = nullptr;
    bIsShown = false;
}

void UMenuScreen::ExecFrame(double DeltaTime)
{
    if (!bIsShown) return;
    if (TimeTilChange > 0.0)
    {
        TimeTilChange -= DeltaTime;
        if (TimeTilChange < 0.0)
        {
            TimeTilChange = 0.0;
        }
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    }

    const bool bExitDown = PC && PC->WasInputKeyJustPressed(EKeys::Escape);

    if (bExitDown)
    {
        if (TimeTilChange <= 0.0)
        {
            TimeTilChange = 0.5;

            if (!bExitLatch && !CloseTopmost())
            {
                ShowExitDlg();
            }
        }

        bExitLatch = true;
    }
    else
    {
        bExitLatch = false;
    }

    if (bShowMissionsRequested)
    {
        ShowMissionSelectDlg();
        bShowMissionsRequested = false;
    }

    if (bRequestVideoChange)
    {
        bRequestVideoChange = false;

        //ChangeVideo();
        Setup();
    }
}

bool UMenuScreen::CloseTopmost()
{
    if (MsnElemDlg && MsnElemDlg->GetVisibility() == ESlateVisibility::Visible) { HideMsnElemDlg(); return true; }
    if (MsnEventDlg && MsnEventDlg->GetVisibility() == ESlateVisibility::Visible) { HideMsnEventDlg(); return true; }

    if (OptionsScreen && OptionsScreen->GetVisibility() == ESlateVisibility::Visible)
    {
        ReturnFromOptions();
        return true;
    }

    if (CmpSelectDlg && CmpSelectDlg->GetVisibility() == ESlateVisibility::Visible)
    {
        ShowMenuDlg();
        return true;
    }

    if (MenuDlg && MenuDlg->GetVisibility() != ESlateVisibility::Visible)
    {
        ShowMenuDlg();
        return true;
    }

    return false;
}

void UMenuScreen::Show()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] UMenuScreen::Show()"));
    if (bIsShown)
        return;

    if(OfflineCutsceneExport::TryStart(this)){bIsShown=true;return;}

    bool bHasSaveNow = false;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>())
        {
            UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] UStarshatterPlayerSubsystem"));

            if (!PlayerSS->HasLoaded())
            {
                PlayerSS->LoadFromBoot();
            }

            bHasSaveNow = PlayerSS->DoesSaveExistNow();

            UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] SaveExistsNow=%d"), bHasSaveNow ? 1 : 0);
        }
    }

    if (bHasSaveNow)
    {
        ShowMenuDlg();
    }
    else
    {
        ShowFirstTimeDlg();
    }

    bIsShown = true;
}

void UMenuScreen::Hide()
{
    if (!bIsShown)
        return;

    HideAll();
    bIsShown = false;
}

bool UMenuScreen::HasLivePlayerSession()
{
    auto* Simulation=Sim::GetSim();const auto* Pilot=Simulation?Simulation->GetPlayerShip():nullptr;
    if(!Simulation || !Simulation->GetMission() || !Pilot)return false;
    // Menu/cutscene camera hosts are presentation objects, not a flying pilot.
    return !Pilot->Design() || FCStringAnsi::Stricmp(Pilot->Design()->name,"CameraPod")!=0;
}
bool UMenuScreen::CanOpenMainMenu()
{
    const auto* Simulation=Sim::GetSim();
    return MenuAccessForState(HasLivePlayerSession(),FStationServices::IsDocked(),Simulation && Simulation->IsNetworked());
}
void UMenuScreen::ShowModEditorDlg()
{
    if(!FSSWFeaturePolicy::ModEditor())return;
    if(!CanOpenMainMenu())return;
    APlayerController* PC=GetOwningPlayer();if(!PC)return;
    bModMissionToolsActive=false;bReturnToCustomMissionsAfterEditor=false;
    HideAll();
    if(!ModEditorDlg)
    {
        UClass* Class=UModEditorDlg::StaticClass();
        if(auto* GI=GetGameInstance())if(auto* Assets=GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>())
            if(auto Registered=Assets->GetWidgetClass(TEXT("UI.ModEditorPopupClass"),false);Registered && Registered->IsChildOf(UModEditorDlg::StaticClass()))Class=Registered.Get();
        ModEditorDlg=CreateWidget<UModEditorDlg>(PC,Class);
    }
    if(!ModEditorDlg)return;
    if(ModEditorDlg->IsInViewport())ModEditorDlg->RemoveFromParent();
    if(!ModEditorDlg->BeginEditingSession())return;
    ModEditorDlg->SetMenuManager(this);ShowDialog(ModEditorDlg,300);
    ApplyUIFocus(PC,ModEditorDlg);
}
void UMenuScreen::OpenModMissionTools()
{
    // Legacy map/editor tools manipulate the menu preview simulation. Never
    // give them a live docked mission, even though source editing is safe there.
    if(!CanOpenMainMenu() || HasLivePlayerSession())return;
    bModMissionToolsActive=true;ShowCustomMissionDlg();
}
void UMenuScreen::ShowMenuDlg()
{
    if(!CanOpenMainMenu())
    {UE_LOG(LogTemp,Display,TEXT("[MenuAccess] Main menu requires docking; multiplayer pilots must resign first."));return;}
    if(bModMissionToolsActive){bModMissionToolsActive=false;ShowModEditorDlg();return;}
    bReturnToFreeFlight = false;
    if (auto* Game = Cast<USSWGameInstance>(GetGameInstance())) Game->EndTrainingTest();
    HideAll();

    if (!MenuDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UMenuScreen::ShowMenuDlg: MenuDlgClass is NULL (set it in WBP_MenuScreen defaults)"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("UMenuScreen::ShowMenuDlg: OwningPlayer is NULL"));
        return;
    }

    if (!MenuDlg)
    {
        MenuDlg = CreateWidget<UMenuDlg>(PC, MenuDlgClass);
        if (!MenuDlg)
        {
            UE_LOG(LogTemp, Error, TEXT("UMenuScreen::ShowMenuDlg: Failed to create MenuDlg"));
            return;
        }

        MenuDlg->Manager = this;
        MenuDlg->AddToViewport((int32)EMenuZOrder::Z_MENU_BASE);
        MenuDlg->SetDialogInputEnabled(false);
        MenuDlg->SetVisibility(ESlateVisibility::Hidden);
    }

    MenuDlg->Manager = this;

    ++ZCounter;
    // Restore input mode and focus as well as visibility when dismissing a modal.
    ShowDialog(MenuDlg, (int32)EMenuZOrder::Z_MENU_BASE);
    MenuDlg->Show();

    CurrentDialog = MenuDlg;
}

void UMenuScreen::ShowCampaignSelectDlg()
{
    // Campaigns and single missions are now offered by agents in the live session.
    ReturnFromMissionBriefing();
}

void UMenuScreen::ShowOperationsMissionsDlg()
{
    // Redirect stale menu/Blueprint callbacks into the live career session.
    ReturnFromMissionBriefing();
}

void UMenuScreen::ShowOperationsDlg()
{
    // Retired operation pages have no navigation entry.
    ReturnFromMissionBriefing();
}

void UMenuScreen::ShowCmpnScreen()
{
    // Redirect stale menu/Blueprint callbacks into the live career session.
    ReturnFromMissionBriefing();
}

void UMenuScreen::HideCmpnScreen()
{
    HideDialog(CmpnScreen);
}

void UMenuScreen::StartFreeFlight()
{
    TGuardValue<bool> DirectLaunch(bSkipMissionPreparation, true);
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    Campaign* Repository = Game ? Game->GetAgentTrainingCampaign() : nullptr;
    if (!Repository) return;

    // Normal career entry: agent assignments and outcomes use the persistent pilot journal.
    Game->EndTrainingTest();
    if(auto* State=Game->GetSubsystem<ULocalGameStateSubsystem>())
    {
        const int32 Resume=State->ResumeMissionId();
        const auto* Pilot=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
        const bool Eligible = Pilot && TrainingShuttleRules::CanResumeSavedMission(Pilot->GetPlayerInfo(), Resume);
        if(Eligible && Resume!=INDEX_NONE && Resume!=Mission::FreeFlightId && OpenQueuedAssignment(Resume,true))return;
    }

    const auto* PlayersForEntry = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* EntryProfile = PlayersForEntry ? &PlayersForEntry->GetPlayerInfo() : nullptr;
    const auto* ActiveHull = EntryProfile ? EntryProfile->HangarShips.FindByPredicate([&](const FPlayerHangarShip& Hull)
        { return Hull.HullId == EntryProfile->AssignedHangarHull && !Hull.bLost; }) : nullptr;
    const FString Dock = ActiveHull ? ActiveHull->Location : FString();
    const bool SurfaceEntry = EntryProfile && (TrainingShuttleRules::AtJanek(*EntryProfile) ||
        Dock.Equals(TEXT("Station / Janus / Janek Station"), ESearchCase::IgnoreCase));
    const bool StationEntry = !SurfaceEntry && Dock.StartsWith(TEXT("Station / "));
    // Rebuild the small entry definition so a change of dock cannot reuse the previous base.
    Game->FreeFlightMission.Reset();
    if (!Game->FreeFlightMission)
    {
        auto Model = MakeShared<Mission>(Mission::FreeFlightId);
        FS_CampaignMission Row;
        auto* Data = Game->GetSubsystem<UStarshatterGameDataSubsystem>();
        const TCHAR* Scene = SurfaceEntry ? TEXT("FreeFlightSurface.json") :
            StationEntry ? TEXT("FreeFlightStation.json") : TEXT("FreeFlight.json");
        bool Loaded = Data && Data->ReadLooseMission(
            FPaths::ProjectContentDir() / TEXT("GameData/Campaigns/01/Scenes") / Scene, Row);
        if (Loaded && StationEntry)
        {
            // Resolve the saved dock against the authored local station scenery. Never use
            // the selected remote hangar row as a destination or teleport its hull here.
            TArray<FString> Parts;
            Dock.ParseIntoArray(Parts, TEXT(" / "), true);
            FS_CampaignMission CarrierScene;
            Loaded = Parts.Num() == 3 && Data->ReadLooseMission(
                FPaths::ProjectContentDir() / TEXT("GameData/Campaigns/01/Scenes/FreeFlight.json"), CarrierScene);
            const FS_MissionElement* Base = Loaded ? CarrierScene.Element.FindByPredicate([&](const FS_MissionElement& Element)
                { return Element.Name.Equals(Parts[2], ESearchCase::IgnoreCase) && Element.Carrier.IsEmpty(); }) : nullptr;
            Loaded = Base && Parts[1].Equals(CarrierScene.MissionSystem, ESearchCase::IgnoreCase);
            if (Loaded)
            {
                Row.MissionSystem = CarrierScene.MissionSystem;
                Row.MissionRegion = Base->RegionName.IsEmpty() ? CarrierScene.MissionRegion : Base->RegionName;
                Row.Element[0] = *Base;
                for (int32 Index = 1; Index < Row.Element.Num(); ++Index)
                    Row.Element[Index].Carrier = Row.Element[Index].Squadron.IsEmpty() ? Base->Name : FString();
            }
        }
        Loaded = Loaded && Model->LoadFromCampaignMissionData(Row) && Model->GetPlayer();
        if (!Loaded)
        { UE_LOG(LogTemp, Error, TEXT("Career entry could not load %s"), SurfaceEntry ? TEXT("Janek Station") : TEXT("Orion")); return; }
        Game->FreeFlightMission = MoveTemp(Model);
    }

    if(auto* Players=Game->GetSubsystem<UStarshatterPlayerSubsystem>())
        FreeFlightHullRules::Apply(*Game->FreeFlightMission,Players->GetPlayerInfo());
    if (auto* Runtime = Game->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetGameMode(EGameMode::PREP);
    ShowStandaloneMissionDlg(Game->FreeFlightMission.Get(), Repository);
    if (CurrentDialog == MissionBriefingDlg && MissionBriefingDlg)
        MissionBriefingDlg->OnCommit();
}

void UMenuScreen::StartTestMission(bool bResume)
{
    TGuardValue<bool> DirectLaunch(bSkipMissionPreparation, true);
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    Campaign* Repository = Game ? Game->GetAgentTrainingCampaign() : nullptr;
    if (!Repository) return;
    if (!Game->TestFlightMission)
    {
        FS_CampaignMission Row;
        auto* Data=Game->GetSubsystem<UStarshatterGameDataSubsystem>();
        const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Tests/JanekTest.json");
        auto Model = MakeShared<Mission>(Mission::TestFlightId);
        if (!Data || !Data->ReadLooseMission(Path,Row) || !Model->LoadFromCampaignMissionData(Row) || !Model->GetPlayer())
        { UE_LOG(LogTemp, Error, TEXT("[TestMission] Invalid Janek test environment.")); return; }
        Game->TestFlightMission = MoveTemp(Model);
    }
    Game->BeginTrainingTest(bResume);
    if (auto* Runtime = Game->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetGameMode(EGameMode::PREP);
    ShowStandaloneMissionDlg(Game->TestFlightMission.Get(), Repository);
    if (CurrentDialog == MissionBriefingDlg && MissionBriefingDlg) MissionBriefingDlg->OnCommit();
}

bool UMenuScreen::OpenQueuedAssignment(int32 MissionId, bool bFromFreeFlight, FText* OutReason)
{
    TGuardValue<bool> DirectLaunch(bSkipMissionPreparation, true);
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    auto* Catalog = Game ? Game->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    if (OutReason) *OutReason = FText::GetEmpty();
    if (!Catalog || !Pilot)
    {
        if (OutReason) *OutReason = INVTEXT("Pilot or mission catalog is unavailable. Reopen Agents and retry.");
        return false;
    }

    const auto* Queued = Game->GetTrainingAgent().Find(AgentAI::PilotKey(*Pilot));
    if (!Queued || Queued->MissionId != MissionId || Queued->State != AgentAI::EState::Accepted ||
        !Catalog->CanAccept(MissionId, Queued->Issuer))
    {
        if (OutReason) *OutReason = INVTEXT("This mission is not the next accepted assignment, or its requirements are no longer met. Check your agent queue.");
        return false;
    }

    if(MissionId==FreeFlightAssignment::Id || DynamicMissionAI::IsDynamic(MissionId))
    {
        FText Reason;
        const bool Started = Catalog->StartIndependentAssignment(MissionId, Reason);
        if (OutReason) *OutReason = Reason;
        return Started;
    }
    Mission* Model = Catalog->GetMission(MissionId);
    Campaign* Repository = Catalog->GetRepository(MissionId);
    if (!Model || !Repository)
    {
        if (OutReason) *OutReason = INVTEXT("The mission definition could not be loaded. Verify the installed game files.");
        return false;
    }
    const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Flight = Model->GetPlayer();
    const auto* Required = Flight ? Flight->GetShipDesign() : nullptr;
    const FName RequiredHull = Required ? FName(*Required->ShipName) : NAME_None;
    if (!Players || !MissionHullAccessRules::FindAvailable(Players->GetPlayerInfo(), RequiredHull))
    {
        const FText Reason = FText::Format(INVTEXT("This mission requires an available {0} in your hangar. Check the Ship Store."), FText::FromName(RequiredHull));
        if (OutReason) *OutReason = Reason;
        HUDView::Message("%s", TCHAR_TO_UTF8(*Reason.ToString()));
        return false;
    }
    // Authored campaign/story craft and callsigns remain untouched. Ownership is
    // an entry requirement; Free Verse selection is a separate provisioning path.
    // Surface lessons use an implied shuttle transfer. Their authored start,
    // aircraft and callsigns are used directly, without a carrier jump leg.
    bReturnToFreeFlight = bFromFreeFlight;
    if (auto* Runtime = Game->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetGameMode(EGameMode::PREP);
    int32 CampaignId, OriginalId;
    if (AgentMissionRules::Decode(MissionId, CampaignId, OriginalId))
    {
        Repository->SetMissionId(OriginalId);
        ShowMissionDlg();
    }
    else ShowStandaloneMissionDlg(Model, Repository);
    if (CurrentDialog == MissionBriefingDlg && MissionBriefingDlg) MissionBriefingDlg->OnCommit();
    return true;
}

void UMenuScreen::ShowStandaloneMissionDlg(Mission* InMission, Campaign* InRepository)
{
    if (!InMission || !InRepository)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] Standalone briefing requires a mission and repository"));
        return;
    }
    PendingStandaloneMission = InMission;
    PendingStandaloneRepository = InRepository;
    ShowMissionDlg();
}

void UMenuScreen::ShowMissionDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionDlg: BEGIN"));

    if (!MissionScreenClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowMissionDlg: MissionScreenClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowMissionDlg: OwningPlayer is NULL"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionDlg: before EnsureDialog"));
    EnsureDialog<UMissionBriefingDlg>(MissionScreenClass, MissionBriefingDlg);

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionDlg: after EnsureDialog MissionBriefingDlg=%s"),
        *GetNameSafe(MissionBriefingDlg));

    if (!MissionBriefingDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowMissionDlg: EnsureDialog failed (MissionBriefingDlg is NULL)"));
        return;
    }

    // Capture the source before HideAll clears CurrentDialog. Re-showing the
    // briefing itself must not replace the original caller with the briefing.
    if (CurrentDialog != MissionBriefingDlg)
    {
        MissionBriefingCaller = CurrentDialog.Get();
        bBriefingCallerWasStandalone = (PendingStandaloneMission != nullptr);
    }
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionDlg: before HideAll"));
    HideAll();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionDlg: before SetMenuManager/InitializeDlg"));
    MissionBriefingDlg->SetMenuManager(this);
    MissionBriefingDlg->InitializeDlg(this);

    CurrentDialog = MissionBriefingDlg;
    // This widget remains the live-scene controller, never a visible preparation page.
    MissionBriefingDlg->SetDialogInputEnabled(false);
    MissionBriefingDlg->SetVisibility(ESlateVisibility::Collapsed);

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionDlg: before ShowMsnDlg"));
    if (PendingStandaloneMission)
        MissionBriefingDlg->SetStandaloneMission(PendingStandaloneMission, PendingStandaloneRepository);
    else
        MissionBriefingDlg->ClearStandaloneMission();
    PendingStandaloneMission = nullptr;
    PendingStandaloneRepository = nullptr;
    MissionBriefingDlg->ShowMsnDlg();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionDlg: END InViewport=%d Vis=%d"),
        MissionBriefingDlg->IsInViewport() ? 1 : 0,
        (int32)MissionBriefingDlg->GetVisibility());
}

UMissionDebriefDlg* UMenuScreen::GetMissionDebriefDlg()
{
    TSubclassOf<UMissionDebriefDlg> DebriefClass = UMissionDebriefDlg::StaticClass();
    if (UGameInstance* GI = GetGameInstance())
        if (auto* Assets = GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>())
            if (auto Registered = ResolveWidgetOrLog<UMissionDebriefDlg>(Assets,TEXT("UI.MissionDebriefClass")))
                DebriefClass = Registered;
    return EnsureDialog<UMissionDebriefDlg>(DebriefClass,MissionDebriefDlg);
}

void UMenuScreen::ShowMissionDebriefDlg(Campaign* TrainingCampaign)
{
    if (auto* Debrief = GetMissionDebriefDlg())
    {
        HideAll();
        Debrief->SetTrainingCampaign(TrainingCampaign);
        Debrief->Show();
        ShowDialog(Debrief,300);
    }
}

void UMenuScreen::DismissLivePopup(UBaseScreen* Popup)
{
    HideDialog(Popup);
    if (auto* PC=GetOwningPlayer())
    {
        PC->SetInputMode(FInputModeGameAndUI());
        PC->bShowMouseCursor=true;
    }
}

void UMenuScreen::ReturnFromMissionBriefing()
{
    // The live career shell replaces both retired preparation/operations screens.
    bReturnToFreeFlight = false;
    MissionBriefingCaller.Reset();
    bBriefingCallerWasStandalone = false;
    if (auto* Game = Cast<USSWGameInstance>(GetGameInstance()); Game && Game->IsTrainingTestSession())
        StartTestMission(true);
    else
        StartFreeFlight();
}

void UMenuScreen::ShowMissionSelectDlg()
{
    if(bModMissionToolsActive){ShowCustomMissionDlg();return;}
    // Redirect stale menu/Blueprint callbacks into the live career session.
    ReturnFromMissionBriefing();
}

void UMenuScreen::ShowMultiplayerMissionDlg()
{
    if(!FSSWFeaturePolicy::Multiplayer())return;
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    if (!MultiplayerMissionDlg)
        MultiplayerMissionDlg = CreateWidget<UMultiplayerDlg>(PC, UMultiplayerDlg::StaticClass());
    if (!MultiplayerMissionDlg) return;
    HideAll();
    MultiplayerMissionDlg->SetMenuManager(this);
    if (MultiplayerMissionDlg->IsInViewport()) MultiplayerMissionDlg->RemoveFromParent();
    MultiplayerMissionDlg->AddToViewport(300);
    MultiplayerMissionDlg->SetVisibility(ESlateVisibility::Visible);
    MultiplayerMissionDlg->SetIsEnabled(true);
    MultiplayerMissionDlg->SetIsFocusable(true);
    MultiplayerMissionDlg->SetDialogInputEnabled(true);
    CurrentDialog = MultiplayerMissionDlg;
    ApplyUIFocus(PC, MultiplayerMissionDlg);
    MultiplayerMissionDlg->Show();
}

void UMenuScreen::ShowSingleMissionDlg()
{
    // Campaigns and single missions are now offered by agents in the live session.
    ReturnFromMissionBriefing();
}

void UMenuScreen::ShowCustomMissionDlg()
{
    if(!bModMissionToolsActive){ShowModEditorDlg();return;}
    if(!CanOpenMainMenu() || HasLivePlayerSession())return;
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    // Native class requires no new Blueprint asset. A future WBP subclass can
    // replace it without altering the routing code.
    if (!CustomMissionDlg)
    {
        CustomMissionDlg = CreateWidget<UCustomMissionDlg>(PC, UCustomMissionDlg::StaticClass());
        if (!CustomMissionDlg) return;
    }

    HideAll();
    CustomMissionDlg->SetMenuManager(this);
    CustomMissionDlg->InitializeDlg(this);
    if (CustomMissionDlg->IsInViewport()) CustomMissionDlg->RemoveFromParent();
    CustomMissionDlg->AddToViewport(300);
    CustomMissionDlg->SetIsEnabled(true);
    CustomMissionDlg->SetIsFocusable(true);
    CustomMissionDlg->SetVisibility(ESlateVisibility::Visible);
    CustomMissionDlg->SetDialogInputEnabled(true);
    CurrentDialog = CustomMissionDlg;
    ApplyUIFocus(PC, CustomMissionDlg);
    CustomMissionDlg->Show();
}

void UMenuScreen::ShowMissionEditorDlg()
{
    // Single Missions is displayed by a separate menu host. Give the editor a
    // higher viewport layer so its transparent parent cannot consume clicks.
    constexpr int32 MissionEditorZOrder = 2000;

    if (!MsnEditDlgClass)
    {
        if (UGameInstance* GI = GetGameInstance())
        {
            InitializeMenu(GI);
        }
    }

    EnsureDialog<UMissionEditorDlg>(MsnEditDlgClass, MsnEditDlg);
    if (!MsnEditDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionEditor] Cannot open: UI.MissionEditorClass is unresolved or widget creation failed."));
        return;
    }

    HideAll();
    MsnEditDlg->SetMenuManager(this);
    ShowDialog(MsnEditDlg, MissionEditorZOrder);
    MsnEditDlg->SetIsEnabled(true);
    MsnEditDlg->SetIsFocusable(true);
    MsnEditDlg->SetDialogInputEnabled(true);
    MsnEditDlg->Show();

    // Focus after Show() has populated its editable UMG controls.
    ApplyUIFocus(GetOwningPlayer(), MsnEditDlg);
    UE_LOG(LogTemp, Display, TEXT("[MissionEditor] Shown: %s Z=%d enabled=%d visibility=%d"),
        *GetNameSafe(MsnEditDlg), MissionEditorZOrder,
        MsnEditDlg->GetIsEnabled() ? 1 : 0, (int32)MsnEditDlg->GetVisibility());
}

void UMenuScreen::ShowMsnElemDlg()
{
    EnsureDialog<UMissionElementDlg>(MsnElemDlgClass, MsnElemDlg);
    if (!MsnElemDlg) return;

    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, 2000);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, 2100);

    ShowDialog(MsnElemDlg, 2200);
}

void UMenuScreen::HideMsnElemDlg()
{
    HideDialog(MsnElemDlg);

    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, 2000);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, 2100);
}

void UMenuScreen::ShowMissionEventDlg()
{
    EnsureDialog<UMissionEventDlg>(MsnEventDlgClass, MsnEventDlg);
    if (!MsnEventDlg) return;

    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, 2000);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, 2100);

    ShowDialog(MsnEventDlg, 2200);
}

void UMenuScreen::HideMsnEventDlg()
{
    HideDialog(MsnEventDlg);

    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, 2000);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, 2100);
}

void UMenuScreen::ShowNavDlg()
{
    EnsureDialog<UMissionEditorNavDlg>(MsnEditNavDlgClass, MsnEditNavDlg);
    if (!MsnEditNavDlg) return;

    if (MsnEditNavDlg->GetVisibility() != ESlateVisibility::Visible)
    {
        HideAll();
        ShowDialog(MsnEditNavDlg, 2100);
    }
}

void UMenuScreen::HideNavDlg()
{
    HideDialog(MsnEditNavDlg);
}

bool UMenuScreen::IsNavShown() const
{
    return (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible);
}

void UMenuScreen::ShowFirstTimeDlg()
{
    if (!MenuDlg)
        ShowMenuDlg();

    if (!FirstTimeDlg && FirstTimeDlgClass)
    {
        FirstTimeDlg = CreateWidget<UFirstTimeDlg>(GetOwningPlayer(), FirstTimeDlgClass);
        if (!FirstTimeDlg)
            return;

        FirstTimeDlg->SetMenuManager(this);
        FirstTimeDlg->AddToViewport((int32)EMenuZOrder::Z_MODAL);
    }

    ShowDialog(MenuDlg, (int32)EMenuZOrder::Z_MENU_BASE);
    MenuDlg->SetDialogInputEnabled(false);

    ShowDialog(FirstTimeDlg, (int32)EMenuZOrder::Z_MODAL);
    FirstTimeDlg->SetDialogInputEnabled(true);

    CurrentDialog = FirstTimeDlg;
}

void UMenuScreen::ShowPlayerDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowPlayerDlg: BEGIN"));

    if (!PlayerDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowPlayerDlg: PlayerDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowPlayerDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UPlayerDlg>(PlayerDlgClass, PlayerDlg);
    if (!PlayerDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowPlayerDlg: EnsureDialog failed (PlayerDlg is NULL)"));
        return;
    }

    HideAll();

    PlayerDlg->SetMenuManager(this);
    PlayerDlg->InitializeDlg(this);

    if (PlayerDlg->IsInViewport())
        PlayerDlg->RemoveFromParent();

    PlayerDlg->AddToViewport(200);

    PlayerDlg->SetVisibility(ESlateVisibility::Visible);
    PlayerDlg->SetIsEnabled(true);
    PlayerDlg->SetIsFocusable(true);
    PlayerDlg->SetDialogInputEnabled(true);

    CurrentDialog = PlayerDlg;

    ApplyUIFocus(PC, PlayerDlg);

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowPlayerDlg: SHOWN InViewport=%d Vis=%d"),
        PlayerDlg->IsInViewport() ? 1 : 0,
        (int32)PlayerDlg->GetVisibility());
}

void UMenuScreen::ShowTacRefDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowTacRefDlg: BEGIN"));

    if (!TacRefDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowTacRefDlg: TacRefDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowTacRefDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UTacRefDlg>(TacRefDlgClass, TacRefDlg);
    if (!TacRefDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowTacRefDlg: EnsureDialog failed (TacRefDlg is NULL)"));
        return;
    }

    HideAll();

    TacRefDlg->SetMenuManager(this);
    TacRefDlg->InitializeDlg(this);

    if (TacRefDlg->IsInViewport())
        TacRefDlg->RemoveFromParent();

    TacRefDlg->AddToViewport(200);

    TacRefDlg->SetVisibility(ESlateVisibility::Visible);
    TacRefDlg->SetIsEnabled(true);
    TacRefDlg->SetIsFocusable(true);
    TacRefDlg->SetDialogInputEnabled(true);

    CurrentDialog = TacRefDlg;

    ApplyUIFocus(PC, TacRefDlg);

    TacRefDlg->Show();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowTacRefDlg: SHOWN InViewport=%d Vis=%d"),
        TacRefDlg->IsInViewport() ? 1 : 0,
        (int32)TacRefDlg->GetVisibility());
}

void UMenuScreen::ShowAwardDlg()
{
    HideAll();
    EnsureDialog<UAwardShowDlg>(AwardDlgClass ? AwardDlgClass : TSubclassOf<UAwardShowDlg>(UAwardShowDlg::StaticClass()), AwardDlg);
    if(AwardDlg)
    {
        if(auto* P=PlayerCharacter::GetCurrentPlayer();P && P->ShowAward())
        {if(P->IsPendingAwardRank())AwardDlg->SetRank(P->GetPendingAwardId());else AwardDlg->SetMedal(P->GetPendingAwardId());}
        AwardDlg->SetMenuManager(this);
    }
    ShowDialog(AwardDlg, true);
    if(AwardDlg)AwardDlg->ShowDialog();
}

void UMenuScreen::ShowExitDlg()
{
    EnsureDialog<UMenuDlg>(MenuDlgClass, MenuDlg);
    EnsureDialog<UExitDlg>(ExitDlgClass, ExitDlg);

    if (!MenuDlg || !ExitDlg) return;

    MenuDlg->SetMenuManager(this);
    ExitDlg->SetMenuManager(this);

    MenuDlg->SetDialogInputEnabled(false);
    ShowDialog(MenuDlg, 100);

    ExitDlg->SetDialogInputEnabled(true);
    ShowDialog(ExitDlg, 200);
}

void UMenuScreen::ShowConfirmDlg()
{
    EnsureDialog<UConfirmDlg>(ConfirmDlgClass, ConfirmDlg);
    if (!ConfirmDlg) return;

    ShowDialog(ConfirmDlg, true);
}

void UMenuScreen::HideConfirmDlg()
{
    HideDialog(ConfirmDlg);
}

void UMenuScreen::ShowLoadDlg()
{
    EnsureDialog<ULoadDlg>(LoadDlgClass, LoadDlg);
    if (!LoadDlg) return;

    ShowDialog(LoadDlg, 250);
}

void UMenuScreen::HideLoadDlg()
{
    HideDialog(LoadDlg);
}

void UMenuScreen::ShowCmpLoadDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowCmpLoadDlg: BEGIN"));

    if (!CmpLoadDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowCmpLoadDlg: CmpLoadDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowCmpLoadDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UCmpLoadDlg>(CmpLoadDlgClass, CmpLoadDlg);
    if (!CmpLoadDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowCmpLoadDlg: EnsureDialog failed (CmpLoadDlg is NULL)"));
        return;
    }

    HideAll();

    CmpLoadDlg->SetMenuManager(this);

    if (CmpLoadDlg->IsInViewport())
    {
        CmpLoadDlg->RemoveFromParent();
    }

    CmpLoadDlg->AddToViewport(250);
    CmpLoadDlg->SetVisibility(ESlateVisibility::Visible);
    CmpLoadDlg->SetIsEnabled(true);
    CmpLoadDlg->SetIsFocusable(true);
    CmpLoadDlg->SetDialogInputEnabled(true);

    CurrentDialog = CmpLoadDlg;

    ApplyUIFocus(PC, CmpLoadDlg);

    CmpLoadDlg->Show();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowCmpLoadDlg: SHOWN InViewport=%d Vis=%d"),
        CmpLoadDlg->IsInViewport() ? 1 : 0,
        (int32)CmpLoadDlg->GetVisibility());
}

void UMenuScreen::HideCmpLoadDlg()
{
    HideDialog(CmpLoadDlg);
}

void UMenuScreen::ShowOptionsScreen()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowOptionsScreen: BEGIN"));

    if (!OptionsScreenClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOptionsScreen: OptionsScreenClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOptionsScreen: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UOptionsScreen>(OptionsScreenClass, OptionsScreen);
    if (!OptionsScreen)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOptionsScreen: EnsureDialog failed (OptionsScreen is NULL)"));
        return;
    }

    HideAll();

    OptionsScreen->SetMenuManager(this);

    if (OptionsScreen->IsInViewport())
        OptionsScreen->RemoveFromParent();

    OptionsScreen->AddToViewport(200);
    OptionsScreen->SetVisibility(ESlateVisibility::Visible);
    OptionsScreen->SetIsEnabled(true);
    OptionsScreen->SetIsFocusable(true);
    OptionsScreen->SetDialogInputEnabled(true);

    CurrentDialog = OptionsScreen;

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowOptionsScreen: SHOWN InViewport=%d Vis=%d"),
        OptionsScreen->IsInViewport() ? 1 : 0,
        (int32)OptionsScreen->GetVisibility());
}

void UMenuScreen::HideOptionsScreen()
{
    HideDialog(OptionsScreen);
}

void UMenuScreen::ReturnFromOptions()
{
    if (MissionOptionsReturn)
    {
        HideOptionsScreen();
        CurrentDialog = nullptr;
        auto Return = MoveTemp(MissionOptionsReturn);
        Return();
        return;
    }
    ShowMenuDlg();
}

void UMenuScreen::ReturnFromPlayerDlg()
{
    ShowMenuDlg();
}

void UMenuScreen::HandleAccept()
{
    if (CurrentDialog && CurrentDialog != this)
    {
        CurrentDialog->HandleAccept();
        return;
    }

    Super::HandleAccept();
}

void UMenuScreen::HandleCancel()
{
    if (CloseTopmost())
        return;

    Super::HandleCancel();
}
void UMenuScreen::EditMission(Mission* Model, MissionInfo* Info)
{
    bReturnToCustomMissionsAfterEditor = (CurrentDialog.Get() == CustomMissionDlg.Get());
    EnsureDialog<UMissionEditorDlg>(MsnEditDlgClass, MsnEditDlg);
    if (!MsnEditDlg || !Model) return;
    MsnEditDlg->InitializeMissionEditor(this);
    MsnEditDlg->SetMissionInfo(Info);
    MsnEditDlg->SetMission(Model);
    ShowMissionEditorDlg();
}
void UMenuScreen::EditMissionElement(Mission* Model, MissionElement* Element)
{
    EnsureDialog<UMissionElementDlg>(MsnElemDlgClass, MsnElemDlg);
    if (!MsnElemDlg) return;
    MsnElemDlg->InitializeDlg(this);
    MsnElemDlg->SetMission(Model);
    MsnElemDlg->SetMissionElement(Element);
    ShowMsnElemDlg();
    MsnElemDlg->ShowDlg();
}
void UMenuScreen::EditMissionEvent(Mission* Model, MissionEvent* Event)
{
    EnsureDialog<UMissionEventDlg>(MsnEventDlgClass, MsnEventDlg);
    if (!MsnEventDlg) return;
    MsnEventDlg->InitializeDlg(this);
    MsnEventDlg->SetMission(Model);
    MsnEventDlg->SetMissionEvent(Event);
    ShowMissionEventDlg();
    MsnEventDlg->ShowDlg();
}
void UMenuScreen::EditMissionMap(Mission* Model, MissionInfo* Info)
{
    EnsureDialog<UMissionEditorNavDlg>(MsnEditNavDlgClass, MsnEditNavDlg);
    if (!MsnEditNavDlg) return;
    MsnEditNavDlg->SetManager(this);
    MsnEditNavDlg->SetMission(Model);
    MsnEditNavDlg->SetMissionInfo(Info);
    ShowNavDlg();
    MsnEditNavDlg->Show();
}
