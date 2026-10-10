#include "NavigationDlg.h"
#include "MissionNavDlg.h"
#include "NavigationPopup.h"
#include "Engine/World.h"
#include "Sim.h"
#include "Ship.h"
#include "Mission.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Components/TextBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UNavigationDlg::CreatePanelContent()
{
    if (bFrameOnly)
    {
        UE_LOG(LogTemp, Display, TEXT("NavigationDlg: showing frame-only panel %s"), *GetName());
        return SNullWidget::NullWidget;
    }
    if(!NavigationMapClass) {
        NavigationMapClass=LoadClass<UMissionNavDlg>(nullptr,
            TEXT("/Game/Screens/Mission/MissionNavPanel.MissionNavPanel_C"));
    }
    UClass* MapClass=NavigationMapClass?NavigationMapClass.Get():UMissionNavDlg::StaticClass();
    NavigationMap=CreateWidget<UMissionNavDlg>(GetOwningPlayer(),MapClass);
    if(!NavigationMap) {
        UE_LOG(LogTemp,Error,TEXT("NavigationDlg: failed to create %s"),*GetNameSafe(MapClass));
        return SNew(STextBlock).Text(FText::FromString(TEXT("Navigation map could not be created.")));
    }
    NavigationMap->SetInMissionNavigation(true);
    NavigationMap->SetVisibility(ESlateVisibility::Visible);
    const TSharedRef<SWidget> MapBody=NavigationMap->TakeWidget();
    NavigationMap->RefreshFromMission();
    const TWeakObjectPtr<UNavigationDlg> Owner(this);
    return SNew(SNavigationPopup).Embedded(true).MapWidget(MapBody)
        .FooterHost(FooterContent).FooterStyle(&FooterButtonStyle)
        .ResolveShip([Owner]()->Ship*{return Owner.IsValid()?Owner->ResolvePanelShip():nullptr;})
        .CanOperate([Owner](){return Owner.IsValid() && Owner->GetWorld() && !Owner->GetWorld()->IsPaused();})
        .OnClose(FSimpleDelegate::CreateWeakLambda(this,[this](){RequestPanelClose();}));
}

FText UNavigationDlg::GetPanelCaption() const
{
    // Resolve through the live simulation each time; never retain simulation pointers.
    auto* LiveSim = Sim::GetSim();
    auto* LiveMission = LiveSim ? LiveSim->GetMission() : nullptr;
    Ship* Player = ResolvePanelShip();
    SimRegion* Region = Player ? Player->GetRegion() : nullptr;
    const char* SystemName = Region && Region->GetSystem() ? Region->GetSystem()->GetName()
        : (LiveMission ? LiveMission->GetSystem() : nullptr);
    const char* SectorName = Region ? Region->GetName()
        : (LiveMission ? LiveMission->GetRegion() : nullptr);
    const FString SystemText = SystemName && *SystemName ? UTF8_TO_TCHAR(SystemName) : TEXT("Unknown system");
    const FString SectorText = SectorName && *SectorName ? UTF8_TO_TCHAR(SectorName) : TEXT("Unknown sector");
    return FText::FromString(FString::Printf(TEXT("NAVIGATION — %s — %s"), *SystemText, *SectorText));
}

void UNavigationDlg::NativeDestruct()
{
    // Release the Slate content before releasing its UMG map owner.
    Super::NativeDestruct();
    if(NavigationMap)NavigationMap->RemoveFromParent();
    NavigationMap=nullptr;
}

void UNavigationDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (PanelTitle)
    {
        const FText Caption = GetPanelCaption();
        if (!PanelTitle->GetText().EqualTo(Caption)) PanelTitle->SetText(Caption);
    }
}
