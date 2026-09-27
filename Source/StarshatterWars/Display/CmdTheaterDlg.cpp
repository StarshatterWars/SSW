/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdTheaterDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdTheaterDlg implementation.

    CmdTheaterPanel supplies only RuntimeHost.
    Theater controls and shared maps are built in C++.
*/

#include "CmdTheaterDlg.h"

// Shared maps:
#include "GalaxyMapPanel.h"
#include "SystemMapPanel.h"
#include "SectorMapPanel.h"

// Runtime environment:
#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystem.h"

// UMG:
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

// Starshatter:
#include "Starshatter.h"
#include "Campaign.h"
#include "MissionElement.h"
#include "Mouse.h"

// Campaign UI:
#include "CmpnScreen.h"
#include "CmdDlg.h"

// UI style:
#include "MissionUIStyle.h"

UCmdTheaterDlg::UCmdTheaterDlg(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdTheaterDlg::NativeConstruct()
{
    Super::NativeConstruct();

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    if (!RuntimeHost)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmdTheaterDlg] RuntimeHost is not present yet. ")
            TEXT("This is expected during Widget Blueprint migration."));
        return;
    }

    BuildRuntimeLayout();
    BuildMapPanels();

    if (GalaxyButton)
    {
        GalaxyButton->OnClicked.RemoveAll(this);
        GalaxyButton->OnClicked.AddDynamic(
            this,
            &UCmdTheaterDlg::OnViewGalaxyClicked);
    }

    if (SystemButton)
    {
        SystemButton->OnClicked.RemoveAll(this);
        SystemButton->OnClicked.AddDynamic(
            this,
            &UCmdTheaterDlg::OnViewSystemClicked);
    }

    if (SectorButton)
    {
        SectorButton->OnClicked.RemoveAll(this);
        SectorButton->OnClicked.AddDynamic(
            this,
            &UCmdTheaterDlg::OnViewSectorClicked);
    }

    if (ZoomInButton)
    {
        ZoomInButton->OnClicked.RemoveAll(this);
        ZoomInButton->OnClicked.AddDynamic(
            this,
            &UCmdTheaterDlg::OnZoomInClicked);
    }

    if (ZoomOutButton)
    {
        ZoomOutButton->OnClicked.RemoveAll(this);
        ZoomOutButton->OnClicked.AddDynamic(
            this,
            &UCmdTheaterDlg::OnZoomOutClicked);
    }

    EnsureDefaultSystemSelection();
    SyncMapContext();
    SetViewMode(VIEW_GALAXY);
}

void UCmdTheaterDlg::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

FReply UCmdTheaterDlg::NativeOnMouseWheel(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    const float WheelDelta =
        InMouseEvent.GetWheelDelta();

    if (WheelDelta > 0.0f)
    {
        OnZoomInClicked();
        return FReply::Handled();
    }

    if (WheelDelta < 0.0f)
    {
        OnZoomOutClicked();
        return FReply::Handled();
    }

    return Super::NativeOnMouseWheel(
        InGeometry,
        InMouseEvent);
}

void UCmdTheaterDlg::SetManager(
    UCmpnScreen* InManager)
{
    Manager = InManager;
}

void UCmdTheaterDlg::SetParentCmdDlg(
    UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
}

