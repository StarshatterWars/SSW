/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionNavDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionNavDlg
*/

#include "MissionNavDlg.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"
#include "MenuButton.h"
#include "MissionNavObjectListObject.h"
#include "MissionNavObjectListView.h"
#include "MissionNavObjectLVElement.h"

#include "GalaxyMapPanel.h"
#include "SectorMapPanel.h"
#include "SystemMapPanel.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystemRegistry.h"

#include "MissionUIStyle.h"
#include "FormattingUtils.h"
#include "CombatGroupRegistry.h"

#include "Campaign.h"
#include "MapView.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionInfo.h"
#include "Orbital.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "StarSystem.h"

#include "Algo/Reverse.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Containers/Queue.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"

UMissionNavDlg::UMissionNavDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> PanelTexObj(
        TEXT("/Game/UI/Panel.Panel"));

    if (PanelTexObj.Succeeded())
    {
        RightPanelBackgroundTexture = PanelTexObj.Object;
    }
}

void UMissionNavDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;
}

Mission* UMissionNavDlg::ResolveMission() const
{
    return ParentDlg ? ParentDlg->GetMissionPtr() : nullptr;
}

UStarshatterEnvironmentSubsystem* UMissionNavDlg::GetEnvironmentSubsystem() const
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<UStarshatterEnvironmentSubsystem>();
}

StarSystem* UMissionNavDlg::FindRuntimeSystemByName(const FString& InSystemName) const
{
    if (InSystemName.IsEmpty())
    {
        return nullptr;
    }

    UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem();
    if (!Env)
    {
        return nullptr;
    }

    for (StarSystem* Sys : Env->GetRuntimeStarSystems())
    {
        if (!Sys)
        {
            continue;
        }

        if (InSystemName.Equals(ANSI_TO_TCHAR(Sys->GetName()), ESearchCase::IgnoreCase))
        {
            return Sys;
        }
    }

    return nullptr;
}

TArray<FString> UMissionNavDlg::GetRuntimeLinkedSystemNames(StarSystem* InSystem) const
{
    TArray<FString> Result;

    if (!InSystem)
    {
        return Result;
    }

    UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem();
    if (!Env)
    {
        return Result;
    }

    for (StarSystem* Candidate : Env->GetRuntimeStarSystems())
    {
        if (!Candidate || Candidate == InSystem)
        {
            continue;
        }

        if (InSystem->HasLinkTo(Candidate))
        {
            Result.Add(ANSI_TO_TCHAR(Candidate->GetName()));
        }
    }

    Result.Sort();
    return Result;
}

void UMissionNavDlg::NativeConstruct()
{
    Super::NativeConstruct();

    ensureMsgf(RuntimeHost, TEXT("MissionNavDlg: RuntimeHost is not bound"));

    if (!ObjectListEntryWidgetClass)
    {
        UClass* RowWidgetClass = LoadClass<UUserWidget>(
            nullptr,
            TEXT("/Game/Screens/Mission/WBP_MissionNavObjectRow.WBP_MissionNavObjectRow_C"));

        if (RowWidgetClass)
        {
            ObjectListEntryWidgetClass = RowWidgetClass;
        }
        else
        {
            ObjectListEntryWidgetClass = UMissionNavObjectLVElement::StaticClass();
        }
    }

    BuildRuntimeLayout();
    BuildNavModeButtons();

    if (ZoomInButton)
    {
        ZoomInButton->OnClicked.RemoveAll(this);
        ZoomInButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnZoomInClicked);
    }

    if (ZoomOutButton)
    {
        ZoomOutButton->OnClicked.RemoveAll(this);
        ZoomOutButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnZoomOutClicked);
    }

    RefreshNavModeSelection();
    RefreshFilterSelection();
    SetNavMode(CurrentNavMode);
}

