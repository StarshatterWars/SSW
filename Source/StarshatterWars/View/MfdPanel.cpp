#include "MfdPanel.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "HUDView.h"
#include "MFDView.h"
#include "Sensor.h"
#include "Ship.h"

static UButton* MakeCorner(UWidgetTree* Tree, UCanvasPanel* Root, const FVector2D& Pos, const TCHAR* Label)
{
    UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(Label));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Button->AddChild(Text);
    if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Button))
    {
        Slot->SetPosition(Pos);
        Slot->SetSize(FVector2D(52.f, 18.f));
        Slot->SetZOrder(2);
    }
    return Button;
}

void UMfdPanel::NativeConstruct()
{
    Super::NativeConstruct();
    BuildIfNeeded();
}

void UMfdPanel::BuildIfNeeded()
{
    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);
    if (!WidgetTree || WidgetTree->RootWidget)
        return;

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MfdRoot"));
    WidgetTree->RootWidget = Root;

    ModeButton = MakeCorner(WidgetTree, Root, FVector2D(2.f, 2.f), TEXT("MODE"));
    RangeButton = MakeCorner(WidgetTree, Root, FVector2D(2.f, 108.f), TEXT("RANGE"));
    DispButton = MakeCorner(WidgetTree, Root, FVector2D(74.f, 2.f), TEXT("DISP"));
    ProbeButton = MakeCorner(WidgetTree, Root, FVector2D(74.f, 108.f), TEXT("PROBE"));

    if (ModeButton) ModeButton->OnClicked.AddUniqueDynamic(this, &UMfdPanel::OnMode);
    if (RangeButton) RangeButton->OnClicked.AddUniqueDynamic(this, &UMfdPanel::OnRange);
    if (DispButton) DispButton->OnClicked.AddUniqueDynamic(this, &UMfdPanel::OnDisp);
    if (ProbeButton) ProbeButton->OnClicked.AddUniqueDynamic(this, &UMfdPanel::OnProbe);
}

FReply UMfdPanel::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::RightMouseButton)
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    if (Local.Y > InGeometry.GetLocalSize().Y * 0.5f && Local.X < InGeometry.GetLocalSize().X * 0.5f)
        HandleCorner(1, true);
    return FReply::Handled();
}

void UMfdPanel::OnMode() { HandleCorner(0, false); }
void UMfdPanel::OnRange() { HandleCorner(1, false); }
void UMfdPanel::OnDisp() { HandleCorner(2, false); }
void UMfdPanel::OnProbe() { HandleCorner(3, false); }

void UMfdPanel::HandleCorner(int32 Corner, bool bRight)
{
    HUDView* Hud = HUDView::GetInstance();
    MFDView* Mfd = Hud ? Hud->GetMFD(MfdIndex) : nullptr;
    Ship* ShipPtr = Mfd ? Mfd->GetShip() : nullptr;
    Sensor* SensorPtr = ShipPtr ? ShipPtr->GetSensor() : nullptr;

    if (Corner == 0 && SensorPtr)
        SensorPtr->SetMode(Sensor::GetNextSensorMode(SensorPtr->GetMode()));
    else if (Corner == 1 && SensorPtr)
        bRight ? SensorPtr->DecreaseRange() : SensorPtr->IncreaseRange();
    else if (Corner == 2 && Hud)
        Hud->CycleMFDMode(MfdIndex);
    else if (Corner == 3 && ShipPtr && ShipPtr->GetProbeLauncher())
        ShipPtr->LaunchProbe();
}
