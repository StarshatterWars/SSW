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
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
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
    return FText::FromString(TEXT("NAVIGATION"));
}

void UNavigationDlg::NativeConstruct()
{
    Super::NativeConstruct();
    UCanvasPanel* TitleCanvas = PanelTitle ? Cast<UCanvasPanel>(PanelTitle->GetParent()) : nullptr;
    UCanvasPanelSlot* TitleSlot = PanelTitle ? Cast<UCanvasPanelSlot>(PanelTitle->Slot) : nullptr;
    if (!WidgetTree || !TitleCanvas || !TitleSlot) return;

    auto AddLocationTitle = [this, TitleCanvas, TitleSlot](TObjectPtr<UTextBlock>& Label, bool bRight)
    {
        if (!Label) Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetFont(PanelTitle->GetFont());
        Label->SetColorAndOpacity(PanelTitle->GetColorAndOpacity());
        Label->SetJustification(bRight ? ETextJustify::Right : ETextJustify::Left);
        Label->SetAutoWrapText(false);
        Label->SetClipping(EWidgetClipping::ClipToBounds);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        UCanvasPanelSlot* LocationSlot = Cast<UCanvasPanelSlot>(Label->Slot);
        if (!LocationSlot) LocationSlot = TitleCanvas->AddChildToCanvas(Label.Get());
        FAnchorData Layout = TitleSlot->GetLayout();
        // Share the Blueprint title's vertical position, but anchor to the bar edges.
        Layout.Anchors.Minimum.X = bRight ? 0.68f : 0.0f;
        Layout.Anchors.Maximum.X = bRight ? 1.0f : 0.32f;
        Layout.Alignment.X = 0.0f;
        Layout.Offsets.Left = 4.0f;
        Layout.Offsets.Right = 4.0f;
        if (Layout.Anchors.Minimum.Y == Layout.Anchors.Maximum.Y && Layout.Offsets.Bottom <= 0.0f)
            Layout.Offsets.Bottom = PanelTitle->GetFont().Size + 12.0f;
        LocationSlot->SetLayout(Layout);
        LocationSlot->SetAutoSize(false);
        LocationSlot->SetZOrder(TitleSlot->GetZOrder() + 1);
    };
    AddLocationTitle(SystemTitleText, false);
    AddLocationTitle(SectorTitleText, true);
    UpdateLocationTitles();
}

void UNavigationDlg::UpdateLocationTitles()
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
    const FText SystemCaption = FText::FromString(SystemText);
    const FText SectorCaption = FText::FromString(SectorText);
    if (SystemTitleText && !SystemTitleText->GetText().EqualTo(SystemCaption)) SystemTitleText->SetText(SystemCaption);
    if (SectorTitleText && !SectorTitleText->GetText().EqualTo(SectorCaption)) SectorTitleText->SetText(SectorCaption);
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
    UpdateLocationTitles();
}