FReply UMissionNavDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UMissionNavDlg::NativeOnMouseWheel(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    const float WheelDelta = InMouseEvent.GetWheelDelta();

    UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] NativeOnMouseWheel Delta=%.3f Mode=%d"),
        WheelDelta,
        static_cast<int32>(CurrentNavMode));

    if (WheelDelta > 0.0f)
    {
        if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
        {
            GalaxyMapPanel->ZoomIn();
            return FReply::Handled();
        }
        else if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
        {
            SystemMapPanel->ZoomIn();
            return FReply::Handled();
        }
        else if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
        {
            SectorMapPanel->ZoomIn();
            return FReply::Handled();
        }
    }
    else if (WheelDelta < 0.0f)
    {
        if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
        {
            GalaxyMapPanel->ZoomOut();
            return FReply::Handled();
        }
        else if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
        {
            SystemMapPanel->ZoomOut();
            return FReply::Handled();
        }
        else if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
        {
            SectorMapPanel->ZoomOut();
            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply UMissionNavDlg::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
    {
        return SectorMapPanel->NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
    {
        return SystemMapPanel->NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
    {
        return GalaxyMapPanel->NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMissionNavDlg::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
    {
        return SectorMapPanel->NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    }

    if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
    {
        return SystemMapPanel->NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    }

    if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
    {
        return GalaxyMapPanel->NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMissionNavDlg::NativeOnMouseMove(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
    {
        return SectorMapPanel->NativeOnMouseMove(InGeometry, InMouseEvent);
    }

    if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
    {
        return SystemMapPanel->NativeOnMouseMove(InGeometry, InMouseEvent);
    }

    if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
    {
        return GalaxyMapPanel->NativeOnMouseMove(InGeometry, InMouseEvent);
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UMissionNavDlg::RefreshFromMission()
{
    MissionPtr = ResolveMission();

    UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] RefreshFromMission: MissionPtr=%p"), MissionPtr);

    if (MissionPtr)
    {
        const FString MissionRegion = FString(ANSI_TO_TCHAR(MissionPtr->GetRegion())).TrimStartAndEnd();

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionNavDlg] RefreshFromMission: MissionRegion='%s'"),
            *MissionRegion);
    }

    if (MissionPtr && MissionPtr->GetStarSystem())
    {
        CurrentMissionSystemName = FString(MissionPtr->GetStarSystem()->GetName());

        if (SelectedSystemName.IsEmpty())
        {
            SelectedSystemName = CurrentMissionSystemName;
        }
    }
    else
    {
        CurrentMissionSystemName.Empty();
    }

    SyncGalaxyMissionAndSelectionState();
    SyncSubPanels();

    if (SectorMapPanel)
    {
        SectorMapPanel->SetMission(MissionPtr);
    }

    RefreshObjectListPanel();
    RefreshDetailPanel();
}

void UMissionNavDlg::SyncGalaxyMissionAndSelectionState()
{
    if (!GalaxyMapPanel)
    {
        return;
    }

    GalaxyMapPanel->SetCurrentMissionSystem(CurrentMissionSystemName);
    GalaxyMapPanel->SetSelectedSystem(SelectedSystemName);

    TArray<FString> Route;

    if (!CurrentMissionSystemName.IsEmpty() &&
        !SelectedSystemName.IsEmpty() &&
        !CurrentMissionSystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase))
    {
        Route = FindShortestGalaxyRoute(CurrentMissionSystemName, SelectedSystemName);
    }

    GalaxyMapPanel->SetRoutePath(Route);
}

void UMissionNavDlg::SyncSubPanels()
{
    FString SystemNameForPanels = SelectedSystemName;

    if (SystemNameForPanels.IsEmpty() && MissionPtr && MissionPtr->GetStarSystem())
    {
        SystemNameForPanels = FString(MissionPtr->GetStarSystem()->GetName());
    }

    if (SystemMapPanel)
    {
        SystemMapPanel->SetViewedSystemName(SystemNameForPanels);
    }

    if (SectorMapPanel)
    {
        SectorMapPanel->SetViewedSystemName(SystemNameForPanels);

        FString MissionRegionName;
        if (MissionPtr)
        {
            MissionRegionName = ANSI_TO_TCHAR(MissionPtr->GetRegion());
            MissionRegionName = MissionRegionName.TrimStartAndEnd();
        }

        // Lock sector view to the selected mission region only.
        SectorMapPanel->SetViewedSectorName(MissionRegionName);
        SectorMapPanel->SetMission(MissionPtr);
    }
}

TArray<FString> UMissionNavDlg::FindShortestGalaxyRoute(
    const FString& StartSystem,
    const FString& GoalSystem) const
{
    TArray<FString> EmptyRoute;

    if (StartSystem.IsEmpty() || GoalSystem.IsEmpty())
    {
        return EmptyRoute;
    }

    if (StartSystem.Equals(GoalSystem, ESearchCase::IgnoreCase))
    {
        TArray<FString> SelfRoute;
        SelfRoute.Add(StartSystem);
        return SelfRoute;
    }

    UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem();
    if (!Env)
    {
        return EmptyRoute;
    }

    TMap<FString, StarSystem*> NodeMap;
    for (StarSystem* Sys : Env->GetRuntimeStarSystems())
    {
        if (Sys)
        {
            NodeMap.Add(ANSI_TO_TCHAR(Sys->GetName()), Sys);
        }
    }

    if (!NodeMap.Contains(StartSystem) || !NodeMap.Contains(GoalSystem))
    {
        return EmptyRoute;
    }

    TQueue<FString> Frontier;
    TSet<FString> Visited;
    TMap<FString, FString> CameFrom;

    Frontier.Enqueue(StartSystem);
    Visited.Add(StartSystem);

    bool bFound = false;

    while (!Frontier.IsEmpty())
    {
        FString Current;
        Frontier.Dequeue(Current);

        if (Current.Equals(GoalSystem, ESearchCase::IgnoreCase))
        {
            bFound = true;
            break;
        }

        StarSystem** CurrentSysPtr = NodeMap.Find(Current);
        if (!CurrentSysPtr || !(*CurrentSysPtr))
        {
            continue;
        }

        StarSystem* CurrentSys = *CurrentSysPtr;
        const TArray<FString> NeighborNames = GetRuntimeLinkedSystemNames(CurrentSys);

        for (const FString& Neighbor : NeighborNames)
        {
            if (Neighbor.IsEmpty() || !NodeMap.Contains(Neighbor) || Visited.Contains(Neighbor))
            {
                continue;
            }

            Visited.Add(Neighbor);
            CameFrom.Add(Neighbor, Current);
            Frontier.Enqueue(Neighbor);

            if (Neighbor.Equals(GoalSystem, ESearchCase::IgnoreCase))
            {
                bFound = true;
                break;
            }
        }

        if (bFound)
        {
            break;
        }
    }

    if (!bFound)
    {
        return EmptyRoute;
    }

    TArray<FString> ReversePath;
    FString Step = GoalSystem;
    ReversePath.Add(Step);

    while (!Step.Equals(StartSystem, ESearchCase::IgnoreCase))
    {
        const FString* Prev = CameFrom.Find(Step);
        if (!Prev)
        {
            EmptyRoute.Empty();
            return EmptyRoute;
        }

        Step = *Prev;
        ReversePath.Add(Step);
    }

    Algo::Reverse(ReversePath);
    return ReversePath;
}

FString UMissionNavDlg::BuildGalaxySystemDetailText(const FString& InSystemName) const
{
    if (InSystemName.IsEmpty())
    {
        return TEXT("NO SYSTEM SELECTED");
    }

    StarSystem* FoundSystem = FindRuntimeSystemByName(InSystemName);
    if (!FoundSystem)
    {
        return FString::Printf(
            TEXT("%s\n\nTYPE: STAR SYSTEM\nNO DATA FOUND"),
            *InSystemName);
    }

    const TArray<FString> Route =
        FindShortestGalaxyRoute(CurrentMissionSystemName, InSystemName);

    const int32 JumpCount = Route.Num() > 0 ? Route.Num() - 1 : 0;
    const FString IffText = FString::Printf(TEXT("%d"), FoundSystem->GetAffiliation());

    const TArray<FString> LinkedNames = GetRuntimeLinkedSystemNames(FoundSystem);

    FString LinkText = TEXT("NONE");
    if (LinkedNames.Num() > 0)
    {
        LinkText = FString::Join(LinkedNames, TEXT(", "));
    }

    const FVector SystemLoc = FoundSystem->GetLocation();

    return FString::Printf(
        TEXT("%s\n\nTYPE: STAR SYSTEM\nIFF: %s\nLOCATION: X %.0f  Y %.0f  Z %.0f\nLINKS: %d\nJUMPS FROM MISSION: %d\nCONNECTED TO: %s\nRADIUS: %.0f"),
        ANSI_TO_TCHAR(FoundSystem->GetName()),
        *IffText,
        SystemLoc.X,
        SystemLoc.Y,
        SystemLoc.Z,
        LinkedNames.Num(),
        JumpCount,
        *LinkText,
        FoundSystem->GetRadius());
}

void UMissionNavDlg::UpdateSystemDetailsPanel(const FString& InSystemName)
{
    if (DetailTitleText)
    {
        DetailTitleText->SetText(FText::FromString(TEXT("DETAIL PANEL")));
        DetailTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        DetailTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    }

    if (DetailBodyText)
    {
        DetailBodyText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
        DetailBodyText->SetFont(MissionUIStyle::GetInfoValueFont());
        DetailBodyText->SetText(FText::FromString(BuildGalaxySystemDetailText(InSystemName)));
    }
}

void UMissionNavDlg::BuildRuntimeLayout()
{
    if (!WidgetTree || !RuntimeHost)
    {
        return;
    }

    if (RootContentRow)
    {
        return;
    }

    RuntimeHost->SetContent(nullptr);

    RootContentRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavRootContentRow"));

    RuntimeHost->SetContent(RootContentRow);

    LeftPanelBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavLeftPanelBorder"));

    LeftPanelBorder->SetPadding(FMargin(0.f));
    LeftPanelBorder->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.f));

    if (UHorizontalBoxSlot* LeftPanelSlot = RootContentRow->AddChildToHorizontalBox(LeftPanelBorder))
    {
        LeftPanelSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
        LeftPanelSlot->SetHorizontalAlignment(HAlign_Fill);
        LeftPanelSlot->SetVerticalAlignment(VAlign_Fill);
        LeftPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    LeftPanelColumn =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavLeftPanelColumn"));

    LeftPanelBorder->SetContent(LeftPanelColumn);

    TopButtonRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavTopButtonRow"));

    if (UVerticalBoxSlot* TopRowSlot = LeftPanelColumn->AddChildToVerticalBox(TopButtonRow))
    {
        TopRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
        TopRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    NavModeButtonBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavModeButtonBox"));

    TopButtonRow->AddChildToHorizontalBox(NavModeButtonBox);

    USpacer* Spacer =
        WidgetTree->ConstructWidget<USpacer>(
            USpacer::StaticClass(),
            TEXT("MissionNavSpacer"));

    if (UHorizontalBoxSlot* SpacerSlot = TopButtonRow->AddChildToHorizontalBox(Spacer))
    {
        SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ZoomButtonBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavZoomButtonBox"));

    TopButtonRow->AddChildToHorizontalBox(ZoomButtonBox);

    ZoomOutButton =
        WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(),
            TEXT("MissionNavZoomOutButton"));

    ZoomOutText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavZoomOutText"));
    ZoomOutText->SetText(FText::FromString(TEXT("-")));
    ZoomOutText->SetJustification(ETextJustify::Center);
    ZoomOutText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    ZoomOutText->SetFont(MissionUIStyle::GetHeaderFont(18));
    ZoomOutButton->AddChild(ZoomOutText);

    if (UHorizontalBoxSlot* ZoomOutSlot = ZoomButtonBox->AddChildToHorizontalBox(ZoomOutButton))
    {
        ZoomOutSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
        ZoomOutSlot->SetHorizontalAlignment(HAlign_Left);
        ZoomOutSlot->SetVerticalAlignment(VAlign_Center);
        ZoomOutSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ZoomInButton =
        WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(),
            TEXT("MissionNavZoomInButton"));

    ZoomInText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavZoomInText"));
    ZoomInText->SetText(FText::FromString(TEXT("+")));
    ZoomInText->SetJustification(ETextJustify::Center);
    ZoomInText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    ZoomInText->SetFont(MissionUIStyle::GetHeaderFont(18));
    ZoomInButton->AddChild(ZoomInText);

    if (UHorizontalBoxSlot* ZoomInSlot = ZoomButtonBox->AddChildToHorizontalBox(ZoomInButton))
    {
        ZoomInSlot->SetPadding(FMargin(0.f));
        ZoomInSlot->SetHorizontalAlignment(HAlign_Left);
        ZoomInSlot->SetVerticalAlignment(VAlign_Center);
        ZoomInSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    MainViewHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavMainViewHost"));

    if (UVerticalBoxSlot* VSlot = LeftPanelColumn->AddChildToVerticalBox(MainViewHost))
    {
        VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    NavSwitcher =
        WidgetTree->ConstructWidget<UWidgetSwitcher>(
            UWidgetSwitcher::StaticClass(),
            TEXT("MissionNavSwitcher"));

    MainViewHost->SetContent(NavSwitcher);

    MainViewHost->SetClipping(EWidgetClipping::ClipToBounds);
    NavSwitcher->SetClipping(EWidgetClipping::ClipToBounds);

    GalaxyPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavGalaxyPanelHost"));

    NavSwitcher->AddChild(GalaxyPanelHost);
    GalaxyPanelHost->SetClipping(EWidgetClipping::ClipToBounds);

    if (!GalaxyMapPanelClass)
    {
        GalaxyMapPanelClass = LoadClass<UGalaxyMapPanel>(
            nullptr,
            TEXT("/Game/Screens/Mission/WBP_GalaxyMapPanel.WBP_GalaxyMapPanel_C"));
    }

    if (!GalaxyMapPanelClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MissionNavDlg: Failed to load WBP_GalaxyMapPanel"));
    }
    else
    {
        GalaxyMapPanel = CreateWidget<UGalaxyMapPanel>(GetWorld(), GalaxyMapPanelClass);

        if (!GalaxyMapPanel)
        {
            UE_LOG(LogTemp, Error, TEXT("MissionNavDlg: Failed to create GalaxyMapPanel widget"));
        }
        else
        {
            GalaxyMapPanel->SetOwnerNavDlg(this);

            if (UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem())
            {
                GalaxyMapPanel->LoadFromRuntimeSystems(Env->GetRuntimeStarSystems());
            }

            GalaxyPanelHost->SetContent(GalaxyMapPanel);

            UE_LOG(LogTemp, Warning,
                TEXT("MissionNavDlg: Created GalaxyMapPanel from %s"),
                *GetNameSafe(GalaxyMapPanelClass));
        }
    }

    SystemPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavSystemPanelHost"));

    NavSwitcher->AddChild(SystemPanelHost);
    SystemPanelHost->SetClipping(EWidgetClipping::ClipToBounds);

    if (!SystemMapPanelClass)
    {
        SystemMapPanelClass = USystemMapPanel::StaticClass();
    }

    SystemMapPanel = CreateWidget<USystemMapPanel>(GetWorld(), SystemMapPanelClass);

    if (!SystemMapPanel)
    {
        UE_LOG(LogTemp, Error, TEXT("MissionNavDlg: Failed to create SystemMapPanel widget"));

        NavBodyText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("MissionNavSystemBodyText"));
        NavBodyText->SetText(FText::FromString(TEXT("SYSTEM MODE")));
        NavBodyText->SetJustification(ETextJustify::Left);
        NavBodyText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        NavBodyText->SetFont(MissionUIStyle::GetHeaderFont(18));
        SystemPanelHost->SetContent(NavBodyText);
    }
    else
    {
        SystemMapPanel->SetOwnerNavDlg(this);
        SystemPanelHost->SetContent(SystemMapPanel);

        UE_LOG(LogTemp, Warning,
            TEXT("MissionNavDlg: Created SystemMapPanel from %s"),
            *GetNameSafe(SystemMapPanelClass));
    }

    SectorPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavSectorPanelHost"));

    NavSwitcher->AddChild(SectorPanelHost);
    SectorPanelHost->SetClipping(EWidgetClipping::ClipToBounds);

    if (!SectorMapPanelClass)
    {
        SectorMapPanelClass = USectorMapPanel::StaticClass();
    }

    SectorMapPanel = CreateWidget<USectorMapPanel>(GetWorld(), SectorMapPanelClass);

    if (!SectorMapPanel)
    {
        UE_LOG(LogTemp, Error, TEXT("MissionNavDlg: Failed to create SectorMapPanel widget"));

        UTextBlock* SectorBodyText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("MissionNavSectorBodyText"));
        SectorBodyText->SetText(FText::FromString(TEXT("SECTOR MODE")));
        SectorBodyText->SetJustification(ETextJustify::Left);
        SectorBodyText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        SectorBodyText->SetFont(MissionUIStyle::GetHeaderFont(18));
        SectorPanelHost->SetContent(SectorBodyText);
    }
    else
    {
        SectorPanelHost->SetContent(SectorMapPanel);
        SectorMapPanel->SetOwnerNavDlg(this);

        UE_LOG(LogTemp, Warning,
            TEXT("MissionNavDlg: Created SectorMapPanel from %s"),
            *GetNameSafe(SectorMapPanelClass));
    }

    RightPanelBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavRightPanelBorder"));

    if (UHorizontalBoxSlot* RightSlot = RootContentRow->AddChildToHorizontalBox(RightPanelBorder))
    {
        RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    RightPanelColumn =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavRightPanelColumn"));

    RightPanelBorder->SetContent(RightPanelColumn);

    BuildRightPanels();
    ApplyPanelStyles();
}

void UMissionNavDlg::BuildNavModeButtons()
{
    if (!NavModeButtonBox || !MenuButtonClass)
    {
        return;
    }

    NavModeButtonBox->ClearChildren();
    NavModeButtons.Empty();

    CreateNavModeButton(TEXT("GALAXY"), NavModeButtonBox);
    CreateNavModeButton(TEXT("SYSTEM"), NavModeButtonBox);
    CreateNavModeButton(TEXT("SECTOR"), NavModeButtonBox);
}

UMenuButton* UMissionNavDlg::CreateNavModeButton(const FString& Label, UHorizontalBox* ParentBox)
{
    if (!ParentBox || !MenuButtonClass)
    {
        return nullptr;
    }

    UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
    if (!NewButton)
    {
        return nullptr;
    }

    NewButton->MenuOption = Label;
    NewButton->WidthOverride = 132.f;
    NewButton->HeightOverride = 34.f;
    NewButton->LabelFontSize = 14;

    if (UTextBlock* LabelText = Cast<UTextBlock>(NewButton->GetWidgetFromName(TEXT("Label"))))
    {
        LabelText->SetText(FText::FromString(Label));
    }

    if (UHorizontalBoxSlot* ButtonSlot = ParentBox->AddChildToHorizontalBox(NewButton))
    {
        ButtonSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        ButtonSlot->SetHorizontalAlignment(HAlign_Left);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    NewButton->OnSelected.RemoveDynamic(this, &UMissionNavDlg::OnNavModeButtonSelected);
    NewButton->OnSelected.AddDynamic(this, &UMissionNavDlg::OnNavModeButtonSelected);

    NewButton->OnHovered.RemoveDynamic(this, &UMissionNavDlg::OnNavModeButtonHovered);
    NewButton->OnHovered.AddDynamic(this, &UMissionNavDlg::OnNavModeButtonHovered);

    NavModeButtons.Add(NewButton);
    return NewButton;
}

void UMissionNavDlg::BuildRightPanels()
{
    if (!WidgetTree || !RightPanelColumn)
    {
        return;
    }

    RightPanelColumn->ClearChildren();

    FilterPanelHost =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavFilterPanelHost"));

    if (UVerticalBoxSlot* FilterPanelSlot = RightPanelColumn->AddChildToVerticalBox(FilterPanelHost))
    {
        FilterPanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
        FilterPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    FilterButtonGrid =
        WidgetTree->ConstructWidget<UUniformGridPanel>(
            UUniformGridPanel::StaticClass(),
            TEXT("MissionNavFilterButtonGrid"));

    if (UVerticalBoxSlot* GridSlot = FilterPanelHost->AddChildToVerticalBox(FilterButtonGrid))
    {
        GridSlot->SetPadding(FMargin(0.f));
        GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    BuildFilterButtons();

    ObjectListBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavObjectListBorder"));

    if (UVerticalBoxSlot* ObjectListBorderSlot = RightPanelColumn->AddChildToVerticalBox(ObjectListBorder))
    {
        ObjectListBorderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
        ObjectListBorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ObjectListPanel =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavObjectListPanel"));
    ObjectListBorder->SetContent(ObjectListPanel);

    ObjectListTitleBar =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavObjectTitleBar"));

    if (UVerticalBoxSlot* TitleSlot = ObjectListPanel->AddChildToVerticalBox(ObjectListTitleBar))
    {
        TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ObjectListTitleText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavObjectListTitleText"));
    ObjectListTitleText->SetText(FText::FromString(GetObjectPanelTitle()));
    ObjectListTitleText->SetJustification(ETextJustify::Center);
    ObjectListTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    ObjectListTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    ObjectListTitleBar->SetContent(ObjectListTitleText);

    ObjectListHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavObjectListHost"));
    ObjectListHost->SetWidthOverride(300.f);
    ObjectListHost->SetHeightOverride(220.f);

    if (UVerticalBoxSlot* ObjectListHostSlot = ObjectListPanel->AddChildToVerticalBox(ObjectListHost))
    {
        ObjectListHostSlot->SetPadding(FMargin(0.f));
        ObjectListHostSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ObjectListView =
        WidgetTree->ConstructWidget<UMissionNavObjectListView>(
            UMissionNavObjectListView::StaticClass(),
            TEXT("MissionNavObjectListView"));

    if (ObjectListView)
    {
        if (!ObjectListEntryWidgetClass)
        {
            ObjectListEntryWidgetClass = UMissionNavObjectLVElement::StaticClass();
        }

        ObjectListView->SetEntryWidgetClassPublic(ObjectListEntryWidgetClass);
        ObjectListView->SetSelectionMode(ESelectionMode::Single);
        ObjectListView->OnItemSelectionChanged().Clear();
        ObjectListView->OnItemSelectionChanged().AddUObject(this, &UMissionNavDlg::OnObjectSelectionChanged);
        ObjectListHost->SetContent(ObjectListView);
    }

    DetailBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavDetailBorder"));

    if (UVerticalBoxSlot* DetailBorderSlot = RightPanelColumn->AddChildToVerticalBox(DetailBorder))
    {
        DetailBorderSlot->SetPadding(FMargin(0.f));
        DetailBorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    DetailPanel =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavDetailPanel"));
    DetailBorder->SetContent(DetailPanel);

    DetailTitleBar =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavDetailTitleBar"));

    if (UVerticalBoxSlot* DetailTitleSlot = DetailPanel->AddChildToVerticalBox(DetailTitleBar))
    {
        DetailTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    DetailTitleText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavDetailTitleText"));
    DetailTitleText->SetText(FText::FromString(GetDetailPanelTitle()));
    DetailTitleText->SetJustification(ETextJustify::Center);
    DetailTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    DetailTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    DetailTitleBar->SetContent(DetailTitleText);

    DetailHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavDetailHost"));
    DetailHost->SetWidthOverride(300.f);
    DetailHost->SetHeightOverride(180.f);

    if (UVerticalBoxSlot* DetailHostSlot = DetailPanel->AddChildToVerticalBox(DetailHost))
    {
        DetailHostSlot->SetPadding(FMargin(0.f));
        DetailHostSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    DetailScrollBox =
        WidgetTree->ConstructWidget<UScrollBox>(
            UScrollBox::StaticClass(),
            TEXT("MissionNavDetailScrollBox"));

    DetailScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
    DetailScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
    DetailScrollBox->SetAnimateWheelScrolling(true);
    DetailScrollBox->SetIsFocusable(true);

    DetailBodyText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavDetailBodyText"));
    DetailBodyText->SetText(FText::FromString(TEXT("NO OBJECT SELECTED")));
    DetailBodyText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
    DetailBodyText->SetFont(MissionUIStyle::GetInfoValueFont());
    DetailBodyText->SetJustification(ETextJustify::Left);
    DetailBodyText->SetAutoWrapText(true);
    DetailBodyText->SetWrapTextAt(280.0f);

    DetailScrollBox->AddChild(DetailBodyText);

    if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(DetailBodyText->Slot))
    {
        ScrollSlot->SetPadding(FMargin(6.f, 4.f, 6.f, 6.f));
    }

    DetailHost->SetContent(DetailScrollBox);

    ApplyPanelStyles();
    RefreshFilterSelection();
    RebuildObjectList();
    RefreshDetailPanel();
}

void UMissionNavDlg::BuildFilterButtons()
{
    if (!FilterButtonGrid || !MenuButtonClass)
    {
        return;
    }

    FilterButtonGrid->ClearChildren();
    FilterButtons.Empty();

    CreateFilterButton(TEXT("SYSTEM"), 0, 0);
    CreateFilterButton(TEXT("PLANET"), 0, 1);
    CreateFilterButton(TEXT("SECTOR"), 1, 0);
    CreateFilterButton(TEXT("STATION"), 1, 1);
    CreateFilterButton(TEXT("STARSHIP"), 2, 0);
    CreateFilterButton(TEXT("FIGHTER"), 2, 1);
}

UMenuButton* UMissionNavDlg::CreateFilterButton(const FString& Label, int32 Row, int32 Column)
{
    if (!FilterButtonGrid || !MenuButtonClass)
    {
        return nullptr;
    }

    UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
    if (!NewButton)
    {
        return nullptr;
    }

    NewButton->MenuOption = Label;
    NewButton->WidthOverride = 118.f;
    NewButton->HeightOverride = 30.f;
    NewButton->LabelFontSize = 13;

    if (UTextBlock* LabelText = Cast<UTextBlock>(NewButton->GetWidgetFromName(TEXT("Label"))))
    {
        LabelText->SetText(FText::FromString(Label));
    }

    if (UUniformGridSlot* GridSlot = FilterButtonGrid->AddChildToUniformGrid(NewButton, Row, Column))
    {
        GridSlot->SetHorizontalAlignment(HAlign_Fill);
        GridSlot->SetVerticalAlignment(VAlign_Fill);
    }

    NewButton->OnSelected.RemoveDynamic(this, &UMissionNavDlg::OnFilterButtonSelected);
    NewButton->OnSelected.AddDynamic(this, &UMissionNavDlg::OnFilterButtonSelected);

    NewButton->OnHovered.RemoveDynamic(this, &UMissionNavDlg::OnFilterButtonHovered);
    NewButton->OnHovered.AddDynamic(this, &UMissionNavDlg::OnFilterButtonHovered);

    FilterButtons.Add(NewButton);
    return NewButton;
}

void UMissionNavDlg::ApplyPanelStyles()
{
    auto ApplyTexturedPanelWhite = [this](UBorder* BorderWidget)
        {
            if (!BorderWidget)
            {
                return;
            }

            if (RightPanelBackgroundTexture)
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(RightPanelBackgroundTexture);
                Brush.ImageSize = FVector2D(256.f, 256.f);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                BorderWidget->SetBrush(Brush);
            }

            BorderWidget->SetBrushColor(FLinearColor::White);
        };

    auto ApplyTexturedPanelDark = [this](UBorder* BorderWidget)
        {
            if (!BorderWidget)
            {
                return;
            }

            if (RightPanelBackgroundTexture)
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(RightPanelBackgroundTexture);
                Brush.ImageSize = FVector2D(256.f, 256.f);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                BorderWidget->SetBrush(Brush);
            }

            BorderWidget->SetBrushColor(MissionUIStyle::PanelBG);
        };

    if (LeftPanelBorder)
    {
        LeftPanelBorder->SetBrush(FSlateBrush());
        LeftPanelBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
    }

    ApplyTexturedPanelDark(RightPanelBorder);
    ApplyTexturedPanelWhite(ObjectListBorder);
    ApplyTexturedPanelWhite(ObjectListTitleBar);
    ApplyTexturedPanelWhite(DetailBorder);
    ApplyTexturedPanelWhite(DetailTitleBar);
}

void UMissionNavDlg::RefreshNavModeSelection()
{
    FString ActiveLabel;

    switch (CurrentNavMode)
    {
    case EMissionNavMode::GALAXY: ActiveLabel = TEXT("GALAXY"); break;
    case EMissionNavMode::SYSTEM: ActiveLabel = TEXT("SYSTEM"); break;
    case EMissionNavMode::SECTOR: ActiveLabel = TEXT("SECTOR"); break;
    default: break;
    }

    for (UMenuButton* Button : NavModeButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button->MenuOption == ActiveLabel);
        }
    }
}

void UMissionNavDlg::RefreshFilterSelection()
{
    const FString ActiveLabel = GetFilterModeLabel(CurrentFilterMode);

    for (UMenuButton* Button : FilterButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button->MenuOption == ActiveLabel);
        }
    }

    if (ObjectListTitleText)
    {
        ObjectListTitleText->SetText(FText::FromString(GetObjectPanelTitle()));
        ObjectListTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        ObjectListTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    }
}

void UMissionNavDlg::RefreshObjectListPanel()
{
    if (ObjectListTitleText)
    {
        ObjectListTitleText->SetText(FText::FromString(GetObjectPanelTitle()));
        ObjectListTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        ObjectListTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    }

    RebuildObjectList();
}

void UMissionNavDlg::RefreshDetailPanel()
{
    if (DetailTitleText)
    {
        DetailTitleText->SetText(FText::FromString(GetDetailPanelTitle()));
        DetailTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        DetailTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    }

    if (DetailBodyText)
    {
        DetailBodyText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
        DetailBodyText->SetFont(MissionUIStyle::GetInfoValueFont());

        if (CurrentNavMode == EMissionNavMode::GALAXY && !SelectedSystemName.IsEmpty())
        {
            DetailBodyText->SetText(FText::FromString(BuildGalaxySystemDetailText(SelectedSystemName)));
        }
        else if (SelectedObjectItem)
        {
            DetailBodyText->SetText(FText::FromString(SelectedObjectItem->GetDetailText()));
        }
        else
        {
            DetailBodyText->SetText(FText::FromString(TEXT("NO OBJECT SELECTED")));
        }
    }
}

void UMissionNavDlg::SetNavMode(EMissionNavMode NewMode)
{
    CurrentNavMode = NewMode;

    switch (CurrentNavMode)
    {
    case EMissionNavMode::GALAXY:
        CurrentFilterMode = EMissionNavFilterMode::SYSTEM;
        if (Manager) Manager->NavModeGalaxy();
        break;

    case EMissionNavMode::SYSTEM:
        CurrentFilterMode = EMissionNavFilterMode::PLANET;
        if (Manager) Manager->NavModeSystem();
        break;

    case EMissionNavMode::SECTOR:
        CurrentFilterMode = EMissionNavFilterMode::SECTOR;
        if (Manager) Manager->NavModeSector();

        // Sector mode is mission-region locked.
        // Clear selection unless it is a sector/unit row for detail focus only.
        if (!SelectedObjectItem ||
            (SelectedObjectItem->GetObjectType() != EMissionNavObjectType::Sector &&
                SelectedObjectItem->GetObjectType() != EMissionNavObjectType::Station &&
                SelectedObjectItem->GetObjectType() != EMissionNavObjectType::Starship &&
                SelectedObjectItem->GetObjectType() != EMissionNavObjectType::Fighter))
        {
            SelectedObjectItem = nullptr;
        }
        break;

    default:
        break;
    }

    if (NavSwitcher)
    {
        NavSwitcher->SetActiveWidgetIndex(static_cast<int32>(CurrentNavMode));
    }

    if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
    {
        GalaxyMapPanel->SetFocus();
    }
    else if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
    {
        SystemMapPanel->SetFocus();
    }
    else if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
    {
        SectorMapPanel->SetFocus();
    }

    RefreshNavModeSelection();
    RefreshFilterSelection();
    RefreshFromMission();
}

void UMissionNavDlg::SetFilterMode(EMissionNavFilterMode NewMode)
{
    CurrentFilterMode = NewMode;

    RefreshFilterSelection();

    // Update sector/system context first, then rebuild list against that locked context.
    SyncSubPanels();
    RefreshObjectListPanel();
    RefreshDetailPanel();
}

FString UMissionNavDlg::GetFilterModeLabel(EMissionNavFilterMode Mode) const
{
    switch (Mode)
    {
    case EMissionNavFilterMode::SYSTEM:   return TEXT("SYSTEM");
    case EMissionNavFilterMode::PLANET:   return TEXT("PLANET");
    case EMissionNavFilterMode::SECTOR:   return TEXT("SECTOR");
    case EMissionNavFilterMode::STATION:  return TEXT("STATION");
    case EMissionNavFilterMode::STARSHIP: return TEXT("STARSHIP");
    case EMissionNavFilterMode::FIGHTER:  return TEXT("FIGHTER");
    default:                              return TEXT("UNKNOWN");
    }
}

FString UMissionNavDlg::GetObjectPanelTitle() const
{
    return GetFilterModeLabel(CurrentFilterMode);
}

FString UMissionNavDlg::GetDetailPanelTitle() const
{
    return TEXT("DETAIL PANEL");
}

EMissionNavObjectType UMissionNavDlg::GetCurrentObjectType() const
{
    switch (CurrentFilterMode)
    {
    case EMissionNavFilterMode::SYSTEM:   return EMissionNavObjectType::System;
    case EMissionNavFilterMode::PLANET:   return EMissionNavObjectType::Planet;
    case EMissionNavFilterMode::SECTOR:   return EMissionNavObjectType::Sector;
    case EMissionNavFilterMode::STATION:  return EMissionNavObjectType::Station;
    case EMissionNavFilterMode::STARSHIP: return EMissionNavObjectType::Starship;
    case EMissionNavFilterMode::FIGHTER:  return EMissionNavObjectType::Fighter;
    default:                              return EMissionNavObjectType::None;
    }
}

void UMissionNavDlg::AddObjectItem(
    EMissionNavObjectType ObjectType,
    int32 Index,
    const FString& PrimaryText,
    const FString& SecondaryText,
    const FString& DetailText)
{
    if (!ObjectListView)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] AddObjectItem: ObjectListView is null"));
        return;
    }

    UMissionNavObjectListObject* Item = NewObject<UMissionNavObjectListObject>(this);
    if (!Item)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] AddObjectItem: Failed to allocate item"));
        return;
    }

    Item->InitObjectRow(
        ObjectType,
        Index,
        PrimaryText,
        SecondaryText,
        DetailText);

    ObjectItems.Add(Item);
    ObjectListView->AddItem(Item);

    UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] AddObjectItem: [%d] %s / %s"),
        Index,
        *PrimaryText,
        *SecondaryText);
}

void UMissionNavDlg::RebuildObjectList()
{
    UMissionNavObjectListObject* PreviousSelection = SelectedObjectItem;

    ObjectItems.Empty();

    if (!ObjectListView)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] RebuildObjectList: ObjectListView is null"));
        return;
    }

    ObjectListView->ClearListItems();

    MissionPtr = ResolveMission();
    if (!MissionPtr)
    {
        SelectedObjectItem = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] RebuildObjectList: MissionPtr is null"));
        RefreshDetailPanel();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] RebuildObjectList: Filter=%s Mission=%p"),
        *GetFilterModeLabel(CurrentFilterMode),
        MissionPtr);

    switch (GetCurrentObjectType())
    {
    case EMissionNavObjectType::System:
        BuildSystemObjects();
        break;

    case EMissionNavObjectType::Planet:
        BuildPlanetObjects();
        break;

    case EMissionNavObjectType::Sector:
        BuildSectorObjects();
        break;

    case EMissionNavObjectType::Station:
        BuildMissionElementObjects(EMissionNavObjectType::Station);
        break;

    case EMissionNavObjectType::Starship:
        BuildMissionElementObjects(EMissionNavObjectType::Starship);
        break;

    case EMissionNavObjectType::Fighter:
        BuildMissionElementObjects(EMissionNavObjectType::Fighter);
        break;

    default:
        break;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] RebuildObjectList: Added %d items"), ObjectItems.Num());

    if (ObjectItems.Num() == 0)
    {
        AddObjectItem(
            GetCurrentObjectType(),
            0,
            TEXT("NO DATA"),
            TEXT("DEBUG"),
            TEXT("MISSION NAV PANEL RECEIVED NO REAL DATA FOR THIS FILTER."));
    }

    SelectedObjectItem = nullptr;

    if (PreviousSelection)
    {
        const FString PreviousPrimary = PreviousSelection->GetPrimaryText();
        const EMissionNavObjectType PreviousType = PreviousSelection->GetObjectType();

        for (UMissionNavObjectListObject* Item : ObjectItems)
        {
            if (!Item)
            {
                continue;
            }

            if (Item->GetObjectType() == PreviousType &&
                Item->GetPrimaryText().Equals(PreviousPrimary, ESearchCase::IgnoreCase))
            {
                SelectedObjectItem = Item;
                break;
            }
        }
    }

    if (SelectedObjectItem)
    {
        ObjectListView->SetSelectedItem(SelectedObjectItem);
    }

    SyncSubPanels();
    RefreshDetailPanel();
}

