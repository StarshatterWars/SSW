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

    CmdTheaterPanel supplies RuntimeHost and Border_0.
    Theater controls and shared maps are built in C++.
*/

#include "CmdTheaterDlg.h"
#include "CentralSun.h"
#include "PlanetActor.h"
#include "OrbitalBody.h"
#include "Engine/Texture2D.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

// Shared maps:
#include "GalaxyMapPanel.h"
#include "SystemMapPanel.h"
#include "SectorMapPanel.h"

// Runtime environment:
#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "CombatGroupRegistry.h"
#include "GameStructs.h"

// UMG:
#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "MenuButton.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/ComboBoxString.h"
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

namespace
{
    void ApplyTheaterDropdownListStyle(UComboBoxString* Combo)
    {
        if (!Combo) return;
        FTableRowStyle Rows = Combo->GetItemStyle();
        FSlateBrush Gray;
        Gray.DrawAs = ESlateBrushDrawType::Box;
        Gray.TintColor = FSlateColor(FLinearColor(0.60f, 0.60f, 0.60f, 1.0f));
        Rows.EvenRowBackgroundBrush = Gray;
        Rows.OddRowBackgroundBrush = Gray;
        Rows.EvenRowBackgroundHoveredBrush = Gray;
        Rows.OddRowBackgroundHoveredBrush = Gray;
        Rows.TextColor = FSlateColor(FLinearColor::Black);
        // Preserve selected text and active/inactive selection brushes.
        Combo->SetItemStyle(Rows);

        FComboBoxStyle Style = Combo->GetWidgetStyle();
        Style.ComboButtonStyle.MenuBorderBrush = Gray;
        Combo->SetWidgetStyle(Style);
    }
}
UCmdTheaterDlg::UCmdTheaterDlg(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<UMenuButton> ButtonClass(
        TEXT("/Game/Screens/Operations/WB_MenuButton"));
    if (ButtonClass.Succeeded())
    {
        TheaterMenuButtonClass = ButtonClass.Class;
    }
}

void UCmdTheaterDlg::NativeConstruct()
{
    Super::NativeConstruct();

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    ensureMsgf(
        RuntimeHost,
        TEXT("CmdTheaterDlg: RuntimeHost is not bound"));

    BuildRuntimeLayout();
    BuildMapPanels();




    if (SystemComboBox)
    {
        SystemComboBox->OnSelectionChanged.RemoveAll(this);
        SystemComboBox->OnSelectionChanged.AddDynamic(
            this,
            &UCmdTheaterDlg::OnSystemSelectionChanged);
    }

    if (RegionComboBox)
    {
        RegionComboBox->OnSelectionChanged.RemoveAll(this);
        RegionComboBox->OnSelectionChanged.AddDynamic(
            this,
            &UCmdTheaterDlg::OnRegionSelectionChanged);
    }



    EnsureDefaultSystemSelection();
    SyncMapContext();

    EnsureCentralSun();

    SetViewMode(VIEW_GALAXY);
}

void UCmdTheaterDlg::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
    UpdateSystemSunCamera();
    UpdateSystemPlanets();
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

UMenuButton* UCmdTheaterDlg::CreateRuntimeButton(
    const FString& Label, UHorizontalBox* ParentBox, float Width)
{
    if (!ParentBox || !TheaterMenuButtonClass)
    {
        return nullptr;
    }

    UMenuButton* Button = CreateWidget<UMenuButton>(this, TheaterMenuButtonClass);
    if (!Button) return nullptr;

    Button->MenuOption = Label;
    Button->WidthOverride = Width;
    Button->HeightOverride = 34.0f;
    Button->LabelFontSize = 14;
    if (UTextBlock* Text = Cast<UTextBlock>(Button->GetWidgetFromName(TEXT("Label"))))
    {
        Text->SetText(FText::FromString(Label));
    }

    Button->OnSelected.AddDynamic(this, &UCmdTheaterDlg::HandleTheaterButtonSelected);
    if (UHorizontalBoxSlot* CSlot = ParentBox->AddChildToHorizontalBox(Button))
    {
        CSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        CSlot->SetHorizontalAlignment(HAlign_Left);
        CSlot->SetVerticalAlignment(VAlign_Center);
        CSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }
    return Button;
}