UButton* UCmdTheaterDlg::CreateRuntimeButton(
    const FString& Label,
    UHorizontalBox* ParentBox,
    float Width)
{
    if (!WidgetTree || !ParentBox)
    {
        return nullptr;
    }

    const FString SafeName =
        Label.Replace(TEXT(" "), TEXT("_"));

    UButton* Button =
        WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(),
            *FString::Printf(
                TEXT("CmdTheater_%s_Button"),
                *SafeName));

    if (!Button)
    {
        return nullptr;
    }

    USizeBox* ButtonSize =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            *FString::Printf(
                TEXT("CmdTheater_%s_Size"),
                *SafeName));

    if (!ButtonSize)
    {
        return nullptr;
    }

    ButtonSize->SetWidthOverride(Width);
    ButtonSize->SetHeightOverride(34.0f);

    UTextBlock* ButtonText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            *FString::Printf(
                TEXT("CmdTheater_%s_Text"),
                *SafeName));

    if (!ButtonText)
    {
        return nullptr;
    }

    ButtonText->SetText(
        FText::FromString(Label));

    ButtonText->SetJustification(
        ETextJustify::Center);

    ButtonText->SetColorAndOpacity(
        MissionUIStyle::HeaderText);

    ButtonText->SetFont(
        MissionUIStyle::GetHeaderFont(14));

    Button->AddChild(ButtonText);
    ButtonSize->SetContent(Button);

    if (UHorizontalBoxSlot* CSlot =
        ParentBox->AddChildToHorizontalBox(ButtonSize))
    {
        CSlot->SetPadding(
            FMargin(0.0f, 0.0f, 8.0f, 0.0f));

        CSlot->SetHorizontalAlignment(
            HAlign_Left);

        CSlot->SetVerticalAlignment(
            VAlign_Center);

        CSlot->SetSize(
            FSlateChildSize(
                ESlateSizeRule::Automatic));
    }

    return Button;
}

void UCmdTheaterDlg::BuildRuntimeLayout()
{
    if (!WidgetTree || !RuntimeHost)
    {
        return;
    }

    if (bRuntimeLayoutBuilt)
    {
        return;
    }

    RuntimeHost->SetContent(nullptr);
    RuntimeHost->SetClipping(
        EWidgetClipping::ClipToBounds);

    RootColumn =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("CmdTheaterRootColumn"));

    if (!RootColumn)
    {
        return;
    }

    RuntimeHost->SetContent(RootColumn);

    // ------------------------------------------------------------
    // Top controls
    // ------------------------------------------------------------

    TopButtonRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("CmdTheaterTopButtonRow"));

    if (UVerticalBoxSlot* TopSlot =
        RootColumn->AddChildToVerticalBox(
            TopButtonRow))
    {
        TopSlot->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, 8.0f));

        TopSlot->SetSize(
            FSlateChildSize(
                ESlateSizeRule::Automatic));

        TopSlot->SetHorizontalAlignment(
            HAlign_Fill);

        TopSlot->SetVerticalAlignment(
            VAlign_Top);
    }

    ViewButtonBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("CmdTheaterViewButtonBox"));

    if (UHorizontalBoxSlot* ViewSlot =
        TopButtonRow->AddChildToHorizontalBox(
            ViewButtonBox))
    {
        ViewSlot->SetSize(
            FSlateChildSize(
                ESlateSizeRule::Automatic));
    }

    GalaxyButton =
        CreateRuntimeButton(
            TEXT("GALAXY"),
            ViewButtonBox,
            132.0f);

    SystemButton =
        CreateRuntimeButton(
            TEXT("SYSTEM"),
            ViewButtonBox,
            132.0f);

    SectorButton =
        CreateRuntimeButton(
            TEXT("SECTOR"),
            ViewButtonBox,
            132.0f);

    USpacer* ControlSpacer =
        WidgetTree->ConstructWidget<USpacer>(
            USpacer::StaticClass(),
            TEXT("CmdTheaterControlSpacer"));

    if (UHorizontalBoxSlot* SpacerSlot =
        TopButtonRow->AddChildToHorizontalBox(
            ControlSpacer))
    {
        SpacerSlot->SetSize(
            FSlateChildSize(
                ESlateSizeRule::Fill));
    }

    ZoomButtonBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("CmdTheaterZoomButtonBox"));

    if (UHorizontalBoxSlot* ZoomSlot =
        TopButtonRow->AddChildToHorizontalBox(
            ZoomButtonBox))
    {
        ZoomSlot->SetSize(
            FSlateChildSize(
                ESlateSizeRule::Automatic));
    }

    ZoomOutButton =
        CreateRuntimeButton(
            TEXT("-"),
            ZoomButtonBox,
            44.0f);

    ZoomInButton =
        CreateRuntimeButton(
            TEXT("+"),
            ZoomButtonBox,
            44.0f);

    // ------------------------------------------------------------
    // Map area
    // ------------------------------------------------------------

    MainViewHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("CmdTheaterMainViewHost"));

    MainViewHost->SetClipping(
        EWidgetClipping::ClipToBounds);

    if (UVerticalBoxSlot* MainSlot =
        RootColumn->AddChildToVerticalBox(
            MainViewHost))
    {
        MainSlot->SetSize(
            FSlateChildSize(
                ESlateSizeRule::Fill));

        MainSlot->SetHorizontalAlignment(
            HAlign_Fill);

        MainSlot->SetVerticalAlignment(
            VAlign_Fill);
    }

    RuntimeMapSwitcher =
        WidgetTree->ConstructWidget<UWidgetSwitcher>(
            UWidgetSwitcher::StaticClass(),
            TEXT("CmdTheaterRuntimeMapSwitcher"));

    if (!RuntimeMapSwitcher)
    {
        return;
    }

    RuntimeMapSwitcher->SetClipping(
        EWidgetClipping::ClipToBounds);

    MainViewHost->SetContent(RuntimeMapSwitcher);

    GalaxyPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("CmdTheaterGalaxyPanelHost"));

    SystemPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("CmdTheaterSystemPanelHost"));

    SectorPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("CmdTheaterSectorPanelHost"));

    if (!GalaxyPanelHost ||
        !SystemPanelHost ||
        !SectorPanelHost)
    {
        return;
    }

    GalaxyPanelHost->SetClipping(
        EWidgetClipping::ClipToBounds);

    SystemPanelHost->SetClipping(
        EWidgetClipping::ClipToBounds);

    SectorPanelHost->SetClipping(
        EWidgetClipping::ClipToBounds);

    RuntimeMapSwitcher->AddChild(GalaxyPanelHost);
    RuntimeMapSwitcher->AddChild(SystemPanelHost);
    RuntimeMapSwitcher->AddChild(SectorPanelHost);

    bRuntimeLayoutBuilt = true;

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Runtime layout built"));
}