void UMissionNavDlg::BuildSystemObjects()
{
    if (!MissionPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] BuildSystemObjects: MissionPtr is null"));
        return;
    }

    StarSystem* System = MissionPtr->GetStarSystem();
    if (!System)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] BuildSystemObjects: Mission star system is null"));
        return;
    }

    const FString Primary = FString(System->GetName());
    const FString Secondary = TEXT("STARSYSTEM");
    const FString Detail = FString::Printf(
        TEXT("%s\n\nTYPE: STAR SYSTEM\nRADIUS: %.0f"),
        *Primary,
        System->GetRadius());

    AddObjectItem(
        EMissionNavObjectType::System,
        0,
        Primary,
        Secondary,
        Detail);
}

void UMissionNavDlg::BuildPlanetObjects()
{
    if (!MissionPtr)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] BuildPlanetObjects called"));

    StarSystem* System = MissionPtr->GetStarSystem();
    if (!System)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] BuildPlanetObjects: Mission star system is null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] System = %p Name=%s Radius=%.0f"),
        System,
        *FString(System->GetName()),
        System->GetRadius());

    int32 Index = 0;

    ListIter<OrbitalBody> StarIter = System->GetBodies();
    while (++StarIter)
    {
        OrbitalBody* StarBody = StarIter.value();
        if (!StarBody)
        {
            continue;
        }

        ListIter<OrbitalBody> PlanetIter = StarBody->Satellites();
        while (++PlanetIter)
        {
            OrbitalBody* Planet = PlanetIter.value();
            if (!Planet)
            {
                continue;
            }

            const FString Primary = FString(Planet->GetName());
            const FString Secondary = (Planet->GetType() == Orbital::MOON) ? TEXT("MOON") : TEXT("PLANET");
            const FString Detail = FString::Printf(
                TEXT("%s\n\nTYPE: %s\nORBIT: %.0f\nRADIUS: %.0f"),
                *Primary,
                *Secondary,
                Planet->Orbit(),
                Planet->Radius());

            AddObjectItem(
                EMissionNavObjectType::Planet,
                Index++,
                Primary,
                Secondary,
                Detail);

            ListIter<OrbitalBody> MoonIter = Planet->Satellites();
            while (++MoonIter)
            {
                OrbitalBody* Moon = MoonIter.value();
                if (!Moon)
                {
                    continue;
                }

                const FString MoonPrimary = FString::Printf(
                    TEXT("- %s"),
                    *FString(Moon->GetName()));

                const FString MoonSecondary = TEXT("MOON");
                const FString MoonDetail = FString::Printf(
                    TEXT("%s\n\nTYPE: MOON\nORBIT: %.0f\nRADIUS: %.0f"),
                    *FString(Moon->GetName()),
                    Moon->Orbit(),
                    Moon->Radius());

                AddObjectItem(
                    EMissionNavObjectType::Planet,
                    Index++,
                    MoonPrimary,
                    MoonSecondary,
                    MoonDetail);
            }
        }
    }
}

