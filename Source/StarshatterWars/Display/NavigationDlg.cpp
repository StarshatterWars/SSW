#include "NavigationDlg.h"
#include "MissionNavDlg.h"
#include "NavigationPopup.h"
#include "Engine/World.h"
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
    return FText::FromString(TEXT("Navigation"));
}

void UNavigationDlg::NativeDestruct()
{
    // Release the Slate content before releasing its UMG map owner.
    Super::NativeDestruct();
    if(NavigationMap)NavigationMap->RemoveFromParent();
    NavigationMap=nullptr;
}