void UCmdTheaterDlg::HandleTheaterButtonSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton) return;
    if (SelectedButton == GalaxyButton) OnViewGalaxyClicked();
    else if (SelectedButton == SystemButton) OnViewSystemClicked();
    else if (SelectedButton == SectorButton) OnViewSectorClicked();
    else if (SelectedButton == ZoomInButton) OnZoomInClicked();
    else if (SelectedButton == ZoomOutButton) OnZoomOutClicked();
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
            FMargin(32.0f, 0.0f, 0.0f, 8.0f));

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

    // Return buttons. Visibility is view-dependent:
    // - Galaxy view: neither return button is shown.
    // - System view: GALAXY is shown.
    // - Sector view: GALAXY and SYSTEM are shown.
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

    // There is deliberately no ordinary SECTOR button.
    // On System view, sector/region navigation is provided by RegionComboBox.
    SectorButton = nullptr;

    // ------------------------------------------------------------
    // System selector (Galaxy view only)
    // ------------------------------------------------------------

    SystemSelectorBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("CmdTheaterSystemSelectorBox"));

    if (SystemSelectorBox)
    {
        if (UHorizontalBoxSlot* SystemBoxSlot =
            TopButtonRow->AddChildToHorizontalBox(
                SystemSelectorBox))
        {
            SystemBoxSlot->SetPadding(
                FMargin(0.0f, 0.0f, 8.0f, 0.0f));

            SystemBoxSlot->SetSize(
                FSlateChildSize(
                    ESlateSizeRule::Automatic));

            SystemBoxSlot->SetVerticalAlignment(
                VAlign_Center);
        }

        SystemSelectorLabel =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("CmdTheaterSystemLabel"));

        if (SystemSelectorLabel)
        {
            SystemSelectorLabel->SetText(
                FText::FromString(TEXT("SYSTEM")));

            SystemSelectorLabel->SetColorAndOpacity(
                MissionUIStyle::HeaderText);

            SystemSelectorLabel->SetFont(
                MissionUIStyle::GetHeaderFont(12));

            if (UHorizontalBoxSlot* LabelSlot =
                SystemSelectorBox->AddChildToHorizontalBox(
                    SystemSelectorLabel))
            {
                LabelSlot->SetPadding(
                    FMargin(0.0f, 0.0f, 8.0f, 0.0f));

                LabelSlot->SetVerticalAlignment(
                    VAlign_Center);

                LabelSlot->SetSize(
                    FSlateChildSize(
                        ESlateSizeRule::Automatic));
            }
        }

        SystemComboHost =
            WidgetTree->ConstructWidget<USizeBox>(
                USizeBox::StaticClass(),
                TEXT("CmdTheaterSystemComboHost"));

        if (SystemComboHost)
        {
            SystemComboHost->SetWidthOverride(260.0f);
            SystemComboHost->SetHeightOverride(34.0f);

            if (UHorizontalBoxSlot* ComboHostSlot =
                SystemSelectorBox->AddChildToHorizontalBox(
                    SystemComboHost))
            {
                ComboHostSlot->SetVerticalAlignment(
                    VAlign_Center);

                ComboHostSlot->SetSize(
                    FSlateChildSize(
                        ESlateSizeRule::Automatic));
            }

            SystemComboBox =
                WidgetTree->ConstructWidget<UComboBoxString>(
                    UComboBoxString::StaticClass(),
                    TEXT("CmdTheaterSystemComboBox"));

            if (SystemComboBox)
            {
                ApplyTheaterDropdownListStyle(SystemComboBox);
                SystemComboHost->SetContent(
                    SystemComboBox);
            }
        }

        SystemSelectorBox->SetVisibility(
            ESlateVisibility::Collapsed);
    }

    // ------------------------------------------------------------
    // Sector selector (System view only; sectors are OrbitalRegions)
    // ------------------------------------------------------------

    RegionSelectorBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("CmdTheaterRegionSelectorBox"));

    if (RegionSelectorBox)
    {
        if (UHorizontalBoxSlot* RegionBoxSlot =
            TopButtonRow->AddChildToHorizontalBox(
                RegionSelectorBox))
        {
            RegionBoxSlot->SetPadding(
                FMargin(12.0f, 0.0f, 8.0f, 0.0f));

            RegionBoxSlot->SetSize(
                FSlateChildSize(
                    ESlateSizeRule::Automatic));

            RegionBoxSlot->SetVerticalAlignment(
                VAlign_Center);
        }

        RegionSelectorLabel =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("CmdTheaterRegionLabel"));

        if (RegionSelectorLabel)
        {
            RegionSelectorLabel->SetText(
                FText::FromString(TEXT("SECTOR")));

            RegionSelectorLabel->SetColorAndOpacity(
                MissionUIStyle::HeaderText);

            RegionSelectorLabel->SetFont(
                MissionUIStyle::GetHeaderFont(12));

            if (UHorizontalBoxSlot* LabelSlot =
                RegionSelectorBox->AddChildToHorizontalBox(
                    RegionSelectorLabel))
            {
                LabelSlot->SetPadding(
                    FMargin(0.0f, 0.0f, 8.0f, 0.0f));

                LabelSlot->SetVerticalAlignment(
                    VAlign_Center);

                LabelSlot->SetSize(
                    FSlateChildSize(
                        ESlateSizeRule::Automatic));
            }
        }

        RegionComboHost =
            WidgetTree->ConstructWidget<USizeBox>(
                USizeBox::StaticClass(),
                TEXT("CmdTheaterRegionComboHost"));

        if (RegionComboHost)
        {
            RegionComboHost->SetWidthOverride(260.0f);
            RegionComboHost->SetHeightOverride(34.0f);

            if (UHorizontalBoxSlot* ComboHostSlot =
                RegionSelectorBox->AddChildToHorizontalBox(
                    RegionComboHost))
            {
                ComboHostSlot->SetVerticalAlignment(
                    VAlign_Center);

                ComboHostSlot->SetSize(
                    FSlateChildSize(
                        ESlateSizeRule::Automatic));
            }

            RegionComboBox =
                WidgetTree->ConstructWidget<UComboBoxString>(
                    UComboBoxString::StaticClass(),
                    TEXT("CmdTheaterRegionComboBox"));

            if (RegionComboBox)
            {
                ApplyTheaterDropdownListStyle(RegionComboBox);
                RegionComboHost->SetContent(
                    RegionComboBox);
            }
        }

        RegionSelectorBox->SetVisibility(
            ESlateVisibility::Collapsed);
    }

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

        // Operations System view uses the real 3D ACentralSun.
        // Keep orbit layout and star hit-testing, but do not paint
        // the old 2D primary-star texture.
        SystemMapPanel->SetDrawPrimaryStar2D(false);

        SystemMapPanel->OnPrimaryStarActivated.BindUObject(
            this,
            &UCmdTheaterDlg::HandleSystemPrimaryStarActivated);

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

        SectorMapPanel->OnOperationsGroupSelected.BindUObject(
            this,
            &UCmdTheaterDlg::HandleOperationsGroupSelected);

        // Operations is static/persistent-data driven; no Mission* is required.
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