void UMissionNavDlg::BuildSectorObjects()
{
    if (!MissionPtr)
    {
        return;
    }

    StarSystem* System = MissionPtr->GetStarSystem();
    if (!System)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] BuildSectorObjects: Mission star system is null"));
        return;
    }

    FString MissionRegionName = ANSI_TO_TCHAR(MissionPtr->GetRegion());
    MissionRegionName = MissionRegionName.TrimStartAndEnd();

    if (MissionRegionName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] BuildSectorObjects: Mission region is empty"));
        return;
    }

    ListIter<OrbitalRegion> RegionIter = System->GetAllRegions();
    while (++RegionIter)
    {
        OrbitalRegion* Region = RegionIter.value();
        if (!Region)
        {
            continue;
        }

        const FString RegionName = FString(Region->GetName()).TrimStartAndEnd();

        if (!RegionName.Equals(MissionRegionName, ESearchCase::IgnoreCase))
        {
            continue;
        }

        const FString Primary = RegionName;
        const FString Secondary = TEXT("SECTOR");
        const FString Detail = FString::Printf(
            TEXT("%s\n\nTYPE: SECTOR\nRADIUS: %.0f\nGRID: %.0f"),
            *RegionName,
            Region->Radius(),
            Region->GetGridSpace());

        AddObjectItem(
            EMissionNavObjectType::Sector,
            0,
            Primary,
            Secondary,
            Detail);

        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionNavDlg] BuildSectorObjects: Mission region '%s' was not found in runtime system '%s'"),
        *MissionRegionName,
        *FString(System->GetName()));
}