void UCmdTheaterDlg::BuildMapPanels()
{
    if (bMapsBuilt)
    {
        return;
    }

    if (!RuntimeMapSwitcher ||
        !GalaxyPanelHost ||
        !SystemPanelHost ||
        !SectorPanelHost)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmdTheaterDlg] BuildMapPanels: ")
            TEXT("runtime layout is incomplete"));
        return;
    }

    // ------------------------------------------------------------
    // GALAXY
    // ------------------------------------------------------------

    if (!GalaxyMapPanelClass)
    {
        GalaxyMapPanelClass =
            LoadClass<UGalaxyMapPanel>(
                nullptr,
                TEXT("/Game/Screens/Mission/")
                TEXT("WBP_GalaxyMapPanel.")
                TEXT("WBP_GalaxyMapPanel_C"));
    }

    if (!GalaxyMapPanelClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmdTheaterDlg] Failed to load ")
            TEXT("WBP_GalaxyMapPanel"));
    }
    else
    {
        GalaxyMapPanel =
            CreateWidget<UGalaxyMapPanel>(
                GetWorld(),
                GalaxyMapPanelClass);

        if (!GalaxyMapPanel)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[CmdTheaterDlg] Failed to create ")
                TEXT("GalaxyMapPanel"));
        }
        else
        {
            GalaxyMapPanel->OnSystemSelected.BindUObject(
                this,
                &UCmdTheaterDlg::HandleGalaxySystemSelected);

            GalaxyMapPanel->OnSystemActivated.BindUObject(
                this,
                &UCmdTheaterDlg::HandleGalaxySystemActivated);

            if (UStarshatterEnvironmentSubsystem* Env =
                GetEnvironmentSubsystem())
            {
                const TArray<StarSystem*>& RuntimeSystems =
                    Env->GetRuntimeStarSystems();

                UE_LOG(LogTemp, Warning,
                    TEXT("[CmdTheaterDlg] Galaxy runtime systems=%d"),
                    RuntimeSystems.Num());

                GalaxyMapPanel->LoadFromRuntimeSystems(
                    RuntimeSystems);
            }

            GalaxyMapPanel->SetCurrentMissionSystem(
                TEXT(""));

            TArray<FString> EmptyRoute;
            GalaxyMapPanel->SetRoutePath(
                EmptyRoute);

            GalaxyPanelHost->SetContent(
                GalaxyMapPanel);

            UE_LOG(LogTemp, Warning,
                TEXT("[CmdTheaterDlg] Created GalaxyMapPanel from %s"),
                *GetNameSafe(GalaxyMapPanelClass));
        }
    }

    // ------------------------------------------------------------
    // SYSTEM
    // ------------------------------------------------------------

    if (!SystemMapPanelClass)
    {
        SystemMapPanelClass =
            USystemMapPanel::StaticClass();
    }

    SystemMapPanel =
        CreateWidget<USystemMapPanel>(
            GetWorld(),
            SystemMapPanelClass);

    if (SystemMapPanel)
    {
        SystemMapPanel->SetNavigationOwner(this);
        SystemPanelHost->SetContent(
            SystemMapPanel);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmdTheaterDlg] Failed to create ")
            TEXT("SystemMapPanel"));
    }

    // ------------------------------------------------------------
    // SECTOR
    // ------------------------------------------------------------

    if (!SectorMapPanelClass)
    {
        SectorMapPanelClass =
            USectorMapPanel::StaticClass();
    }

    SectorMapPanel =
        CreateWidget<USectorMapPanel>(
            GetWorld(),
            SectorMapPanelClass);

    if (SectorMapPanel)
    {
        SectorMapPanel->SetOperationsView(true);

        SectorMapPanel->OnElementSelected.BindUObject(
            this,
            &UCmdTheaterDlg::HandleSectorElementSelected);

        // Major-structure campaign/runtime data will be connected separately.
        SectorMapPanel->SetMission(nullptr);

        SectorPanelHost->SetContent(
            SectorMapPanel);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmdTheaterDlg] Failed to create ")
            TEXT("SectorMapPanel"));
    }

    RuntimeMapSwitcher->SetActiveWidgetIndex(
        static_cast<int32>(VIEW_GALAXY));

    bMapsBuilt = true;

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Map panels built: ")
        TEXT("Galaxy=%p System=%p Sector=%p"),
        GalaxyMapPanel,
        SystemMapPanel,
        SectorMapPanel);
}

