#include "QuitView.h"
#include "Engine/Texture2D.h"
#include "MissionUIStyle.h"
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
#include "Kismet/KismetSystemLibrary.h"
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
            Key == EKeys::Three ? '3' : Key == EKeys::Four ? '4' :
            Key == EKeys::Five ? '5' :
            Key == EKeys::NumPadOne ? '1' : Key == EKeys::NumPadTwo ? '2' :
            Key == EKeys::NumPadThree ? '3' : Key == EKeys::NumPadFour ? '4' :
            Key == EKeys::NumPadFive ? '5' : 0;
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
    const FLinearColor XTextColor(0.79f, 0.93f, 1.0f, 1.0f);
    TSharedRef<SVerticalBox> Items = SNew(SVerticalBox);
    Items->AddSlot().AutoHeight().Padding(12, 10, 12, 22)
    [SNew(STextBlock).Text(FText::FromString(TEXT("MISSION CONTROL  /  PAUSED")))
        .ColorAndOpacity(FLinearColor(0.53f, 0.84f, 0.96f))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))];
    const TCHAR* Labels[] = {
        TEXT("1.   END MISSION  /  ACCEPT RESULTS"),
        TEXT("2.   ABORT MISSION  /  DISCARD RESULTS"),
        TEXT("3.   RESUME MISSION"),
        TEXT("4.   CONTROL SETUP"),
        TEXT("5.   QUIT AND EXIT TO WINDOWS")
    };
    PauseButtonStyles.SetNum(5);
    PauseButtonTextures.Reset();
    const TCHAR* Colors[] = {TEXT("Green"), TEXT("Red"), TEXT("Green"), TEXT("Blue"), TEXT("Red")};
    for (int32 Index = 0; Index < 5; ++Index)
    {
        auto Brush = [&](const TCHAR* State)
        {
            FSlateBrush Result;
            const FString Path = FString::Printf(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01%s_%s.T_Btn01%s_%s"), Colors[Index], State, Colors[Index], State);
            if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path))
            {
                PauseButtonTextures.Emplace(Texture);
                Result.SetResourceObject(Texture);
                Result.DrawAs = ESlateBrushDrawType::Image;
            }
            return Result;
        };
        PauseButtonStyles[Index].Normal = Brush(TEXT("n"));
        PauseButtonStyles[Index].Hovered = Brush(TEXT("p"));
        PauseButtonStyles[Index].Pressed = PauseButtonStyles[Index].Hovered;
    }
    for (int32 Index = 0; Index < 5; ++Index)
    {
        Items->AddSlot().AutoHeight().Padding(12, 5)
        [SNew(SButton)
            .ButtonStyle(&PauseButtonStyles[Index])
            .ButtonColorAndOpacity(FLinearColor::White)
            .ContentPadding(FMargin(12, 11))
            .HAlign(HAlign_Left)
            .OnClicked_Lambda([this, Index]() { ExecuteAction(Index + 1); return FReply::Handled(); })
            [SNew(STextBlock)
                .Justification(ETextJustify::Left)
                .ColorAndOpacity(XTextColor)
                .Text(FText::FromString(Labels[Index]))
                .Font(MissionUIStyle::GetHeaderFont(17))]];
    }
    Items->AddSlot().AutoHeight().Padding(12, 9)
    [SNew(STextBlock).AutoWrapText(true)
        .ColorAndOpacity(FLinearColor(1.0f, 0.7f, 0.35f))
        .Text_Lambda([this]() { return FText::FromString(Rejection); })];
    TSharedRef<SWidget> Content = SNew(SOverlay)
        + SOverlay::Slot()
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.005f, 0.012f, 0.025f, 0.83f))]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(620)
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0.018f, 0.037f, 0.065f, 1.0f))
                .Padding(22)[Items]]];
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
    PauseButtonStyles.Reset();
    PauseButtonTextures.Reset();
}

bool QuitView::OnKeyDown(int32 Key, bool bRepeat)
{
    if (!bMenuShown || bRepeat) return false;
    if (Key == 27) { ExecuteAction(Resume); return true; }
    if (Key >= '1' && Key <= '5') { ExecuteAction(Key - '0'); return true; }
    return false;
}
void QuitView::ExecuteAction(uintptr_t Action)
{
    if (!bMenuShown || Action < Accept || Action > ExitWindows) return;
    if (Action == Accept && !CanAccept())
    {
        if (UQuitMissionMenu* Widget = MenuWidget.Get()) Widget->SetStatusMessage(FText::FromString(Rejection));
        return;
    }
    // Exit is handled locally, so it cannot fall through into the existing
    // mission Abort/Accept handler (which would change campaign state).
    if (Action == ExitWindows)
    {
        APlayerController* PC = Controller.Get();
        CloseMenu();
        if (PC) UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false);
        return;
    }
    CloseMenu();
    if (ActionHandler) ActionHandler(Action);
}