void UCmdTheaterDlg::NativeDestruct()
{
    ClearSystemPlanets();
    RestoreSystemSunCamera();

    if (IsValid(SystemSunCamera))
    {
        SystemSunCamera->Destroy();
        SystemSunCamera = nullptr;
    }

    if (IsValid(CentralSun))
    {
        CentralSun->Destroy();
        CentralSun = nullptr;
    }

    Super::NativeDestruct();
}

void UCmdTheaterDlg::RestoreSystemSunCamera()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && IsValid(SystemSunCamera) &&
        PC->GetViewTarget() == SystemSunCamera.Get())
    {
        AActor* RestoreTarget = PreviousSunViewTarget.Get();
        PC->SetViewTarget(RestoreTarget ? RestoreTarget : PC);
    }

    PreviousSunViewTarget.Reset();
}

void UCmdTheaterDlg::UpdateSystemSunCamera()
{
    if (CurrentViewMode != VIEW_SYSTEM || !IsVisible())
    {
        RestoreSystemSunCamera();
        if (IsValid(CentralSun))
        {
            CentralSun->HideSun();
        }
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->PlayerCameraManager || !IsValid(CentralSun))
    {
        return;
    }

    AActor* SunActor = CentralSun->GetSunActor();
    if (!IsValid(SunActor))
    {
        return;
    }

    // BP_Star creates additional flare meshes. Their size and bounds center
    // must not determine the camera target: use its named photosphere.
    if (!bSunPresentationReady)
    {
        CentralSun->HideSun();
        if (!SystemMapPanel) return;
        const FGeometry& Layout = SystemMapPanel->GetCachedGeometry();
        const FVector2D Size = Layout.GetLocalSize();
        const FVector2D Origin = Layout.LocalToAbsolute(FVector2D::ZeroVector);
        if (Size.X <= 1.0f || Size.Y <= 1.0f)
        {
            SunLayoutStableFrames = 0;
            return;
        }
        const bool bStable = Size.Equals(LastSunPanelSize, 0.1f) &&
            Origin.Equals(LastSunPanelOrigin, 0.1f);
        SunLayoutStableFrames = bStable ? SunLayoutStableFrames + 1 : 0;
        LastSunPanelSize = Size;
        LastSunPanelOrigin = Origin;
    }
    TArray<UStaticMeshComponent*> Meshes;
    SunActor->GetComponents<UStaticMeshComponent>(Meshes);
    UStaticMeshComponent* StarMesh = nullptr;
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        if (IsValid(Mesh) && Mesh->GetFName() == FName(TEXT("SM_Star")))
        {
            StarMesh = Mesh;
            break;
        }
    }

    if (!StarMesh || !StarMesh->GetStaticMesh())
    {
        // Do not silently aim at an unrelated effect if the Blueprint changes.
        return;
    }

    const FBoxSphereBounds StarBounds =
        StarMesh->CalcBounds(StarMesh->GetComponentTransform());
    const FVector StarCenter = StarBounds.Origin;
    const double Radius = StarBounds.BoxExtent.GetMax();

    int32 Width = 0;
    int32 Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Radius <= KINDA_SMALL_NUMBER || Width <= 0 || Height <= 0)
    {
        return;
    }

    if (!IsValid(SystemSunCamera))
    {
        SystemSunCamera = GetWorld()->SpawnActor<ACameraActor>();
        if (!SystemSunCamera)
        {
            return;
        }
        SystemSunCamera->SetActorRotation(
            PC->PlayerCameraManager->GetCameraRotation());
        SystemSunCamera->GetCameraComponent()->SetFieldOfView(60.0f);
        SystemSunCamera->GetCameraComponent()->bConstrainAspectRatio = false;
    }

    UCameraComponent* Camera = SystemSunCamera->GetCameraComponent();
    // Keep horizontal FOV fixed so viewport width gives a stable pixel scale.
    Camera->bOverrideAspectRatioAxisConstraint = true;
    Camera->AspectRatioAxisConstraint = AspectRatio_MaintainXFOV;

    const double HalfFOV = FMath::DegreesToRadians(Camera->FieldOfView * 0.5);
    const double FocalPixels = Width / (2.0 * FMath::Tan(HalfFOV));
    // SunDiameterPixels is the base diameter at 1x map zoom.
    // Use the current interpolated zoom so focus animations also resize the star.
    const double MapZoom = SystemMapPanel ? SystemMapPanel->GetMapZoomScale() : 1.0;
    // Keep the star at least 64 physical pixels across when zooming out.
    // This also caps camera distance at the stable minimum-size view.
    const double DiameterPixels = FMath::Max(64.0, SunDiameterPixels * MapZoom);
    const double PixelRadius = DiameterPixels * 0.5;
    // Exact perspective silhouette distance for a sphere centered in the view.
    const double Ratio = FocalPixels / PixelRadius;
    const double Distance = Radius * FMath::Sqrt(1.0 + Ratio * Ratio);

    FVector CameraLocation =
        StarCenter - SystemSunCamera->GetActorForwardVector() * Distance;

    // Match the actual Slate orbit center, including layout, DPI and pan.
    if (SystemMapPanel)
    {
        const FGeometry& Geometry = SystemMapPanel->GetCachedGeometry();
        if (Geometry.GetLocalSize().X > 0.0f && Geometry.GetLocalSize().Y > 0.0f)
        {
            FVector2D PixelPosition;
            FVector2D ViewportPosition;
            USlateBlueprintLibrary::LocalToViewport(
                this, Geometry, SystemMapPanel->GetSystemCenterLocal(),
                PixelPosition, ViewportPosition);

            const double OffsetRight =
                (PixelPosition.X - Width * 0.5) * Distance / FocalPixels;
            const double OffsetUp =
                (Height * 0.5 - PixelPosition.Y) * Distance / FocalPixels;

            CameraLocation -=
                SystemSunCamera->GetActorRightVector() * OffsetRight +
                SystemSunCamera->GetActorUpVector() * OffsetUp;
        }
    }

    SystemSunCamera->SetActorLocation(CameraLocation);

    if (PC->GetViewTarget() != SystemSunCamera.Get())
    {
        PreviousSunViewTarget = PC->GetViewTarget();
        PC->SetViewTarget(SystemSunCamera.Get());
    }
    if (!bSunPresentationReady)
    {
        // Wait until the camera manager has consumed the positioned camera,
        // and Slate has completed two stable layout passes.
        bSunPresentationReady = SunLayoutStableFrames >= 2 &&
            PC->PlayerCameraManager->GetCameraLocation().Equals(CameraLocation, 1.0f);
    }
    CentralSun->SetSunVisible(bSunPresentationReady &&
        SystemMapPanel && !SystemMapPanel->IsPlanetView());
}