void UCmdTheaterDlg::ShowTheaterDlg()
{
    Mode = 1;

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    if (!bRuntimeLayoutBuilt)
    {
        BuildRuntimeLayout();
    }

    if (!bMapsBuilt)
    {
        BuildMapPanels();
    }

    if (GalaxyMapPanel)
    {
        if (UStarshatterEnvironmentSubsystem* Env =
            GetEnvironmentSubsystem())
        {
            const TArray<StarSystem*>& RuntimeSystems =
                Env->GetRuntimeStarSystems();

            UE_LOG(LogTemp, Warning,
                TEXT("[CmdTheaterDlg] Show: Galaxy runtime systems=%d"),
                RuntimeSystems.Num());

            GalaxyMapPanel->LoadFromRuntimeSystems(
                RuntimeSystems);
        }

        GalaxyMapPanel->SetCurrentMissionSystem(
            TEXT(""));

        TArray<FString> EmptyRoute;
        GalaxyMapPanel->SetRoutePath(
            EmptyRoute);
    }

    EnsureDefaultSystemSelection();
    SyncMapContext();

    SetViewMode(VIEW_GALAXY);

    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);
    SetIsFocusable(true);

    if (GalaxyMapPanel)
    {
        GalaxyMapPanel->SetFocus();
    }

    Mouse::Show(true);
}