void UMissionNavDlg::BuildMissionElementObjects(EMissionNavObjectType ObjectType)
{
    if (!MissionPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionNavDlg] BuildMissionElementObjects: MissionPtr is null"));
        return;
    }

    const FString MissionRegion =
        FString(ANSI_TO_TCHAR(MissionPtr->GetRegion())).TrimStartAndEnd();

    const FString MissionRegionUpper = MissionRegion.ToUpper();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionNavDlg] BuildMissionElementObjects: Mission='%s' MissionRegion='%s' FilterType=%d"),
        MissionPtr ? *FString(MissionPtr->GetName()) : TEXT("NONE"),
        *MissionRegion,
        static_cast<int32>(ObjectType));

    // -------------------------------------------------------------
    // Local indicator helper (matches your existing system)
    // -------------------------------------------------------------
    auto GetCombatGroupUnitIndicator =
        [](const FS_CombatGroup& Group, const FS_CombatGroupUnit& Unit) -> FString
        {
            const FString UnitClass = Unit.UnitClass.TrimStartAndEnd().ToUpper();

            if (UnitClass == TEXT("DRONE"))       return TEXT("DR");
            if (UnitClass == TEXT("FIGHTER"))     return TEXT("VF");
            if (UnitClass == TEXT("ATTACK"))      return TEXT("VA");
            if (UnitClass == TEXT("LCA"))         return TEXT("LC");

            if (UnitClass == TEXT("COURIER"))     return TEXT("CR");
            if (UnitClass == TEXT("CARGO"))       return TEXT("CG");
            if (UnitClass == TEXT("FREIGHTER"))   return TEXT("FT");

            if (UnitClass == TEXT("CORVETTE"))    return TEXT("CVT");
            if (UnitClass == TEXT("FRIGATE"))     return TEXT("FF");
            if (UnitClass == TEXT("DESTROYER"))   return TEXT("DD");
            if (UnitClass == TEXT("CRUISER"))     return TEXT("CA");
            if (UnitClass == TEXT("BATTLESHIP"))  return TEXT("BB");
            if (UnitClass == TEXT("CARRIER"))     return TEXT("CV");
            if (UnitClass == TEXT("DREADNAUGHT")) return TEXT("DN");

            if (UnitClass == TEXT("STATION"))     return TEXT("ST");
            if (UnitClass == TEXT("STARBASE"))    return TEXT("SB");
            if (UnitClass == TEXT("FARCASTER"))   return TEXT("FC");

            if (UnitClass == TEXT("MINE"))        return TEXT("MN");
            if (UnitClass == TEXT("COMSAT"))      return TEXT("CS");
            if (UnitClass == TEXT("DEFSAT"))      return TEXT("DS");
            if (UnitClass == TEXT("SWACS"))       return TEXT("SW");

            if (UnitClass == TEXT("BUILDING"))    return TEXT("BLD");
            if (UnitClass == TEXT("FACTORY"))     return TEXT("FAC");
            if (UnitClass == TEXT("SAM"))         return TEXT("SAM");
            if (UnitClass == TEXT("EWR"))         return TEXT("EWR");
            if (UnitClass == TEXT("C3I"))         return TEXT("C3");

            switch (Group.Type)
            {
            case ECOMBATGROUP_TYPE::STATION:             return TEXT("ST");
            case ECOMBATGROUP_TYPE::STARBASE:            return TEXT("SB");
            case ECOMBATGROUP_TYPE::BATTERY:             return TEXT("SAM");
            case ECOMBATGROUP_TYPE::MINEFIELD:           return TEXT("MN");

            case ECOMBATGROUP_TYPE::CARRIER_GROUP:       return TEXT("CV");
            case ECOMBATGROUP_TYPE::BATTLE_GROUP:        return TEXT("CA");
            case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:  return TEXT("DD");

            case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:    return TEXT("VF");
            case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:  return TEXT("VF");
            case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:     return TEXT("VA");
            case ECOMBATGROUP_TYPE::LCA_SQUADRON:        return TEXT("LC");

            default:
                return TEXT("UN");
            }
        };

    struct FNavRow
    {
        FString Primary;
        FString Secondary;
        FString Detail;
    };

    TArray<FNavRow> Rows;

    const TMap<FName, FS_CombatGroup>& AllGroups = CombatGroupRegistry::GetAll();

    for (const TPair<FName, FS_CombatGroup>& Pair : AllGroups)
    {
        const FS_CombatGroup& Group = Pair.Value;

        const FString GroupRegion = Group.Region.TrimStartAndEnd();
        const FString GroupRegionUpper = GroupRegion.ToUpper();

        // -------------------------------------------------------------
        // Hard region lock
        // -------------------------------------------------------------
        if (MissionRegionUpper.IsEmpty() || GroupRegionUpper != MissionRegionUpper)
        {
            continue;
        }

        bool bUseUnits = false;
        FString TypeLabel;

        switch (ObjectType)
        {
        case EMissionNavObjectType::Station:
            bUseUnits =
                Group.Type == ECOMBATGROUP_TYPE::STATION ||
                Group.Type == ECOMBATGROUP_TYPE::STARBASE ||
                Group.Type == ECOMBATGROUP_TYPE::BATTERY ||
                Group.Type == ECOMBATGROUP_TYPE::MINEFIELD;
            TypeLabel = TEXT("STATION");
            break;

        case EMissionNavObjectType::Starship:
            bUseUnits =
                Group.Type == ECOMBATGROUP_TYPE::CARRIER_GROUP ||
                Group.Type == ECOMBATGROUP_TYPE::BATTLE_GROUP ||
                Group.Type == ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
            TypeLabel = TEXT("STARSHIP");
            break;

        case EMissionNavObjectType::Fighter:
            bUseUnits =
                Group.Type == ECOMBATGROUP_TYPE::FIGHTER_SQUADRON ||
                Group.Type == ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON ||
                Group.Type == ECOMBATGROUP_TYPE::ATTACK_SQUADRON ||
                Group.Type == ECOMBATGROUP_TYPE::LCA_SQUADRON;
            TypeLabel = TEXT("FIGHTER");
            break;

        default:
            break;
        }

        if (!bUseUnits)
        {
            continue;
        }

        // -------------------------------------------------------------
        // Build rows from UNITS (not groups)
        // -------------------------------------------------------------
        for (const FS_CombatGroupUnit& Unit : Group.Unit)
        {
            const FString RawName = Unit.UnitName.TrimStartAndEnd();
            if (RawName.IsEmpty())
            {
                continue;
            }

            FString UnitRegion = Unit.UnitRegion.TrimStartAndEnd();
            if (UnitRegion.IsEmpty())
            {
                UnitRegion = GroupRegion;
            }

            const FString UnitRegionUpper = UnitRegion.ToUpper();

            if (MissionRegionUpper.IsEmpty() || UnitRegionUpper != MissionRegionUpper)
            {
                continue;
            }

            const FString Indicator = GetCombatGroupUnitIndicator(Group, Unit);

            const FString Primary = RawName;
            const FString Secondary = Indicator;

            const FVector UnitLoc =
                !Unit.UnitLoc.IsNearlyZero() ? Unit.UnitLoc : Group.Location;

            FNavRow Row;
            Row.Primary = Primary;
            Row.Secondary = Secondary;
            Row.Detail = FString::Printf(
                TEXT("%s\n\nTYPE: %s\nDESIGN: %s\nREGION: %s\nIFF: %d\nLOCATION: X %.0f  Y %.0f  Z %.0f\nGROUP: %s\nGROUP TYPE: %s\nCOUNT: %d"),
                *Primary,
                *TypeLabel,
                Unit.UnitDesign.IsEmpty() ? TEXT("UNKNOWN") : *Unit.UnitDesign,
                *UnitRegion,
                Group.Iff,
                UnitLoc.X,
                UnitLoc.Y,
                UnitLoc.Z,
                Group.DisplayName.IsEmpty() ? *Pair.Key.ToString() : *Group.DisplayName,
                *UFormattingUtils::GetGroupTypeDisplayName(Group.Type),
                Unit.UnitCount);

            Rows.Add(Row);

            UE_LOG(LogTemp, Warning,
                TEXT("[MissionNavDlg]   ACCEPT UNIT: Name='%s' Indicator='%s' Region='%s'"),
                *Primary,
                *Secondary,
                *UnitRegion);
        }
    }

    Rows.Sort([](const FNavRow& A, const FNavRow& B)
        {
            return A.Primary < B.Primary;
        });

    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        AddObjectItem(
            ObjectType,
            Index,
            Rows[Index].Primary,
            Rows[Index].Secondary,
            Rows[Index].Detail);
    }
}

