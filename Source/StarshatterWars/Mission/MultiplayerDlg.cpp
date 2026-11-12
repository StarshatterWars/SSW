#include "MultiplayerDlg.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "SSWSessionLobbyGameMode.h"
#include "SSWSessionSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/Spacer.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameInstance.h"
#include "MissionUIStyle.h"
#include "StarshatterUIStyleSubsystem.h"
#include "MenuScreen.h"

namespace
{
    constexpr float PlayerColumnWidth = 145.f;
    constexpr float PingColumnWidth = 100.f;
    constexpr float SessionRowInset = 16.f;
    constexpr float BrowserInset = 10.f;

    void StyleSessionRow(UButton& Button, bool bSelected = false)
    {
        FButtonStyle Style = Button.GetStyle();
        const FLinearColor Surface = bSelected ? FLinearColor(.029f, .098f, .143f) : FLinearColor(.019f, .043f, .064f);
        const FLinearColor Edge = bSelected ? FLinearColor(.08f, .42f, .56f) : FLinearColor(.045f, .093f, .125f);
        Style.Normal = FSlateRoundedBoxBrush(Surface, 5.f, Edge, 1.f);
        Style.Hovered = FSlateRoundedBoxBrush(FLinearColor(.027f, .081f, .116f), 5.f, FLinearColor(.06f, .27f, .37f), 1.f);
        Style.Pressed = FSlateRoundedBoxBrush(FLinearColor(.032f, .111f, .15f), 5.f, FLinearColor(.09f, .47f, .6f), 1.f);
        Style.Disabled = Style.Normal;
        Style.NormalPadding = Style.PressedPadding = FMargin(0);
        Button.SetStyle(Style);
    }

    FString MultiplayerMissionLabel(const FString& Name)
    {
        return Name.Equals(TEXT("Hawk Match"), ESearchCase::IgnoreCase) ? FString(TEXT("Battle Royale")) : Name;
    }
}

void USSWSessionRowButton::Configure(UMultiplayerDlg* Screen,int32 Row,bool bMissionRow)
{
    Dialog=Screen;
    Index=Row;
    bMission=bMissionRow;
    OnClicked.AddUniqueDynamic(this,&ThisClass::Clicked);
}