void UCmdTheaterDlg::ClearSystemPlanets()
{
    for (auto& Entry : SystemPlanetActors)
    {
        if (IsValid(Entry.Value))
        {
            Entry.Value->Destroy();
        }
    }
    SystemPlanetActors.Empty();
    SystemPlanetRadii.Empty();
    PlanetActorSystemName.Empty();
    if (SystemMapPanel)
    {
        SystemMapPanel->SetPlanetsRenderedIn3D(TSet<FString>());
    }
}

void UCmdTheaterDlg::UpdateSystemPlanets()
{
    if (!SystemMapPanel || !IsValid(SystemSunCamera) ||
        CurrentViewMode != VIEW_SYSTEM || !IsVisible())
    {
        ClearSystemPlanets();
        return;
    }

    if (PlanetActorSystemName != SystemMapPanel->GetViewedSystemName())
    {
        ClearSystemPlanets();
        PlanetActorSystemName = SystemMapPanel->GetViewedSystemName();
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC || PC->GetViewTarget() != SystemSunCamera.Get())
    {
        ClearSystemPlanets();
        return;
    }
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    const FGeometry& Geometry = SystemMapPanel->GetCachedGeometry();
    if (Width <= 0 || Height <= 0 || Geometry.GetLocalSize().IsNearlyZero())
    {
        return;
    }

    const double Focal = Width / (2.0 * FMath::Tan(FMath::DegreesToRadians(
        SystemSunCamera->GetCameraComponent()->FieldOfView * 0.5)));
    // A common camera-space plane makes icon positions independent of world orbits.
    const double Depth = 10000.0;
    const FVector Forward = SystemSunCamera->GetActorForwardVector();
    const FVector Right = SystemSunCamera->GetActorRightVector();
    const FVector Up = SystemSunCamera->GetActorUpVector();
    const FVector CameraLocation = SystemSunCamera->GetActorLocation();

    auto LoadSurface = [](const char* Name) -> UTexture2D*
    {
        const FString TextureName = UTF8_TO_TCHAR(Name ? Name : "");
        if (TextureName.IsEmpty()) return nullptr;
        const FString Path = FString::Printf(
            TEXT("/Game/GameData/Galaxy/PlanetMaterials/%s.%s"),
            *TextureName, *TextureName);
        return LoadObject<UTexture2D>(nullptr, *Path);
    };

    TArray<OrbitalBody*> Bodies;
    SystemMapPanel->GetPlanetMapBodies(Bodies);

    // Include each planet's moons. The shared placement lookup already returns
    // the exact moon center and draw diameter used by Slate, including zoom.
    const int32 PlanetCount = Bodies.Num();
    for (int32 PlanetIndex = 0; PlanetIndex < PlanetCount; ++PlanetIndex)
    {
        OrbitalBody* ParentPlanet = Bodies[PlanetIndex];
        if (!ParentPlanet) continue;

        ListIter<OrbitalBody> MoonIter = ParentPlanet->Satellites();
        while (++MoonIter)
        {
            if (OrbitalBody* Moon = MoonIter.value())
            {
                Bodies.Add(Moon);
            }
        }
    }
    TSet<FString> Rendered;
    TSet<FString> CurrentBodies;
    for (OrbitalBody* Body : Bodies)
    {
        if (!Body) continue;
        const FString Name = ANSI_TO_TCHAR(Body->GetName());
        CurrentBodies.Add(Name);
        FVector2D Center;
        float Diameter = 0.0f;
        if (!SystemMapPanel->GetPlanetMapPlacement(Name, Center, Diameter))
        {
            if (AActor* Existing = SystemPlanetActors.FindRef(Name).Get())
                Existing->SetActorHiddenInGame(true);
            continue;
        }

        APlanetActor* Planet = Cast<APlanetActor>(SystemPlanetActors.FindRef(Name).Get());
        if (!IsValid(Planet))
        {
            UTexture2D* Surface = LoadSurface(Body->GetTexture());
            if (!Surface) continue;
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Planet = GetWorld()->SpawnActor<APlanetActor>(
                APlanetActor::StaticClass(), FVector::ZeroVector,
                FRotator::ZeroRotator, SpawnParams);
            if (!Planet) continue;
            Planet->SetActorHiddenInGame(true);
            Planet->SetPlanetTextures(Surface,
                LoadSurface(Body->GetGlossTexture()),
                LoadSurface(Body->GetGlowTexture()));
            Planet->SetActorEnableCollision(false);
            // Runtime tilt is stored in radians. Tilt the actor once; the
            // planet mesh then spins about its own local axis inside it.
            const float TiltDegrees = FMath::RadiansToDegrees(
                static_cast<float>(Body->GetTilt()));
            Planet->SetActorRotation(FRotator(TiltDegrees, 0.0f, 0.0f));
            // Slow presentation spin: one revolution every three minutes.
            // Respect retrograde direction without using real-time day lengths.
            Planet->SetAxialRotationDegreesPerSecond(
                Body->IsRetrograde() ? -2.0f : 2.0f);
            Planet->SetAxialRotationEnabled(true);
            Planet->SetActorTickEnabled(true);
            Planet->SetLightDirection((-Forward + Up * 0.3 - Right * 0.4).GetSafeNormal());
            SystemPlanetActors.Add(Name, Planet);
        }

        FVector2D Pixel, Unused, EdgePixel;
        USlateBlueprintLibrary::LocalToViewport(this, Geometry, Center, Pixel, Unused);
        USlateBlueprintLibrary::LocalToViewport(
            this, Geometry, Center + FVector2D(Diameter * 0.5f, 0.0f),
            EdgePixel, Unused);
        const double PixelRadius = FVector2D::Distance(Pixel, EdgePixel);
        const FVector Location = CameraLocation + Forward * Depth +
            Right * ((Pixel.X - Width * 0.5) * Depth / Focal) +
            Up * ((Height * 0.5 - Pixel.Y) * Depth / Focal);
        Planet->SetActorLocation(Location);
        const float Radius = static_cast<float>(Depth * PixelRadius /
            FMath::Sqrt(Focal * Focal + PixelRadius * PixelRadius));
        const float* PreviousRadius = SystemPlanetRadii.Find(Name);
        if (!PreviousRadius || !FMath::IsNearlyEqual(*PreviousRadius, Radius, 0.01f))
        {
            Planet->SetPlanetRadius(Radius);
            SystemPlanetRadii.Add(Name, Radius);
        }

        const FVector2D Size = Geometry.GetLocalSize();
        const bool bInside = Center.X >= Diameter * 0.5f &&
            Center.Y >= Diameter * 0.5f && Center.X <= Size.X - Diameter * 0.5f &&
            Center.Y <= Size.Y - Diameter * 0.5f;
        Planet->SetActorHiddenInGame(!bInside);
        // At the panel edge, Slate provides proper clipping for the fallback icon.
        if (bInside) Rendered.Add(Name);
    }

    for (auto It = SystemPlanetActors.CreateIterator(); It; ++It)
    {
        if (!CurrentBodies.Contains(It.Key()))
        {
            if (IsValid(It.Value())) It.Value()->Destroy();
            SystemPlanetRadii.Remove(It.Key());
            It.RemoveCurrent();
        }
    }
    SystemMapPanel->SetPlanetsRenderedIn3D(Rendered);
}