void UMissionNavDlg::OnNavModeButtonSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    const FString& Option = SelectedButton->MenuOption;

    if (Option == TEXT("GALAXY"))
    {
        SetNavMode(EMissionNavMode::GALAXY);
    }
    else if (Option == TEXT("SYSTEM"))
    {
        SetNavMode(EMissionNavMode::SYSTEM);
    }
    else if (Option == TEXT("SECTOR"))
    {
        // Force sector mode back to mission-region baseline.
        SelectedObjectItem = nullptr;

        if (ObjectListView)
        {
            ObjectListView->ClearSelection();
        }

        SetNavMode(EMissionNavMode::SECTOR);
    }
}

void UMissionNavDlg::OnNavModeButtonHovered(UMenuButton* HoveredButton)
{
}

void UMissionNavDlg::OnFilterButtonSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    const FString& Option = SelectedButton->MenuOption;

    if (Option == TEXT("SYSTEM"))
    {
        SetNavMode(EMissionNavMode::GALAXY);
        SetFilterMode(EMissionNavFilterMode::SYSTEM);
    }
    else if (Option == TEXT("PLANET"))
    {
        SetNavMode(EMissionNavMode::SYSTEM);
        SetFilterMode(EMissionNavFilterMode::PLANET);
    }
    else if (Option == TEXT("SECTOR"))
    {
        SetNavMode(EMissionNavMode::SECTOR);
        SetFilterMode(EMissionNavFilterMode::SECTOR);
    }
    else if (Option == TEXT("STATION"))
    {
        SetNavMode(EMissionNavMode::SECTOR);
        SetFilterMode(EMissionNavFilterMode::STATION);
    }
    else if (Option == TEXT("STARSHIP"))
    {
        SetNavMode(EMissionNavMode::SECTOR);
        SetFilterMode(EMissionNavFilterMode::STARSHIP);
    }
    else if (Option == TEXT("FIGHTER"))
    {
        SetNavMode(EMissionNavMode::SECTOR);
        SetFilterMode(EMissionNavFilterMode::FIGHTER);
    }
}