void USSWSessionRowButton::Clicked()
{
    if (auto* Screen = Dialog.Get())
    {
        if (bMission) Screen->HostMission(Index);
        else Screen->SelectSession(Index);
    }
}
UMultiplayerDlg::UMultiplayerDlg(const FObjectInitializer& I):Super(I){SetIsFocusable(true);}
USSWSessionSubsystem* UMultiplayerDlg::Service()const{return GetGameInstance()?GetGameInstance()->GetSubsystem<USSWSessionSubsystem>():nullptr;}
TSharedRef<SWidget> UMultiplayerDlg::RebuildWidget(){if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);BuildSessionScreen();return Super::RebuildWidget();}
UTextBlock* UMultiplayerDlg::Label(const FString& Text,int32 Size)
{
    auto* W=WidgetTree->ConstructWidget<UTextBlock>();
    W->SetText(FText::FromString(Text));W->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::Clamp(Size - 2, 11, 22)));
    W->SetColorAndOpacity(FSlateColor(FLinearColor(.78f,.9f,.96f)));W->SetAutoWrapText(false);
    return W;
}
UButton* UMultiplayerDlg::Button(const FString& Text,UVerticalBox* Parent)
{
    auto* B=WidgetTree->ConstructWidget<UButton>();B->AddChild(Label(Text,16));
    B->SetToolTipText(FText::FromString(Text));MissionUIStyle::ApplyActionButtonStyle(B,TEXT("Cyan"));
    CastChecked<UTextBlock>(B->GetContent())->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 13));
    auto* Height=WidgetTree->ConstructWidget<USizeBox>();Height->SetHeightOverride(48);Height->SetContent(B);
    Parent->AddChildToVerticalBox(Height)->SetPadding(FMargin(0,6));return B;
}
void UMultiplayerDlg::BuildSessionScreen()
{
    if(WidgetTree->RootWidget && WidgetTree->RootWidget->GetFName()==TEXT("SSWNativeSessionScreen"))return;
    auto* Frame=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("SSWNativeSessionScreen"));
    Frame->SetBrushColor(FLinearColor(.008f,.017f,.03f,1.f));Frame->SetPadding(FMargin(28));WidgetTree->RootWidget=Frame;
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);Frame->SetContent(Scale);
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1080);Size->SetHeightOverride(740);Scale->SetContent(Size);
    auto* Main=WidgetTree->ConstructWidget<UVerticalBox>();Size->SetContent(Main);
    Main->SetClipping(EWidgetClipping::ClipToBounds);
    if (GetOwningPlayer<ASSWSessionLobbyController>())
    {
        Frame->SetBrushColor(FLinearColor(0.008f, 0.017f, 0.03f, 0.18f));
        auto* PlayField = WidgetTree->ConstructWidget<UOverlay>();
        Size->SetContent(PlayField);
        auto* Background = PlayField->AddChildToOverlay(Main);
        Background->SetHorizontalAlignment(HAlign_Fill);
        Background->SetVerticalAlignment(VAlign_Fill);
        BuildLobbyScreen(Main, PlayField);
        return;
    }

    auto* Header=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(Header)->SetPadding(FMargin(0,0,0,18));
    auto* ScreenTitle = Label(TEXT("MULTIPLAYER"), 30);
    ScreenTitle->SetFont(MissionUIStyle::GetHeaderFont(38));
    Header->AddChildToVerticalBox(ScreenTitle);
    auto* Modes = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Modes)->SetPadding(FMargin(0, 0, 0, 12));
    auto AddTab = [&](const TCHAR* Caption)
    {
        auto* Container = WidgetTree->ConstructWidget<UVerticalBox>();
        auto* Width = WidgetTree->ConstructWidget<USizeBox>();
        Width->SetWidthOverride(210.f);
        Width->SetContent(Container);
        Modes->AddChildToHorizontalBox(Width)->SetPadding(FMargin(0, 0, 8, 0));
        auto* Tab = Button(Caption, Container);
        Tab->SetToolTipText(FText::GetEmpty());
        return Tab;
    };
    HostTab = AddTab(TEXT("CREATE SESSION"));
    HostTab->OnClicked.AddUniqueDynamic(this, &ThisClass::HostMode);
    JoinTab = AddTab(TEXT("JOIN SESSION"));
    JoinTab->OnClicked.AddUniqueDynamic(this, &ThisClass::JoinMode);
    Modes->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LAN = WidgetTree->ConstructWidget<UCheckBox>();
    LAN->SetIsChecked(true);
    LAN->SetContent(Label(TEXT("LAN game (local network)"), 16));
    Modes->AddChildToHorizontalBox(LAN)->SetVerticalAlignment(VAlign_Center);
    bModeSelected = true;
    HostPanel=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(HostPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* HostCard=WidgetTree->ConstructWidget<UBorder>();HostCard->SetBrushColor(FLinearColor(.012f,.028f,.045f));HostCard->SetPadding(FMargin(22));
    HostPanel->AddChildToVerticalBox(HostCard)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Form=WidgetTree->ConstructWidget<UVerticalBox>();HostCard->SetContent(Form);
    auto* Settings = WidgetTree->ConstructWidget<UHorizontalBox>();
    Form->AddChildToVerticalBox(Settings)->SetPadding(FMargin(0, 0, 0, 20));
    auto* NameField = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* NameSlot = Settings->AddChildToHorizontalBox(NameField);
    NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    NameSlot->SetPadding(FMargin(0, 0, 16, 0));
    NameField->AddChildToVerticalBox(Label(TEXT("SESSION NAME"), 14))->SetPadding(FMargin(0, 0, 0, 6));
    SessionName=WidgetTree->ConstructWidget<UEditableTextBox>();SessionName->SetText(FText::FromString(TEXT("Starshatter Wars")));
    FEditableTextBoxStyle InputStyle=SessionName->GetWidgetStyle();
    FSlateBrush InputBrush=*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));InputBrush.TintColor=FSlateColor(FLinearColor(.008f,.025f,.04f));
    InputStyle.BackgroundImageNormal=InputBrush;InputStyle.BackgroundImageHovered=InputBrush;InputStyle.BackgroundImageFocused=InputBrush;InputStyle.BackgroundImageReadOnly=InputBrush;
    InputStyle.SetForegroundColor(FSlateColor(FLinearColor(.9f,.96f,1.f)));InputStyle.SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14));InputStyle.Padding=FMargin(12,8);
    SessionName->SetWidgetStyle(InputStyle);
    NameField->AddChildToVerticalBox(SessionName);
    auto* PlayerField = WidgetTree->ConstructWidget<UVerticalBox>();
    Settings->AddChildToHorizontalBox(PlayerField);
    PlayerField->AddChildToVerticalBox(Label(TEXT("PLAYER LIMIT"), 14))->SetPadding(FMargin(0, 0, 0, 6));
    auto* CapacitySize=WidgetTree->ConstructWidget<USizeBox>();CapacitySize->SetWidthOverride(160);CapacitySize->SetHeightOverride(44);
    PlayerField->AddChildToVerticalBox(CapacitySize);
    Capacity=WidgetTree->ConstructWidget<USpinBox>();Capacity->SetMinValue(2);Capacity->SetMaxValue(FS_CampaignMission::MultiplayerConnectionLimit);Capacity->SetMinSliderValue(2);Capacity->SetMaxSliderValue(FS_CampaignMission::MultiplayerConnectionLimit);Capacity->SetDelta(1);Capacity->SetValue(4);
    Capacity->SetMinFractionalDigits(0);Capacity->SetMaxFractionalDigits(0);Capacity->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14));Capacity->SetForegroundColor(FSlateColor(FLinearColor::White));
    FSpinBoxStyle SpinStyle=Capacity->GetWidgetStyle();SpinStyle.BackgroundBrush=InputBrush;SpinStyle.HoveredBackgroundBrush=InputBrush;SpinStyle.InactiveFillBrush=InputBrush;SpinStyle.ActiveFillBrush=InputBrush;Capacity->SetWidgetStyle(SpinStyle);CapacitySize->SetContent(Capacity);
    Form->AddChildToVerticalBox(Label(TEXT("CHOOSE A MISSION"), 18))->SetPadding(FMargin(0, 0, 0, 10));
    auto* Catalog = WidgetTree->ConstructWidget<UScrollBox>();
    Catalog->SetClipping(EWidgetClipping::ClipToBounds);
    Form->AddChildToVerticalBox(Catalog)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    if (const auto* Data = GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>())
    {
        const auto& Missions = Data->GetMultiplayerMissionScenarios();
        for (int32 Index = 0; Index < Missions.Num(); ++Index)
        {
            auto* Row = WidgetTree->ConstructWidget<USSWSessionRowButton>();
            Row->Configure(this, Index, true);
            const auto& Mission = Missions[Index];
            auto* Card = WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* Copy = WidgetTree->ConstructWidget<UVerticalBox>();
            auto* CopySlot = Card->AddChildToHorizontalBox(Copy);
            CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CopySlot->SetVerticalAlignment(VAlign_Center);
            Copy->AddChildToVerticalBox(Label(MultiplayerMissionLabel(Mission.MissionName), 18));
            const FString Location = Mission.MissionSystem == Mission.MissionRegion
                ? Mission.MissionSystem : FString::Printf(TEXT("%s / %s"), *Mission.MissionSystem, *Mission.MissionRegion);
            Copy->AddChildToVerticalBox(Label(Location, 14))->SetPadding(FMargin(0, 2, 0, 0));
            auto* Limit = Card->AddChildToHorizontalBox(Label(FString::Printf(TEXT("Max %d players"),
                Mission.GetMultiplayerCapacity()), 14));
            Limit->SetVerticalAlignment(VAlign_Center);
            auto* CardSlot = CastChecked<UButtonSlot>(Row->AddChild(Card));
            CardSlot->SetHorizontalAlignment(HAlign_Fill);
            CardSlot->SetVerticalAlignment(VAlign_Fill);
            CardSlot->SetPadding(FMargin(SessionRowInset, 0));
            if (!Mission.Desc.IsEmpty()) Row->SetToolTipText(FText::FromString(Mission.Desc));
            StyleSessionRow(*Row);
            Row->SetClickMethod(EButtonClickMethod::MouseDown);
            auto* Height = WidgetTree->ConstructWidget<USizeBox>();
            Height->SetHeightOverride(52);
            Height->SetContent(Row);
            CastChecked<UScrollBoxSlot>(Catalog->AddChild(Height))->SetPadding(FMargin(0, 0, 0, 6));
            MissionButtons.Add(Row);
        }
    }
    if (MissionButtons.IsEmpty()) Catalog->AddChild(Label(TEXT("No multiplayer missions available."), 16));
    JoinPanel=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(JoinPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* BrowserHeader = WidgetTree->ConstructWidget<UHorizontalBox>();
    JoinPanel->AddChildToVerticalBox(BrowserHeader);
    auto* Heading = BrowserHeader->AddChildToHorizontalBox(Label(TEXT("AVAILABLE SESSIONS"), 18));
    Heading->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Heading->SetVerticalAlignment(VAlign_Center);
    auto* RefreshGroup = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* RefreshWidth = WidgetTree->ConstructWidget<USizeBox>();
    RefreshWidth->SetWidthOverride(230.f);
    RefreshWidth->SetContent(RefreshGroup);
    BrowserHeader->AddChildToHorizontalBox(RefreshWidth);
    SearchButton=Button(TEXT("FIND GAMES / REFRESH"),RefreshGroup);SearchButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Search);
    auto* Columns=WidgetTree->ConstructWidget<UHorizontalBox>();JoinPanel->AddChildToVerticalBox(Columns)->SetPadding(FMargin(BrowserInset + SessionRowInset, 12, BrowserInset + SessionRowInset, 8));
    Columns->AddChildToHorizontalBox(Label(TEXT("SESSION"),14))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    for(const TCHAR* Caption:{TEXT("PLAYERS"),TEXT("PING")}){auto* Cell=WidgetTree->ConstructWidget<USizeBox>();Cell->SetWidthOverride(FCString::Strcmp(Caption, TEXT("PLAYERS")) == 0 ? PlayerColumnWidth : PingColumnWidth);Cell->SetContent(Label(Caption,14));Columns->AddChildToHorizontalBox(Cell);}
    auto* Browser=WidgetTree->ConstructWidget<UBorder>();Browser->SetBrushColor(FLinearColor(.012f,.028f,.045f));Browser->SetPadding(FMargin(BrowserInset));
    JoinPanel->AddChildToVerticalBox(Browser)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Browser->SetContent(Scroll);
    Rows=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Rows);
    auto* JoinGroup = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* JoinWidth = WidgetTree->ConstructWidget<USizeBox>();
    JoinWidth->SetWidthOverride(230.f);
    JoinWidth->SetContent(JoinGroup);
    JoinPanel->AddChildToVerticalBox(JoinWidth)->SetHorizontalAlignment(HAlign_Right);
    JoinButton=Button(TEXT("JOIN SELECTED GAME"),JoinGroup);JoinButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Join);
    auto* StatusBar=WidgetTree->ConstructWidget<UBorder>();StatusBar->SetPadding(FMargin(12));StatusBar->SetBrushColor(FLinearColor(.012f,.033f,.05f));
    Main->AddChildToVerticalBox(StatusBar)->SetPadding(FMargin(0,12,0,8));StatusLabel=Label(TEXT("Create a session or search for games."),14);StatusLabel->SetAutoWrapText(true);StatusBar->SetContent(StatusLabel);
    auto* Footer=WidgetTree->ConstructWidget<UHorizontalBox>();Main->AddChildToVerticalBox(Footer);
    auto FooterGroup=[&](){auto* Group=WidgetTree->ConstructWidget<UVerticalBox>();auto* FooterSlot=Footer->AddChildToHorizontalBox(Group);FooterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));FooterSlot->SetPadding(FMargin(6,0));return Group;};
    CancelSearchButton=Button(TEXT("CANCEL SEARCH"),FooterGroup());CancelSearchButton->OnClicked.AddUniqueDynamic(this,&ThisClass::CancelSearch);
    LeaveButton=Button(TEXT("LEAVE SESSION"),FooterGroup());LeaveButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Leave);
    AddMainMenuButton(FooterGroup());RefreshState();
}
void UMultiplayerDlg::NativeConstruct(){Super::NativeConstruct();BuildSessionScreen();if(auto* S=Service())S->OnChanged.AddUniqueDynamic(this,&ThisClass::RefreshState);RefreshState();}
void UMultiplayerDlg::NativeDestruct(){if(auto* S=Service())S->OnChanged.RemoveDynamic(this,&ThisClass::RefreshState);Super::NativeDestruct();}
void UMultiplayerDlg::Show(){Super::Show();BuildSessionScreen();RefreshState();}
void UMultiplayerDlg::RefreshState(){
 if(!HostPanel||!JoinPanel)return;auto* S=Service();const bool Busy=S&&S->IsBusy();const bool Active=S&&S->HasSession();
 HostPanel->SetVisibility(bModeSelected&&bHostMode?ESlateVisibility::Visible:ESlateVisibility::Collapsed);JoinPanel->SetVisibility(bModeSelected&&!bHostMode?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 for (UButton* MissionButton : MissionButtons) MissionButton->SetIsEnabled(S&&!Busy&&!Active);SearchButton->SetIsEnabled(S&&!Busy&&!Active);LAN->SetIsEnabled(!Busy&&!Active);SessionName->SetIsEnabled(!Busy&&!Active);Capacity->SetIsEnabled(!Busy&&!Active);
 if(S && !S->Results.IsValidIndex(Selected))Selected=INDEX_NONE;
 JoinButton->SetIsEnabled(S&&!Busy&&!Active&&S->Results.IsValidIndex(Selected)&&S->Results[Selected].Players<S->Results[Selected].Capacity);
 LeaveButton->SetIsEnabled(!Busy);LeaveButton->SetVisibility(Active?ESlateVisibility::Visible:ESlateVisibility::Collapsed);CancelSearchButton->SetVisibility(S&&S->State==ESSWSessionState::Searching?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 if (HostTab && JoinTab)
 {
     StyleSessionRow(*HostTab, bHostMode);
     StyleSessionRow(*JoinTab, !bHostMode);
 }
 StatusLabel->SetText(FText::FromString(S?S->Status:TEXT("Session service unavailable.")));Rows->ClearChildren();
 if(!S||S->Results.IsEmpty())Rows->AddChildToVerticalBox(Label(TEXT("No games listed. Select Find games to refresh."),16))->SetPadding(FMargin(12,20));
 if(S)for(int I=0;I<S->Results.Num();++I){
  const auto& Entry = S->Results[I];
  const bool bFull = Entry.Players >= Entry.Capacity;
  auto* Button = WidgetTree->ConstructWidget<USSWSessionRowButton>();
  Button->Configure(this, I);
  auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
  auto* ButtonSlot = CastChecked<UButtonSlot>(Button->AddChild(Row));
  ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
  ButtonSlot->SetVerticalAlignment(VAlign_Fill);
  ButtonSlot->SetPadding(FMargin(SessionRowInset, 0));

  auto* Name = Label(Entry.Mission.IsEmpty() ? Entry.Name : FString::Printf(TEXT("%s  ·  %s"),
      *Entry.Name, *MultiplayerMissionLabel(Entry.Mission)), 14);
  Name->SetAutoWrapText(false);
  Name->SetClipping(EWidgetClipping::ClipToBounds);
  auto* NameSlot = Row->AddChildToHorizontalBox(Name);
  NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
  NameSlot->SetVerticalAlignment(VAlign_Center);
  NameSlot->SetPadding(FMargin(0, 0, 12, 0));

  auto* Players = Label(FString::Printf(TEXT("%d / %d%s"), Entry.Players, Entry.Capacity,
      bFull ? TEXT("  FULL") : TEXT("")), 14);
  auto* Ping = Label(FString::Printf(TEXT("%d ms"), Entry.Ping), 14);
  auto AddColumn = [&](UTextBlock* Caption, float Width)
  {
      auto* Cell = WidgetTree->ConstructWidget<USizeBox>();
      Cell->SetWidthOverride(Width);
      Cell->SetContent(Caption);
      Row->AddChildToHorizontalBox(Cell)->SetVerticalAlignment(VAlign_Center);
  };
  AddColumn(Players, PlayerColumnWidth);
  AddColumn(Ping, PingColumnWidth);

  StyleSessionRow(*Button, I == Selected);
  Button->SetIsEnabled(!Busy && !Active && !bFull);
  auto* Height = WidgetTree->ConstructWidget<USizeBox>();
  Height->SetHeightOverride(48);
  Height->SetContent(Button);
  Rows->AddChildToVerticalBox(Height)->SetPadding(FMargin(0, 4));
 }

}
void UMultiplayerDlg::SelectSession(int32 Index){Selected=Index;RefreshState();}
void UMultiplayerDlg::HostMode(){bModeSelected=true;bHostMode=true;RefreshState();}
void UMultiplayerDlg::JoinMode(){bModeSelected=true;bHostMode=false;RefreshState();}
void UMultiplayerDlg::Create(){HostMode();}

void UMultiplayerDlg::HostMission(int32 Index)
{
    if (auto* S = Service())
        S->CreateMissionSession(SessionName->GetText().ToString(), FMath::RoundToInt(Capacity->GetValue()),
            LAN->IsChecked(), Index);
}
void UMultiplayerDlg::Search(){Selected=INDEX_NONE;if(auto* S=Service())S->FindSessions(LAN->IsChecked());}
void UMultiplayerDlg::Join(){if(auto* S=Service())S->JoinSession(Selected);}
void UMultiplayerDlg::Leave(){if(auto* S=Service())S->LeaveSession();}
void UMultiplayerDlg::CancelSearch(){if(auto* S=Service())S->CancelSearch();}
void UMultiplayerDlg::Back(){if(LobbyRoster){Leave();return;}if(auto* S=Service()){if(S->IsBusy()){S->CancelSearch();return;}if(S->HasSession()){S->LeaveSession();return;}}RemoveFromParent();if(auto* Manager=GetMenuManager())Manager->ShowMenuDlg();}
void UMultiplayerDlg::HandleAccept()
{
    if (LobbyRoster)
    {
        ToggleReady();
        return;
    }
    if (bHostMode) Create();
    else Join();
}
void UMultiplayerDlg::HandleCancel(){Back();}

void UMultiplayerDlg::SubmitChat(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod != ETextCommit::OnEnter) return;
    if (auto* Controller = GetOwningPlayer<ASSWSessionLobbyController>())
    {
        Controller->SendSessionChat(Text.ToString());
        ChatEntry->SetText(FText::GetEmpty());
    }
}

void UMultiplayerDlg::RefreshChat()
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!State || !ChatLog) return;

    ChatLog->ClearChildren();
    for (const auto& Message : State->ChatHistory)
    {
        auto* Line = Label(FString::Printf(TEXT("%s: %s"), *Message.Sender, *Message.Text), 14);
        Line->SetAutoWrapText(true);
        ChatLog->AddChild(Line);
    }
    ChatLog->ScrollToEnd();
}