void UCmdTheaterDlg::EnsureCentralSun()
{
    if (IsValid(CentralSun))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[CmdTheaterDlg] EnsureCentralSun: World is null"));

        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    CentralSun =
        World->SpawnActor<ACentralSun>(
            ACentralSun::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams);

    if (!CentralSun)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[CmdTheaterDlg] Failed to spawn CentralSun"));

        return;
    }

    CentralSun->HideSun();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[CmdTheaterDlg] CentralSun spawned: %s"),
        *GetNameSafe(CentralSun));
}

void UCmdTheaterDlg::UpdateCentralSunVisibility()
{
    EnsureCentralSun();

    if (!CentralSun)
    {
        return;
    }

    // Only the camera-placement path may reveal the sun.
    if (CurrentViewMode != VIEW_SYSTEM || !bSunPresentationReady)
    {
        CentralSun->HideSun();
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
    SelectedOperationsGroup = nullptr;

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
        const bool bHadExplicitSector =
            !SelectedSectorName.IsEmpty();

        SectorMapPanel->SetViewedSystemName(
            SelectedSystemName);

        if (bHadExplicitSector)
        {
            SectorMapPanel->SetViewedSectorName(
                SelectedSectorName);

            SelectedSectorName =
                SectorMapPanel->GetViewedSectorName();
        }
    }

    RefreshSystemSelector();
    RefreshRegionSelector();
}

