#include "CockpitMiniControls.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "FighterHUDDetails.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "FlightPanelStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TConstArrayView<FCockpitShortcut> SCockpitMiniControls::Shortcuts()
{
    static const FCockpitShortcut Actions[]={
        {TEXT("Contacts"),TEXT("Contacts"),EShipSystemIcon::Sensor},
        {TEXT("Engineering"),TEXT("Engineering"),EShipSystemIcon::Reactor},
        {TEXT("Weapons"),TEXT("Weapons"),EShipSystemIcon::Weapon},
        {TEXT("Navigation"),TEXT("Navigation"),EShipSystemIcon::Navigation},
        {TEXT("Radio"),TEXT("Radio — available after launch"),EShipSystemIcon::Radio},
        {TEXT("Camera"),TEXT("Camera — available after launch"),EShipSystemIcon::Camera},
        {TEXT("Cargo"),TEXT("Cargo Manifest"),EShipSystemIcon::Cargo},
        {TEXT("Player"),TEXT("Player Logbook, Hangar and Skills"),EShipSystemIcon::Gazetteer},
        {TEXT("Options"),TEXT("In-flight Options"),EShipSystemIcon::Options},
        {TEXT("Exit"),TEXT("Mission exit menu"),EShipSystemIcon::Exit}};
    return Actions;
}
int32 SCockpitMiniControls::CellAt(const FVector2D& Point)
{
    if (Point.X<0 || Point.X>=800 || Point.Y<0 || Point.Y>=64) return INDEX_NONE;
    return FMath::FloorToInt(Point.X/80);
}
void SCockpitMiniControls::Construct(const FArguments&,TWeakPtr<SFighterHUDDetails> InHUD,UWidgetComponent* InSurface)
{
    HUD=InHUD; Surface=InSurface;
    static const FButtonStyle Button=FButtonStyle().SetNormal(FlightPanelStyle::Window()).SetHovered(FlightPanelStyle::TitleBar()).SetPressed(FlightPanelStyle::Glass());
    auto Row=SNew(SHorizontalBox);
    for (const auto& Item : Shortcuts())
        Row->AddSlot().FillWidth(1).Padding(3)
        [SNew(SButton).ButtonStyle(&Button).ToolTipText(FText::FromString(Item.Tooltip))
            .IsEnabled_Lambda([Weak=HUD,Action=Item.Action]() { auto View=Weak.Pin(); return View && View->CanUseCockpitShortcut(Action); })
            .OnClicked_Lambda([Weak=HUD,Action=Item.Action]() { if (auto View=Weak.Pin()) View->ExecuteCockpitShortcut(Action); return FReply::Handled(); })
            [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SShipSystemIcon).Icon(Item.Icon)]]];
    ChildSlot[SNew(SBorder).BorderImage(&FlightPanelStyle::Window()).Padding(0)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(64)[Row]]
            + SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(32)
                [SNew(STextBlock).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Regular",20))
                    .Text_Lambda([this]() { return Hovered!=INDEX_NONE ? FText::FromString(Shortcuts()[Hovered].Tooltip) : INVTEXT("COCKPIT CONTROLS"); })]]]];
}
bool SCockpitMiniControls::RayPoint(FVector2D& Point) const
{
    auto* Screen=Surface.Get(); auto* World=Screen ? Screen->GetWorld() : nullptr;
    auto* PC=World ? World->GetFirstPlayerController() : nullptr; FVector Origin,Direction;
    if (!PC || !PC->bShowMouseCursor || !PC->DeprojectMousePositionToWorld(Origin,Direction)) return false;
    const auto Transform=Screen->GetComponentTransform();
    const FVector Start=Transform.InverseTransformPosition(Origin),Ray=Transform.InverseTransformVector(Direction);
    if (FMath::Abs(Ray.X)<UE_SMALL_NUMBER) return false;
    const double T=-Start.X/Ray.X; if (T<=0) return false;
    const FVector Hit=Start+Ray*T; Point=FVector2D(-Hit.Y,-Hit.Z); return true;
}
void SCockpitMiniControls::SampleHover()
{ FVector2D Point; Hovered=RayPoint(Point) ? CellAt(Point) : INDEX_NONE; }
bool SCockpitMiniControls::HandleClick()
{
    FVector2D Point; const int32 Cell=RayPoint(Point) ? CellAt(Point) : INDEX_NONE;
    if (Cell==INDEX_NONE) return false;
    if (auto View=HUD.Pin()) View->ExecuteCockpitShortcut(Shortcuts()[Cell].Action);
    return true;
}