void UCmdTheaterDlg::ExecFrame()
{
    if (!Stars)
    {
        Stars = Starshatter::GetInstance();
    }

    if (!CampaignPtr)
    {
        CampaignPtr = Campaign::GetCampaign();
    }
}

UStarshatterEnvironmentSubsystem*
UCmdTheaterDlg::GetEnvironmentSubsystem() const
{
    UGameInstance* GI =
        GetGameInstance();

    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<
        UStarshatterEnvironmentSubsystem>();
}

StarSystem* UCmdTheaterDlg::FindRuntimeSystemByName(
    const FString& InSystemName) const
{
    if (InSystemName.IsEmpty())
    {
        return nullptr;
    }

    UStarshatterEnvironmentSubsystem* Env =
        GetEnvironmentSubsystem();

    if (!Env)
    {
        return nullptr;
    }

    for (StarSystem* System :
        Env->GetRuntimeStarSystems())
    {
        if (!System)
        {
            continue;
        }

        if (InSystemName.Equals(
            ANSI_TO_TCHAR(System->GetName()),
            ESearchCase::IgnoreCase))
        {
            return System;
        }
    }

    return nullptr;
}

void UCmdTheaterDlg::EnsureDefaultSystemSelection()
{
    if (!SelectedSystemName.IsEmpty() &&
        FindRuntimeSystemByName(
            SelectedSystemName))
    {
        return;
    }

    SelectedSystemName.Empty();
    SelectedSectorName.Empty();
    SelectedStructureElement = nullptr;

    UStarshatterEnvironmentSubsystem* Env =
        GetEnvironmentSubsystem();

    if (!Env)
    {
        return;
    }

    for (StarSystem* System :
        Env->GetRuntimeStarSystems())
    {
        if (!System)
        {
            continue;
        }

        SelectedSystemName =
            ANSI_TO_TCHAR(System->GetName());

        break;
    }
}

void UCmdTheaterDlg::SyncMapContext()
{
    EnsureDefaultSystemSelection();

    if (SelectedSystemName.IsEmpty())
    {
        return;
    }

    if (GalaxyMapPanel)
    {
        GalaxyMapPanel->SetSelectedSystem(
            SelectedSystemName);
    }

    if (SystemMapPanel)
    {
        SystemMapPanel->SetViewedSystemName(
            SelectedSystemName);
    }

    if (SectorMapPanel)
    {
        SectorMapPanel->SetViewedSystemName(
            SelectedSystemName);

        if (!SelectedSectorName.IsEmpty())
        {
            SectorMapPanel->SetViewedSectorName(
                SelectedSectorName);
        }

        SelectedSectorName =
            SectorMapPanel->GetViewedSectorName();
    }
}

void UCmdTheaterDlg::SetViewMode(
    EViewMode NewMode)
{
    CurrentViewMode = NewMode;

    switch (CurrentViewMode)
    {
    case VIEW_GALAXY:
        CurrentSelectionMode =
            SELECT_SYSTEM;
        break;

    case VIEW_SYSTEM:
        CurrentSelectionMode =
            SELECT_PLANET;
        break;

    case VIEW_REGION:
        CurrentSelectionMode =
            SELECT_STATION;
        break;

    default:
        CurrentSelectionMode =
            SELECT_NONE;
        break;
    }

    SyncMapContext();

    if (RuntimeMapSwitcher)
    {
        RuntimeMapSwitcher->SetActiveWidgetIndex(
            static_cast<int32>(
                CurrentViewMode));
    }

    if (CurrentViewMode == VIEW_GALAXY &&
        GalaxyMapPanel)
    {
        GalaxyMapPanel->SetFocus();
    }
    else if (CurrentViewMode == VIEW_SYSTEM &&
        SystemMapPanel)
    {
        SystemMapPanel->SetFocus();
    }
    else if (CurrentViewMode == VIEW_REGION &&
        SectorMapPanel)
    {
        SectorMapPanel->SetFocus();
    }

    RefreshViewButtons();
}