void UCmdTheaterDlg::RefreshSystemSelector()
{
    if (!SystemComboBox)
    {
        return;
    }

    TGuardValue<bool> UpdatingGuard(
        bUpdatingSystemSelector,
        true);

    SystemComboBox->ClearOptions();

    UStarshatterEnvironmentSubsystem* Env =
        GetEnvironmentSubsystem();

    if (!Env)
    {
        SystemComboBox->ClearSelection();
        return;
    }

    TArray<FString> SystemNames;

    for (StarSystem* System :
        Env->GetRuntimeStarSystems())
    {
        if (!System)
        {
            continue;
        }

        const FString SystemName =
            FString(ANSI_TO_TCHAR(System->GetName()))
            .TrimStartAndEnd();

        if (!SystemName.IsEmpty())
        {
            SystemNames.AddUnique(SystemName);
        }
    }

    for (const FString& SystemName :
        SystemNames)
    {
        SystemComboBox->AddOption(SystemName);
    }

    if (SystemNames.IsEmpty())
    {
        SelectedSystemName.Empty();
        SystemComboBox->ClearSelection();
        return;
    }

    FString SystemToSelect;

    for (const FString& SystemName :
        SystemNames)
    {
        if (SystemName.Equals(
                SelectedSystemName,
                ESearchCase::IgnoreCase))
        {
            SystemToSelect = SystemName;
            break;
        }
    }

    if (SystemToSelect.IsEmpty())
    {
        SystemToSelect = SystemNames[0];

        const bool bSystemChanged =
            !SelectedSystemName.Equals(
                SystemToSelect,
                ESearchCase::IgnoreCase);

        SelectedSystemName = SystemToSelect;

        if (bSystemChanged)
        {
            SelectedSectorName.Empty();
            SelectedStructureElement = nullptr;
            SelectedOperationsGroup = nullptr;
        }
    }

    SystemComboBox->SetSelectedOption(
        SystemToSelect);
}