void UMissionNavDlg::OnFilterButtonHovered(UMenuButton* HoveredButton)
{
}

void UMissionNavDlg::OnObjectSelectionChanged(UObject* SelectedItem)
{
    SelectedObjectItem = Cast<UMissionNavObjectListObject>(SelectedItem);

    if (!SelectedObjectItem)
    {
        RefreshDetailPanel();
        return;
    }

    const EMissionNavObjectType ObjectType = SelectedObjectItem->GetObjectType();

    // -------------------------------------------------
    // SYSTEM
    // -------------------------------------------------
    if (ObjectType == EMissionNavObjectType::System)
    {
        const FString TargetSystemName =
            SelectedObjectItem->GetPrimaryText().TrimStartAndEnd();

        if (!TargetSystemName.IsEmpty())
        {
            SelectedSystemName = TargetSystemName;
        }
        

        if (SystemMapPanel)
        {
            SystemMapPanel->SetViewedSystemName(SelectedSystemName);
            SystemMapPanel->SetSelectedBodyName(TEXT(""));
            SystemMapPanel->ShowSystemOverview();
            SystemMapPanel->SetFocus();
        }

        CurrentNavMode = EMissionNavMode::SYSTEM;

        if (NavSwitcher)
        {
            NavSwitcher->SetActiveWidgetIndex(static_cast<int32>(EMissionNavMode::SYSTEM));
        }

        RefreshNavModeSelection();
        SyncSubPanels();
        RefreshDetailPanel();
        return;
    }

    // -------------------------------------------------
    // PLANET
    // -------------------------------------------------
    if (ObjectType == EMissionNavObjectType::Planet)
    {
        FString TargetName = SelectedObjectItem->GetPrimaryText();
        TargetName = TargetName.Replace(TEXT("- "), TEXT(""));
        TargetName = TargetName.TrimStartAndEnd();

        CurrentNavMode = EMissionNavMode::SYSTEM;

        if (NavSwitcher)
        {
            NavSwitcher->SetActiveWidgetIndex(static_cast<int32>(EMissionNavMode::SYSTEM));
        }

        if (SystemMapPanel)
        {
            if (!SelectedSystemName.IsEmpty())
            {
                SystemMapPanel->SetViewedSystemName(SelectedSystemName);
            }

            SystemMapPanel->SetSelectedBodyName(TargetName);
            SystemMapPanel->CenterOnBodyByName(TargetName);
            SystemMapPanel->SetFocus();
        }

        RefreshNavModeSelection();
        SyncSubPanels();
        RefreshDetailPanel();
        return;
    }

    // -------------------------------------------------
    // SECTOR OBJECTS (Station / Ship / Fighter)
    // -------------------------------------------------
    if (ObjectType == EMissionNavObjectType::Sector ||
        ObjectType == EMissionNavObjectType::Station ||
        ObjectType == EMissionNavObjectType::Starship ||
        ObjectType == EMissionNavObjectType::Fighter)
    {
        CurrentNavMode = EMissionNavMode::SECTOR;

        if (NavSwitcher)
        {
            NavSwitcher->SetActiveWidgetIndex(static_cast<int32>(EMissionNavMode::SECTOR));
        }

        RefreshNavModeSelection();
        SyncSubPanels();

        if (SectorMapPanel)
        {
            SectorMapPanel->SetFocus();

            if (ObjectType == EMissionNavObjectType::Station ||
                ObjectType == EMissionNavObjectType::Starship ||
                ObjectType == EMissionNavObjectType::Fighter)
            {
                if (MissionPtr)
                {
                    const FString TargetName =
                        SelectedObjectItem->GetPrimaryText().TrimStartAndEnd();

                    UE_LOG(LogTemp, Warning,
                        TEXT("[MissionNavDlg] Selected row: Type=%d Name='%s'"),
                        (int32)ObjectType,
                        *TargetName);

                    const FString MissionRegion =
                        FString(ANSI_TO_TCHAR(MissionPtr->GetRegion()))
                        .TrimStartAndEnd();

                    MissionElement* BestMatch = nullptr;

                    // -------------------------------------------------
                    // EXACT NAME MATCH ONLY (NO NORMALIZATION)
                    // -------------------------------------------------
                    ListIter<MissionElement> ElemIter = MissionPtr->GetElements();
                    while (++ElemIter)
                    {
                        MissionElement* Elem = ElemIter.value();
                        if (!Elem)
                        {
                            continue;
                        }

                        const FString ElemRegion =
                            FString(ANSI_TO_TCHAR(Elem->GetRegion()))
                            .TrimStartAndEnd();

                        if (!ElemRegion.Equals(MissionRegion, ESearchCase::IgnoreCase))
                        {
                            continue;
                        }

                        const FString ElemName =
                            FString(ANSI_TO_TCHAR(Elem->GetName()))
                            .TrimStartAndEnd();

                        if (ElemName.Equals(TargetName, ESearchCase::IgnoreCase))
                        {
                            BestMatch = Elem;
                            break;
                        }
                        
                        UE_LOG(LogTemp, Warning,
                            TEXT("[MissionNavDlg] Candidate MissionElement: Name='%s' Region='%s' IsStatic=%d IsStarship=%d IsSquadron=%d"),
                            ANSI_TO_TCHAR(Elem->GetName()),
                            ANSI_TO_TCHAR(Elem->GetRegion()),
                            Elem->IsStatic() ? 1 : 0,
                            Elem->IsStarship() ? 1 : 0,
                            Elem->IsSquadron() ? 1 : 0);
                    }

                    // -------------------------------------------------
                    // APPLY SELECTION + CENTER
                    // -------------------------------------------------
                    if (BestMatch)
                    {
                        SectorMapPanel->SetSelectedElement(BestMatch);
                        SectorMapPanel->CenterOnElement(BestMatch);

                        UE_LOG(LogTemp, Warning,
                            TEXT("[MissionNavDlg] Exact match: '%s'"),
                            ANSI_TO_TCHAR(BestMatch->GetName()));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning,
                            TEXT("[MissionNavDlg] No match for '%s'"),
                            *TargetName);
                    }
                }
            }
        }

        RefreshDetailPanel();
        return;
    }

    RefreshDetailPanel();
}