void UCmdTheaterDlg::RefreshViewButtons()
{
    if (GalaxyButton)
    {
        GalaxyButton->SetIsEnabled(
            CurrentViewMode != VIEW_GALAXY);
    }

    if (SystemButton)
    {
        SystemButton->SetIsEnabled(
            CurrentViewMode != VIEW_SYSTEM);
    }

    if (SectorButton)
    {
        SectorButton->SetIsEnabled(
            CurrentViewMode != VIEW_REGION);
    }
}

void UCmdTheaterDlg::HandleGalaxySystemSelected(
    const FString& InSystemName)
{
    const FString CleanName =
        InSystemName.TrimStartAndEnd();

    if (CleanName.IsEmpty())
    {
        return;
    }

    const bool bChanged =
        !SelectedSystemName.Equals(
            CleanName,
            ESearchCase::IgnoreCase);

    SelectedSystemName =
        CleanName;

    if (bChanged)
    {
        SelectedSectorName.Empty();
        SelectedStructureElement = nullptr;
    }

    SyncMapContext();

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Galaxy system selected: %s"),
        *SelectedSystemName);
}

void UCmdTheaterDlg::HandleGalaxySystemActivated(
    const FString& InSystemName)
{
    HandleGalaxySystemSelected(
        InSystemName);

    if (SelectedSystemName.IsEmpty())
    {
        return;
    }

    if (SystemMapPanel)
    {
        SystemMapPanel->SetViewedSystemName(
            SelectedSystemName);

        SystemMapPanel->SetSelectedBodyName(
            TEXT(""));

        SystemMapPanel->ShowSystemOverview();
    }

    SetViewMode(
        VIEW_SYSTEM);
}

void UCmdTheaterDlg::HandleSectorElementSelected(
    MissionElement* InElement)
{
    SelectedStructureElement =
        InElement;

    if (!InElement)
    {
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Operations structure selected: %s"),
        ANSI_TO_TCHAR(
            InElement->GetName()));
}

void UCmdTheaterDlg::OnViewGalaxyClicked()
{
    SetViewMode(
        VIEW_GALAXY);
}

void UCmdTheaterDlg::OnViewSystemClicked()
{
    EnsureDefaultSystemSelection();

    if (SystemMapPanel &&
        !SelectedSystemName.IsEmpty())
    {
        SystemMapPanel->SetViewedSystemName(
            SelectedSystemName);
    }

    SetViewMode(
        VIEW_SYSTEM);
}

void UCmdTheaterDlg::OnViewSectorClicked()
{
    EnsureDefaultSystemSelection();

    SetViewMode(
        VIEW_REGION);
}

void UCmdTheaterDlg::OnZoomInClicked()
{
    if (CurrentViewMode == VIEW_GALAXY &&
        GalaxyMapPanel)
    {
        GalaxyMapPanel->ZoomIn();
    }
    else if (CurrentViewMode == VIEW_SYSTEM &&
        SystemMapPanel)
    {
        SystemMapPanel->ZoomIn();
    }
    else if (CurrentViewMode == VIEW_REGION &&
        SectorMapPanel)
    {
        SectorMapPanel->ZoomIn();
    }
}

void UCmdTheaterDlg::OnZoomOutClicked()
{
    if (CurrentViewMode == VIEW_GALAXY &&
        GalaxyMapPanel)
    {
        GalaxyMapPanel->ZoomOut();
    }
    else if (CurrentViewMode == VIEW_SYSTEM &&
        SystemMapPanel)
    {
        SystemMapPanel->ZoomOut();
    }
    else if (CurrentViewMode == VIEW_REGION &&
        SectorMapPanel)
    {
        SectorMapPanel->ZoomOut();
    }
}