void UCmdTheaterDlg::RefreshRegionSelector()
{
    if (!RegionComboBox)
    {
        return;
    }

    TGuardValue<bool> UpdatingGuard(
        bUpdatingRegionSelector,
        true);

    RegionComboBox->ClearOptions();

    StarSystem* System =
        FindRuntimeSystemByName(
            SelectedSystemName);

    if (!System)
    {
        SelectedSectorName.Empty();
        RegionComboBox->ClearSelection();
        return;
    }

    TArray<FString> RegionNames;

    ListIter<OrbitalRegion> RegionIter =
        System->GetAllRegions();

    while (++RegionIter)
    {
        OrbitalRegion* Region =
            RegionIter.value();

        if (!Region)
        {
            continue;
        }

        const FString RegionName =
            FString(
                ANSI_TO_TCHAR(
                    Region->GetName()))
            .TrimStartAndEnd();

        if (!RegionName.IsEmpty())
        {
            RegionNames.AddUnique(
                RegionName);
        }
    }

    for (const FString& RegionName :
        RegionNames)
    {
        RegionComboBox->AddOption(
            RegionName);
    }

    if (RegionNames.IsEmpty())
    {
        SelectedSectorName.Empty();
        RegionComboBox->ClearSelection();
        return;
    }

    auto FindMatchingRegion =
        [&RegionNames](const FString& Candidate) -> FString
        {
            const FString CleanCandidate =
                Candidate.TrimStartAndEnd();

            if (CleanCandidate.IsEmpty())
            {
                return FString();
            }

            for (const FString& RegionName :
                RegionNames)
            {
                if (RegionName.Equals(
                        CleanCandidate,
                        ESearchCase::IgnoreCase))
                {
                    return RegionName;
                }
            }

            return FString();
        };

    FString RegionToSelect =
        FindMatchingRegion(
            SelectedSectorName);

    // If there is no existing selection, prefer a region containing
    // an Operations major structure. This avoids opening Sector view
    // on an arbitrary empty region when static data is available.
    if (RegionToSelect.IsEmpty())
    {
        for (const FString& RegionName :
            RegionNames)
        {
            const TArray<const FS_CombatGroup*> Groups =
                CombatGroupRegistry::FindByRegion(
                    RegionName);

            bool bHasMajorStructure = false;

            for (const FS_CombatGroup* Group :
                Groups)
            {
                if (!Group)
                {
                    continue;
                }

                if (Group->Type !=
                        ECOMBATGROUP_TYPE::STATION &&
                    Group->Type !=
                        ECOMBATGROUP_TYPE::STARBASE)
                {
                    continue;
                }

                const FString GroupSystem =
                    Group->System.TrimStartAndEnd();

                if (!GroupSystem.IsEmpty() &&
                    !SelectedSystemName.IsEmpty() &&
                    !GroupSystem.Equals(
                        SelectedSystemName,
                        ESearchCase::IgnoreCase))
                {
                    continue;
                }

                bHasMajorStructure = true;
                break;
            }

            if (bHasMajorStructure)
            {
                RegionToSelect =
                    RegionName;
                break;
            }
        }
    }

    if (RegionToSelect.IsEmpty() &&
        SectorMapPanel)
    {
        RegionToSelect =
            FindMatchingRegion(
                SectorMapPanel->
                    GetViewedSectorName());
    }

    if (RegionToSelect.IsEmpty())
    {
        OrbitalRegion* ActiveRegion =
            System->ActiveRegion();

        if (ActiveRegion)
        {
            RegionToSelect =
                FindMatchingRegion(
                    ANSI_TO_TCHAR(
                        ActiveRegion->GetName()));
        }
    }

    if (RegionToSelect.IsEmpty())
    {
        RegionToSelect =
            RegionNames[0];
    }

    SelectedSectorName =
        RegionToSelect;

    RegionComboBox->SetSelectedOption(
        SelectedSectorName);

    if (SectorMapPanel)
    {
        SectorMapPanel->SetViewedSystemName(
            SelectedSystemName);

        SectorMapPanel->SetViewedSectorName(
            SelectedSectorName);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Region selector: System=%s Regions=%d Selected=%s"),
        *SelectedSystemName,
        RegionNames.Num(),
        *SelectedSectorName);
}

void UCmdTheaterDlg::SetPanelBackgroundVisible(bool bVisible)
{
    if (!Border_0)
    {
        return;
    }

    FLinearColor BrushColor =
        Border_0->GetBrushColor();

    BrushColor.A =
        bVisible ? 1.0f : 0.0f;

    Border_0->SetBrushColor(
        BrushColor);
}

void UCmdTheaterDlg::SetViewMode(
    EViewMode NewMode)
{
    if (CurrentViewMode != NewMode)
    {
        bSunPresentationReady = false;
        SunLayoutStableFrames = 0;
        if (IsValid(CentralSun)) CentralSun->HideSun();
    }
    CurrentViewMode = NewMode;
    SetPanelBackgroundVisible(CurrentViewMode != VIEW_SYSTEM);

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

    UpdateCentralSunVisibility();
    UpdateSystemSunCamera();
    UpdateSystemPlanets();

    RefreshViewButtons();
}