void UMultiplayerDlg::ToggleReady()
{
    if (auto* Controller = GetOwningPlayer<ASSWSessionLobbyController>())
        if (const auto* State = Controller->GetPlayerState<ASSWSessionPlayerState>())
            Controller->SetLobbyReady(!State->bReady);
}

void UMultiplayerDlg::RefreshRoster()
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!State || !LobbyRoster) return;

    LobbyRoster->ClearChildren();
    for (const APlayerState* Entry : State->PlayerArray)
        if (const auto* Player = Cast<ASSWSessionPlayerState>(Entry))
        {
            const FString Name = Player->GetPlayerName().IsEmpty() ? TEXT("Connecting pilot") : Player->GetPlayerName();
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* PilotName = Label(Name, 13);
            PilotName->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
            PilotName->SetToolTipText(FText::FromString(Name));
            auto* NameSlot = Row->AddChildToHorizontalBox(PilotName);
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            NameSlot->SetPadding(FMargin(0, 0, 8, 0));
            auto* Status = Label(Player->bReady ? TEXT("\u2713 Ready") : TEXT("Waiting"), 13);
            if (Player->bReady) Status->SetColorAndOpacity(FSlateColor(FLinearColor(0.53f, 1.0f, 0.75f)));
            Row->AddChildToHorizontalBox(Status);
            LobbyRoster->AddChild(Row);
        }

    const auto* Controller = GetOwningPlayer<ASSWSessionLobbyController>();
    const auto* Local = Controller ? Controller->GetPlayerState<ASSWSessionPlayerState>() : nullptr;
    if (ReadyLabel)
        ReadyLabel->SetText(FText::FromString(Local && Local->bReady
            ? TEXT("\u2715  CANCEL READY") : TEXT("\u2713  SET READY")));
}

void UMultiplayerDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!MissionCountdownLabel) return;

    auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!State) return;
    if (ObservedLobby.Get() != State)
    {
        if (auto* Previous = ObservedLobby.Get())
        {
            Previous->OnRosterChanged.RemoveAll(this);
            Previous->OnChatChanged.RemoveAll(this);
        }
        ObservedLobby = State;
        State->OnRosterChanged.AddUObject(this, &ThisClass::RefreshRoster);
        State->OnChatChanged.AddUObject(this, &ThisClass::RefreshChat);
        RefreshRoster();
        RefreshChat();
    }

    if (!State->MissionLaunchError.IsEmpty() && State->MissionStartDeadline <= 0.0)
    {
        if (DisplayedCountdown != INDEX_NONE - 1)
            MissionCountdownLabel->SetText(FText::FromString(State->MissionLaunchError));
        DisplayedCountdown = INDEX_NONE - 1;
        return;
    }

    const int32 Seconds = State->bMissionStartReady ? 0 :
        State->MissionStartDeadline > 0.0 ?
            FMath::Max(1, FMath::CeilToInt(State->MissionStartDeadline - State->GetServerWorldTimeSeconds())) :
            INDEX_NONE;
    if (Seconds == DisplayedCountdown) return;

    DisplayedCountdown = Seconds;
    if (Seconds == INDEX_NONE)
        MissionCountdownLabel->SetText(FText::FromString(TEXT("WAITING FOR PILOTS TO READY UP")));
    else if (Seconds == 0)
        MissionCountdownLabel->SetText(FText::FromString(TEXT("MISSION SIMULATION RUNNING")));
    else
        MissionCountdownLabel->SetText(FText::FromString(FString::Printf(TEXT("MISSION START IN %d"), Seconds)));
}