void UMissionNavDlg::OnZoomInClicked()
{
    if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
    {
        GalaxyMapPanel->ZoomIn();
    }
    else if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
    {
        SystemMapPanel->ZoomIn();
    }
    else if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
    {
        SectorMapPanel->ZoomIn();
    }

    if (Manager)
    {
        Manager->NavZoomIn();
    }
}

void UMissionNavDlg::OnZoomOutClicked()
{
    if (CurrentNavMode == EMissionNavMode::GALAXY && GalaxyMapPanel)
    {
        GalaxyMapPanel->ZoomOut();
    }
    else if (CurrentNavMode == EMissionNavMode::SYSTEM && SystemMapPanel)
    {
        SystemMapPanel->ZoomOut();
    }
    else if (CurrentNavMode == EMissionNavMode::SECTOR && SectorMapPanel)
    {
        SectorMapPanel->ZoomOut();
    }

    if (Manager)
    {
        Manager->NavZoomOut();
    }
}

void UMissionNavDlg::HandleGalaxySystemSelected(const FString& InSystemName)
{
    if (InSystemName.IsEmpty())
    {
        return;
    }

    SelectedSystemName = InSystemName;

    SyncGalaxyMissionAndSelectionState();
    SyncSubPanels();
    UpdateSystemDetailsPanel(SelectedSystemName);

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionNavDlg] HandleGalaxySystemSelected: Mission=%s Selected=%s"),
        *CurrentMissionSystemName,
        *SelectedSystemName);
}

void UMissionNavDlg::HandleGalaxySystemActivated(const FString& InSystemName)
{
    if (InSystemName.IsEmpty())
    {
        return;
    }

    SelectedSystemName = InSystemName;

    if (SystemMapPanel)
    {
        SystemMapPanel->SetViewedSystemName(SelectedSystemName);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionNavDlg] HandleGalaxySystemActivated: %s"),
        *SelectedSystemName);

    SetNavMode(EMissionNavMode::SYSTEM);
}

void UMissionNavDlg::HandleSectorMissionElementSelected(MissionElement* InElement)
{
    if (!InElement || !MissionPtr)
    {
        return;
    }

    auto NormalizeName = [](const FString& InName) -> FString
        {
            FString Name = InName.TrimStartAndEnd();

            TArray<FString> Parts;
            Name.ParseIntoArray(Parts, TEXT(" "), true);

            // Strip leading ordinal token: "1st", "2nd", "3rd", "4th", etc.
            if (Parts.Num() > 0)
            {
                const FString First = Parts[0].ToLower();

                const bool bLooksOrdinal =
                    First.EndsWith(TEXT("st")) ||
                    First.EndsWith(TEXT("nd")) ||
                    First.EndsWith(TEXT("rd")) ||
                    First.EndsWith(TEXT("th"));

                bool bAllDigitsPrefix = true;
                for (int32 i = 0; i < First.Len(); ++i)
                {
                    const TCHAR Ch = First[i];
                    if (!FChar::IsDigit(Ch))
                    {
                        if (i >= First.Len() - 2)
                        {
                            break;
                        }

                        bAllDigitsPrefix = false;
                        break;
                    }
                }

                if (bLooksOrdinal && bAllDigitsPrefix)
                {
                    Parts.RemoveAt(0);
                }
            }

            // Strip common trailing container words
            const TSet<FString> TrailingWords =
            {
                TEXT("station"),
                TEXT("starbase"),
                TEXT("battery"),
                TEXT("minefield"),
                TEXT("squadron"),
                TEXT("wing"),
                TEXT("group")
            };

            while (Parts.Num() > 0)
            {
                const FString Last = Parts.Last().ToLower();
                if (TrailingWords.Contains(Last))
                {
                    Parts.RemoveAt(Parts.Num() - 1);
                }
                else
                {
                    break;
                }
            }

            return FString::Join(Parts, TEXT(" ")).TrimStartAndEnd();
        };

    const FString ElementName = FString(ANSI_TO_TCHAR(InElement->GetName())).TrimStartAndEnd();
    const FString ElementRegion = FString(ANSI_TO_TCHAR(InElement->GetRegion())).TrimStartAndEnd().ToUpper();
    const FString MissionRegion = FString(ANSI_TO_TCHAR(MissionPtr->GetRegion())).TrimStartAndEnd().ToUpper();

    if (!ElementRegion.Equals(MissionRegion, ESearchCase::IgnoreCase))
    {
        return;
    }

    const FString NormalizedElementName = NormalizeName(ElementName);

    SelectedObjectItem = nullptr;

    for (UMissionNavObjectListObject* Item : ObjectItems)
    {
        if (!Item)
        {
            continue;
        }

        const EMissionNavObjectType ItemType = Item->GetObjectType();

        if (ItemType != EMissionNavObjectType::Station &&
            ItemType != EMissionNavObjectType::Starship &&
            ItemType != EMissionNavObjectType::Fighter)
        {
            continue;
        }

        const FString ItemPrimary = Item->GetPrimaryText().TrimStartAndEnd();
        const FString NormalizedItemPrimary = NormalizeName(ItemPrimary);

        if (ItemPrimary.Equals(ElementName, ESearchCase::IgnoreCase) ||
            NormalizedItemPrimary.Equals(NormalizedElementName, ESearchCase::IgnoreCase))
        {
            SelectedObjectItem = Item;

            if (ObjectListView)
            {
                ObjectListView->SetSelectedItem(Item);
            }

            break;
        }
    }

    if (SectorMapPanel)
    {
        SectorMapPanel->SetSelectedElement(InElement);
    }

    RefreshDetailPanel();
}

bool UMissionNavDlg::ShouldShowMissionElementInBriefing(const MissionElement* Elem) const
{
    if (!Elem || !MissionPtr)
    {
        return false;
    }

    const int32 ElemIFF = Elem->GetIFF();
    const int32 MissionTeam = MissionPtr->GetTeam();

    if (ElemIFF == 0)
    {
        return true;
    }

    if (ElemIFF == MissionTeam)
    {
        return true;
    }

    if (Elem->IntelLevel() >= Intel::KNOWN)
    {
        return true;
    }

    return false;
}