void UCmdTheaterDlg::RefreshViewButtons()
{
    // Hierarchical Operations navigation:
    //
    // GALAXY:
    //   SYSTEM [dropdown]
    //
    // SYSTEM:
    //   SECTOR [dropdown]
    //   double-click primary star -> GALAXY
    //
    // SECTOR:
    //   GALAXY [return]   SYSTEM [return]   SECTOR [dropdown]

    if (GalaxyButton)
    {
        // No Galaxy button on Galaxy or System views.
        // System -> Galaxy is performed by double-clicking the central star.
        GalaxyButton->SetVisibility(
            CurrentViewMode == VIEW_REGION
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);

        GalaxyButton->SetIsEnabled(true);
    }

    if (SystemButton)
    {
        SystemButton->SetVisibility(
            CurrentViewMode == VIEW_REGION
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);

        SystemButton->SetIsEnabled(true);
    }

    // Ordinary Sector button is not used.
    if (SectorButton)
    {
        SectorButton->SetVisibility(
            ESlateVisibility::Collapsed);
    }

    if (SystemSelectorBox)
    {
        SystemSelectorBox->SetVisibility(
            CurrentViewMode == VIEW_GALAXY
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);
    }

    if (RegionSelectorBox)
    {
        RegionSelectorBox->SetVisibility(
            (CurrentViewMode == VIEW_SYSTEM ||
             CurrentViewMode == VIEW_REGION)
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);
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
        SelectedOperationsGroup = nullptr;
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

void UCmdTheaterDlg::HandleSystemPrimaryStarActivated(
    const FString& InSystemName)
{
    const FString CleanName =
        InSystemName.TrimStartAndEnd();

    if (!CleanName.IsEmpty())
    {
        SelectedSystemName = CleanName;
    }

    // Keep the corresponding system highlighted when the Galaxy map returns.
    SyncMapContext();

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Primary star double-click -> Galaxy: %s"),
        *SelectedSystemName);

    SetViewMode(
        VIEW_GALAXY);
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
        TEXT("[CmdTheaterDlg] Legacy mission element selected in Theater: %s"),
        ANSI_TO_TCHAR(
            InElement->GetName()));
}

void UCmdTheaterDlg::HandleOperationsGroupSelected(
    const FS_CombatGroup* InGroup)
{
    SelectedOperationsGroup = InGroup;
    SelectedStructureElement = nullptr;

    if (!InGroup)
    {
        return;
    }

    SelectedSectorName =
        InGroup->Region.TrimStartAndEnd();

    FString GroupName =
        InGroup->DisplayName.TrimStartAndEnd();

    if (GroupName.IsEmpty())
    {
        GroupName =
            InGroup->Name.TrimStartAndEnd();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Operations group selected: %s Region=%s Type=%d"),
        *GroupName,
        *SelectedSectorName,
        static_cast<int32>(InGroup->Type));
}

void UCmdTheaterDlg::OnSystemSelectionChanged(
    FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    if (bUpdatingSystemSelector)
    {
        return;
    }

    const FString CleanSystem =
        SelectedItem.TrimStartAndEnd();

    if (CleanSystem.IsEmpty())
    {
        return;
    }

    const bool bChanged =
        !SelectedSystemName.Equals(
            CleanSystem,
            ESearchCase::IgnoreCase);

    SelectedSystemName = CleanSystem;

    if (bChanged)
    {
        SelectedSectorName.Empty();
        SelectedStructureElement = nullptr;
        SelectedOperationsGroup = nullptr;

        if (SectorMapPanel)
        {
            SectorMapPanel->SetSelectedOperationsGroup(nullptr);
        }
    }

    if (SystemMapPanel)
    {
        SystemMapPanel->SetViewedSystemName(
            SelectedSystemName);

        SystemMapPanel->SetSelectedBodyName(
            TEXT(""));

        SystemMapPanel->ShowSystemOverview();
    }

    SyncMapContext();

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Operations system selected: %s"),
        *SelectedSystemName);

    // Choosing a system from the Galaxy screen drills into System view.
    SetViewMode(VIEW_SYSTEM);
}

void UCmdTheaterDlg::OnRegionSelectionChanged(
    FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    if (bUpdatingRegionSelector)
    {
        return;
    }

    const FString CleanRegion =
        SelectedItem.TrimStartAndEnd();

    if (CleanRegion.IsEmpty())
    {
        return;
    }

    SelectedSectorName = CleanRegion;
    SelectedStructureElement = nullptr;
    SelectedOperationsGroup = nullptr;

    if (SectorMapPanel)
    {
        SectorMapPanel->SetSelectedOperationsGroup(nullptr);

        if (!SelectedSystemName.IsEmpty())
        {
            SectorMapPanel->SetViewedSystemName(
                SelectedSystemName);
        }

        SectorMapPanel->SetViewedSectorName(
            SelectedSectorName);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdTheaterDlg] Operations sector selected: System=%s Region=%s"),
        *SelectedSystemName,
        *SelectedSectorName);

    // Choosing a sector/region from the System screen drills into Sector view.
    // If already in Sector view, stay there and simply switch the displayed region.
    if (CurrentViewMode != VIEW_REGION)
    {
        SetViewMode(VIEW_REGION);
    }
    else
    {
        RefreshViewButtons();
    }
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
