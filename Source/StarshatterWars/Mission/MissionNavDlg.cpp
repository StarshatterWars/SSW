#include "MissionNavDlg.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "Input/Reply.h"

#include "MissionPlanner.h"
#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"

UMissionNavDlg::UMissionNavDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // DO NOT access widgets here
    // DO NOT bind delegates here
    // Constructor is for defaults only
}

void UMissionNavDlg::InitializeDlg(UMissionPlanner* InManager)
{
    Manager = InManager;
}

void UMissionNavDlg::SetMissionContext(Campaign* InCampaign, Mission* InMission, MissionInfo* InInfo)
{
    CampaignPtr = InCampaign;
    MissionPtr = InMission;
    MissionInfoPtr = InInfo;
}

void UMissionNavDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Footer
    if (AcceptButton)
    {
        AcceptButton->OnClicked.Clear();
        AcceptButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnAcceptClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.Clear();
        CancelButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnCancelClicked);
    }

    // Tabs
    if (TabSitButton) { TabSitButton->OnClicked.Clear(); TabSitButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnTabSitClicked); }
    if (TabPkgButton) { TabPkgButton->OnClicked.Clear(); TabPkgButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnTabPkgClicked); }
    if (TabMapButton) { TabMapButton->OnClicked.Clear(); TabMapButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnTabMapClicked); }
    if (TabWepButton) { TabWepButton->OnClicked.Clear(); TabWepButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnTabWepClicked); }

    // Nav buttons
    if (GalaxyButton) { GalaxyButton->OnClicked.Clear(); GalaxyButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnNavGalaxyClicked); }
    if (SystemButton) { SystemButton->OnClicked.Clear(); SystemButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnNavSystemClicked); }
    if (SectorButton) { SectorButton->OnClicked.Clear(); SectorButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnNavSectorClicked); }

    // Zoom
    if (ZoomInButton) { ZoomInButton->OnClicked.Clear();  ZoomInButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnZoomInClicked); }
    if (ZoomOutButton) { ZoomOutButton->OnClicked.Clear(); ZoomOutButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnZoomOutClicked); }

    // Filters
    if (FilterSystemButton) { FilterSystemButton->OnClicked.Clear();   FilterSystemButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnFilterSystemClicked); }
    if (FilterPlanetButton) { FilterPlanetButton->OnClicked.Clear();   FilterPlanetButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnFilterPlanetClicked); }
    if (FilterSectorButton) { FilterSectorButton->OnClicked.Clear();   FilterSectorButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnFilterSectorClicked); }
    if (FilterStationButton) { FilterStationButton->OnClicked.Clear();  FilterStationButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnFilterStationClicked); }
    if (FilterStarshipButton) { FilterStarshipButton->OnClicked.Clear(); FilterStarshipButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnFilterStarshipClicked); }
    if (FilterFighterButton) { FilterFighterButton->OnClicked.Clear();  FilterFighterButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnFilterFighterClicked); }
}

FReply UMissionNavDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
    {
        OnAcceptClicked();
        return FReply::Handled();
    }

    if (Key == EKeys::Escape)
    {
        OnCancelClicked();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UMissionNavDlg::ShowDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    RefreshHeader();
    RefreshLists();
}

void UMissionNavDlg::RefreshHeader()
{
    // These are placeholders; wire into your mission/plan screen data.
    if (HeaderTitleText) HeaderTitleText->SetText(FText::FromString(TEXT("MISSION BRIEFING")));
    if (SystemValueText) SystemValueText->SetText(FText::FromString(TEXT("SYSTEM")));
    if (SectorValueText) SectorValueText->SetText(FText::FromString(TEXT("SECTOR")));
    if (TimeText)        TimeText->SetText(FText::FromString(TEXT("DAY 0 00:00:00")));
    if (BriefingBodyText) BriefingBodyText->SetText(FText::FromString(TEXT("BRIEFING TEXT GOES HERE.")));
}

void UMissionNavDlg::RefreshLists()
{
    // Intentionally left for your ListView item model wiring.
}

void UMissionNavDlg::OnAcceptClicked()
{
    if (Manager)
    {
        Manager->OnMissionBriefingAccept();
    }
}

void UMissionNavDlg::OnCancelClicked()
{
    if (Manager)
    {
        Manager->OnMissionBriefingCancel();
    }
}

// Tabs
void UMissionNavDlg::OnTabSitClicked() { if (Manager) Manager->ShowBriefingSitTab(); }
void UMissionNavDlg::OnTabPkgClicked() { if (Manager) Manager->ShowBriefingPkgTab(); }
void UMissionNavDlg::OnTabMapClicked() { if (Manager) Manager->ShowBriefingMapTab(); }
void UMissionNavDlg::OnTabWepClicked() { if (Manager) Manager->ShowBriefingWepTab(); }

// Nav mode
void UMissionNavDlg::OnNavGalaxyClicked() { if (Manager) Manager->NavModeGalaxy(); }
void UMissionNavDlg::OnNavSystemClicked() { if (Manager) Manager->NavModeSystem(); }
void UMissionNavDlg::OnNavSectorClicked() { if (Manager) Manager->NavModeSector(); }

// Zoom
void UMissionNavDlg::OnZoomInClicked() { if (Manager) Manager->NavZoomIn(); }
void UMissionNavDlg::OnZoomOutClicked() { if (Manager) Manager->NavZoomOut(); }

// Filters
void UMissionNavDlg::OnFilterSystemClicked() { if (Manager) Manager->NavFilterSystem(); }
void UMissionNavDlg::OnFilterPlanetClicked() { if (Manager) Manager->NavFilterPlanet(); }
void UMissionNavDlg::OnFilterSectorClicked() { if (Manager) Manager->NavFilterSector(); }
void UMissionNavDlg::OnFilterStationClicked() { if (Manager) Manager->NavFilterStation(); }
void UMissionNavDlg::OnFilterStarshipClicked() { if (Manager) Manager->NavFilterStarship(); }
void UMissionNavDlg::OnFilterFighterClicked() { if (Manager) Manager->NavFilterFighter(); }
