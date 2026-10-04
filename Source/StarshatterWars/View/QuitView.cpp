#include "QuitView.h"
#include "QuitMissionMenu.h"
#include "Sim.h"
#include "Ship.h"
#include "SimContact.h"
#include "SSWRuntimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"

class SSimulationMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSimulationMenu) {}
        SLATE_ARGUMENT(QuitView*, Owner)
        SLATE_ARGUMENT(TSharedPtr<SWidget>, Content)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Owner = Args._Owner;
        ChildSlot [ Args._Content.ToSharedRef() ];
    }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event) override
    {
        if (Event.IsRepeat()) return FReply::Handled();
        const FKey Key = Event.GetKey();
        int32 Code = Key == EKeys::Escape ? 27 :
            Key == EKeys::One ? '1' : Key == EKeys::Two ? '2' :
            Key == EKeys::Three ? '3' : Key == EKeys::Four ? '4' : 0;
        return Owner && Owner->OnKeyDown(Code, false) ? FReply::Handled() : FReply::Unhandled();
    }
private:
    QuitView* Owner = nullptr;
};

QuitView* QuitView::quit_view = nullptr;
QuitView::QuitView(View* Parent) : View(Parent, 0, 0, 1024, 768) { quit_view = this; }
QuitView::~QuitView() { CloseMenu(); if (quit_view == this) quit_view = nullptr; }
void QuitView::Initialize(View* Parent) { if (!quit_view) new QuitView(Parent); }
void QuitView::Close() { delete quit_view; quit_view = nullptr; }
void QuitView::Configure(APlayerController* PC, TFunction<void(uintptr_t)> Handler, TSubclassOf<UQuitMissionMenu> WidgetClass)
{
    MenuWidgetClass = WidgetClass;
    Controller = PC;
    ActionHandler = MoveTemp(Handler);
}

bool QuitView::CanAccept()
{
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Simulation || !Simulation->GetMission() || !Player) { Rejection = TEXT("No active player ship: there are no mission results to accept."); return false; }
    if (Player->GetMissionClockMS() < 60000)
    { Rejection = TEXT("Mission too short. Fly for at least 60 seconds, or abort."); return false; }
    ListIter<SimContact> Iter = Player->GetContactList();
    while (++Iter)
    {
        SimContact* Contact = Iter.value();
        if (!Contact) continue;
        Ship* Other = Contact->GetShip();
        const int IFF = Contact->GetIFF(Player);
        const double Distance = FVector::Distance(Contact->Location(), Player->GetLocation());
        if (Contact->Threat(Player) || (Other && IFF > 0 && IFF != Player->GetIFF() &&
            ((Other->IsDropship() && Distance < 50000.0) || (Other->IsStarship() && Distance < 100000.0))))
        { Rejection = TEXT("Threats are present. Resume the mission or abort."); return false; }
    }
    Rejection.Empty();
    return true;
}

void QuitView::ShowMenu()
{
    APlayerController* PC = Controller.Get();
    UWorld* World = PC ? PC->GetWorld() : nullptr;
    if (bMenuShown || !World || !World->GetGameViewport()) return;
    bMenuShown = true;
    Rejection.Empty();
    bPreviousWorldPaused = UGameplayStatics::IsGamePaused(World);
    bPreviousCursor = PC->bShowMouseCursor;
    if (USSWRuntimeSubsystem* Runtime = World->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
    { bPreviousRuntimePaused = Runtime->IsPaused(); Runtime->SetPaused(true); }
    UGameplayStatics::SetGamePaused(World, true);
    if (MenuWidgetClass)
    {
        UQuitMissionMenu* Widget = CreateWidget<UQuitMissionMenu>(PC, MenuWidgetClass);
        if (Widget)
        {
            MenuWidget = Widget;
            Widget->AddToViewport(2000);
            Widget->OnActionRequested.BindLambda([this](EMissionQuitAction Action) { ExecuteAction(static_cast<uintptr_t>(Action)); });
            Widget->SetStatusMessage(FText::GetEmpty());
            PC->bShowMouseCursor = true;
            FInputModeUIOnly Mode;
            Mode.SetWidgetToFocus(Widget->TakeWidget());
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(Mode);
            return;
        }
        UE_LOG(LogTemp, Warning, TEXT("[QuitView] Cannot create mission menu widget; using Slate fallback"));
    }
    TSharedRef<SVerticalBox> Items = SNew(SVerticalBox);
    Items->AddSlot().AutoHeight().Padding(12)[SNew(STextBlock).Text(FText::FromString(TEXT("SIMULATION MENU")))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))];
    const TCHAR* Labels[] = { TEXT("1. End Mission and Accept Results"), TEXT("2. Abort and Discard Mission"),
        TEXT("3. Resume Current Mission"), TEXT("4. Control Setup") };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Items->AddSlot().AutoHeight().Padding(6)[SNew(SButton)
            .OnClicked_Lambda([this, Index]() { ExecuteAction(Index + 1); return FReply::Handled(); })
            [SNew(STextBlock).Text(FText::FromString(Labels[Index])).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))]];
    }
    Items->AddSlot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true)
        .Text_Lambda([this]() { return FText::FromString(Rejection); })];
    TSharedRef<SWidget> Content = SNew(SOverlay)
        + SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0,0,0,0.65f))]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(580)[SNew(SBorder).Padding(20)[Items]]];
    Overlay = SNew(SSimulationMenu).Owner(this).Content(Content);
    World->GetGameViewport()->AddViewportWidgetContent(Overlay.ToSharedRef(), 2000);
    PC->bShowMouseCursor = true;
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Overlay);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(Mode);
}

void QuitView::CloseMenu()
{
    if (!bMenuShown) return;
    bMenuShown = false;
    if (UQuitMissionMenu* Widget = MenuWidget.Get())
    {
        Widget->OnActionRequested.Unbind();
        Widget->SetDialogInputEnabled(false);
        Widget->RemoveFromParent();
    }
    MenuWidget.Reset();
    if (APlayerController* PC = Controller.Get())
    {
        UWorld* World = PC->GetWorld();
        if (World && World->GetGameViewport() && Overlay)
            World->GetGameViewport()->RemoveViewportWidgetContent(Overlay.ToSharedRef());
        if (World)
        {
            UGameplayStatics::SetGamePaused(World, bPreviousWorldPaused);
            if (USSWRuntimeSubsystem* Runtime = World->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
                Runtime->SetPaused(bPreviousRuntimePaused);
        }
        PC->bShowMouseCursor = bPreviousCursor;
        PC->SetInputMode(FInputModeGameOnly());
    }
    Overlay.Reset();
}

bool QuitView::OnKeyDown(int32 Key, bool bRepeat)
{
    if (!bMenuShown || bRepeat) return false;
    if (Key == 27) { ExecuteAction(Resume); return true; }
    if (Key >= '1' && Key <= '4') { ExecuteAction(Key - '0'); return true; }
    return false;
}
void QuitView::ExecuteAction(uintptr_t Action)
{
    if (!bMenuShown || Action < Accept || Action > Controls) return;
    if (Action == Accept && !CanAccept())
    {
        if (UQuitMissionMenu* Widget = MenuWidget.Get()) Widget->SetStatusMessage(FText::FromString(Rejection));
        return;
    }
    CloseMenu();
    if (ActionHandler) ActionHandler(Action);
}