void UMultiplayerDlg::BuildLobbyScreen(UVerticalBox* Main, UOverlay* PlayField)
{
    auto* LobbyTitle = Label(TEXT("MISSION LOBBY"), 30);
    LobbyTitle->SetFont(MissionUIStyle::GetHeaderFont(38));
    Main->AddChildToVerticalBox(LobbyTitle)->SetPadding(FMargin(0, 0, 0, 8));
    MissionCountdownLabel = Label(TEXT("WAITING FOR PILOTS TO READY UP"), 28);
    MissionCountdownLabel->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 26));
    MissionCountdownLabel->SetShadowColorAndOpacity(FLinearColor::Black);
    MissionCountdownLabel->SetShadowOffset(FVector2D(1.0, 2.0));
    auto* Countdown = PlayField->AddChildToOverlay(MissionCountdownLabel);
    Countdown->SetHorizontalAlignment(HAlign_Center);
    Countdown->SetVerticalAlignment(VAlign_Center);

    auto* Spacer = WidgetTree->ConstructWidget<USizeBox>();
    Main->AddChildToVerticalBox(Spacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
    PanelSize->SetHeightOverride(180.0f);
    PanelSize->SetContent(Columns);
    Main->AddChildToVerticalBox(PanelSize);

    auto* PilotCard = WidgetTree->ConstructWidget<UBorder>();
    auto* PilotSlot = Columns->AddChildToHorizontalBox(PilotCard);
    FSlateChildSize PilotWidth(ESlateSizeRule::Fill);
    PilotWidth.Value = 0.38f;
    PilotSlot->SetSize(PilotWidth);
    PilotSlot->SetPadding(FMargin(0, 0, 12, 0));
    PilotCard->SetPadding(FMargin(16));
    PilotCard->SetBrushColor(FLinearColor(.02f, .055f, .08f, 0.85f));
    auto* Pilots = WidgetTree->ConstructWidget<UVerticalBox>();
    PilotCard->SetContent(Pilots);
    Pilots->AddChildToVerticalBox(Label(TEXT("PILOTS"), 18))->SetPadding(FMargin(0, 0, 0, 12));
    LobbyRoster = WidgetTree->ConstructWidget<UScrollBox>();
    Pilots->AddChildToVerticalBox(LobbyRoster)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* Actions = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* ActionSize = WidgetTree->ConstructWidget<USizeBox>();
    ActionSize->SetWidthOverride(200.0f);
    ActionSize->SetContent(Actions);
    auto* ActionSlot = Columns->AddChildToHorizontalBox(ActionSize);
    ActionSlot->SetVerticalAlignment(VAlign_Center);
    ActionSlot->SetPadding(FMargin(0, 0, 12, 0));
    auto* Ready = Button(TEXT("\u2713  SET READY"), Actions);
    Ready->SetToolTipText(FText::GetEmpty());
    Ready->SetClickMethod(EButtonClickMethod::MouseDown);
    ReadyLabel = Cast<UTextBlock>(Ready->GetChildAt(0));
    Ready->OnClicked.AddDynamic(this, &ThisClass::ToggleReady);
    AddMainMenuButton(Actions);

    auto* ChatCard = WidgetTree->ConstructWidget<UBorder>();
    auto* ChatSlot = Columns->AddChildToHorizontalBox(ChatCard);
    FSlateChildSize ChatWidth(ESlateSizeRule::Fill);
    ChatWidth.Value = 0.62f;
    ChatSlot->SetSize(ChatWidth);
    ChatCard->SetPadding(FMargin(16));
    ChatCard->SetBrushColor(FLinearColor(.02f, .055f, .08f, 0.85f));
    auto* Chat = WidgetTree->ConstructWidget<UVerticalBox>();
    ChatCard->SetContent(Chat);
    Chat->AddChildToVerticalBox(Label(TEXT("SESSION CHAT"), 18))->SetPadding(FMargin(0, 0, 0, 12));
    ChatLog = WidgetTree->ConstructWidget<UScrollBox>();
    Chat->AddChildToVerticalBox(ChatLog)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    ChatEntry = WidgetTree->ConstructWidget<UEditableTextBox>();
    FEditableTextBoxStyle Style = ChatEntry->GetWidgetStyle();
    FSlateBrush Background = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    Background.TintColor = FSlateColor(FLinearColor(.008f, .025f, .04f));
    Style.BackgroundImageNormal = Background;
    Style.BackgroundImageHovered = Background;
    Style.BackgroundImageFocused = Background;
    Style.SetForegroundColor(FSlateColor(FLinearColor(.9f, .96f, 1.f)));
    Style.SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 13));
    Style.Padding = FMargin(12, 10);
    ChatEntry->SetWidgetStyle(Style);
    ChatEntry->SetHintText(FText::FromString(TEXT("Type a message and press Enter")));
    ChatEntry->SetClearKeyboardFocusOnCommit(false);
    ChatEntry->OnTextCommitted.AddDynamic(this, &ThisClass::SubmitChat);
    Chat->AddChildToVerticalBox(ChatEntry)->SetPadding(FMargin(0, 12, 0, 0));

}

void UMultiplayerDlg::AddMainMenuButton(UVerticalBox* Parent)
{
    auto* ReturnButton = Button(TEXT("MAIN MENU"), Parent);
    ReturnButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Back);

    auto* Texture = LoadObject<UTexture2D>(nullptr,
        TEXT("/Game/SciFiUIComponents/Textures/Icons4/T_ICON_Menu.T_ICON_Menu"));
    if (!Texture) return;

    auto* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* Icon = WidgetTree->ConstructWidget<UImage>();
    FSlateBrush IconBrush;
    IconBrush.SetResourceObject(Texture);
    IconBrush.ImageSize = FVector2D(22.0, 22.0);
    Icon->SetBrush(IconBrush);
    auto* IconSlot = ContentRow->AddChildToHorizontalBox(Icon);
    IconSlot->SetVerticalAlignment(VAlign_Center);
    IconSlot->SetPadding(FMargin(0, 0, 10, 0));
    ContentRow->AddChildToHorizontalBox(Label(TEXT("MAIN MENU"), 16))->SetVerticalAlignment(VAlign_Center);
    ReturnButton->ClearChildren();
    ReturnButton->AddChild(ContentRow);
}
