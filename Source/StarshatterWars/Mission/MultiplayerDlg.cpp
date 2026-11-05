#include "MultiplayerDlg.h"
#include "SSWSessionSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/Spacer.h"
#include "Styling/CoreStyle.h"
#include "Engine/GameInstance.h"
#include "MissionUIStyle.h"
#include "StarshatterUIStyleSubsystem.h"
#include "MenuScreen.h"
void USSWSessionRowButton::Configure(UMultiplayerDlg* Screen,int32 Row){Dialog=Screen;Index=Row;OnClicked.AddUniqueDynamic(this,&ThisClass::Clicked);}
void USSWSessionRowButton::Clicked(){if(Dialog.IsValid())Dialog->SelectSession(Index);}
UMultiplayerDlg::UMultiplayerDlg(const FObjectInitializer& I):Super(I){SetIsFocusable(true);}
USSWSessionSubsystem* UMultiplayerDlg::Service()const{return GetGameInstance()?GetGameInstance()->GetSubsystem<USSWSessionSubsystem>():nullptr;}
TSharedRef<SWidget> UMultiplayerDlg::RebuildWidget(){if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);BuildSessionScreen();return Super::RebuildWidget();}
UTextBlock* UMultiplayerDlg::Label(const FString& Text,int32 Size)
{
    auto* W=WidgetTree->ConstructWidget<UTextBlock>();
    W->SetText(FText::FromString(Text));W->SetFont(MissionUIStyle::GetRowFont(Size));
    W->SetColorAndOpacity(FSlateColor(FLinearColor(.78f,.9f,.96f)));W->SetAutoWrapText(false);
    return W;
}
UButton* UMultiplayerDlg::Button(const FString& Text,UVerticalBox* Parent)
{
    auto* B=WidgetTree->ConstructWidget<UButton>();B->AddChild(Label(Text,16));
    B->SetToolTipText(FText::FromString(Text));MissionUIStyle::ApplyActionButtonStyle(B,TEXT("Cyan"));
    auto* Height=WidgetTree->ConstructWidget<USizeBox>();Height->SetHeightOverride(48);Height->SetContent(B);
    Parent->AddChildToVerticalBox(Height)->SetPadding(FMargin(0,6));return B;
}
void UMultiplayerDlg::BuildSessionScreen()
{
    if(WidgetTree->RootWidget && WidgetTree->RootWidget->GetFName()==TEXT("SSWNativeSessionScreen"))return;
    auto* Frame=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("SSWNativeSessionScreen"));
    Frame->SetBrushColor(FLinearColor(.008f,.017f,.03f,.98f));Frame->SetPadding(FMargin(28));WidgetTree->RootWidget=Frame;
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);Frame->SetContent(Scale);
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1080);Size->SetHeightOverride(740);Scale->SetContent(Size);
    auto* Main=WidgetTree->ConstructWidget<UVerticalBox>();Size->SetContent(Main);
    auto* Header=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(Header)->SetPadding(FMargin(0,0,0,18));
    auto* ScreenTitle=Label(TEXT("MULTIPLAYER"),30);ScreenTitle->SetFont(MissionUIStyle::GetHeaderFont(30));Header->AddChildToVerticalBox(ScreenTitle);
    Header->AddChildToVerticalBox(Label(TEXT("Create a game or find a session."),16))->SetPadding(FMargin(0,8,0,0));
    auto* Modes=WidgetTree->ConstructWidget<UHorizontalBox>();Main->AddChildToVerticalBox(Modes)->SetPadding(FMargin(0,0,0,12));
    auto* Left=WidgetTree->ConstructWidget<UVerticalBox>();auto* Right=WidgetTree->ConstructWidget<UVerticalBox>();
    auto* LeftSlot=Modes->AddChildToHorizontalBox(Left);LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));LeftSlot->SetPadding(FMargin(0,0,8,0));
    auto* RightSlot=Modes->AddChildToHorizontalBox(Right);RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));RightSlot->SetPadding(FMargin(8,0,0,0));
    HostTab=Button(TEXT("CREATE SESSION"),Left);HostTab->OnClicked.AddUniqueDynamic(this,&ThisClass::HostMode);
    JoinTab=Button(TEXT("JOIN SESSION"),Right);JoinTab->OnClicked.AddUniqueDynamic(this,&ThisClass::JoinMode);
    auto* Network=WidgetTree->ConstructWidget<UHorizontalBox>();Main->AddChildToVerticalBox(Network)->SetPadding(FMargin(0,0,0,16));
    LAN=WidgetTree->ConstructWidget<UCheckBox>();LAN->SetIsChecked(true);Network->AddChildToHorizontalBox(LAN)->SetVerticalAlignment(VAlign_Center);
    Network->AddChildToHorizontalBox(Label(TEXT("LAN game (local network)"),16))->SetPadding(FMargin(10,0,0,0));
    HostPanel=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(HostPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* HostCard=WidgetTree->ConstructWidget<UBorder>();HostCard->SetBrushColor(FLinearColor(.02f,.055f,.08f));HostCard->SetPadding(FMargin(24));
    HostPanel->AddChildToVerticalBox(HostCard)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Form=WidgetTree->ConstructWidget<UVerticalBox>();HostCard->SetContent(Form);
    Form->AddChildToVerticalBox(Label(TEXT("SESSION SETTINGS"),20))->SetPadding(FMargin(0,0,0,22));
    Form->AddChildToVerticalBox(Label(TEXT("SESSION NAME"),14))->SetPadding(FMargin(0,0,0,8));
    SessionName=WidgetTree->ConstructWidget<UEditableTextBox>();SessionName->SetText(FText::FromString(TEXT("Starshatter Wars")));
    FEditableTextBoxStyle InputStyle=SessionName->GetWidgetStyle();
    FSlateBrush InputBrush=*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));InputBrush.TintColor=FSlateColor(FLinearColor(.008f,.025f,.04f));
    InputStyle.BackgroundImageNormal=InputBrush;InputStyle.BackgroundImageHovered=InputBrush;InputStyle.BackgroundImageFocused=InputBrush;InputStyle.BackgroundImageReadOnly=InputBrush;
    InputStyle.SetForegroundColor(FSlateColor(FLinearColor(.9f,.96f,1.f)));InputStyle.SetFont(MissionUIStyle::GetRowFont(18));InputStyle.Padding=FMargin(12,8);
    SessionName->SetWidgetStyle(InputStyle);Form->AddChildToVerticalBox(SessionName)->SetPadding(FMargin(0,0,0,24));
    Form->AddChildToVerticalBox(Label(TEXT("PLAYER LIMIT"),14))->SetPadding(FMargin(0,0,0,8));
    auto* CapacitySize=WidgetTree->ConstructWidget<USizeBox>();CapacitySize->SetWidthOverride(160);CapacitySize->SetHeightOverride(44);
    Form->AddChildToVerticalBox(CapacitySize)->SetHorizontalAlignment(HAlign_Left);
    Capacity=WidgetTree->ConstructWidget<USpinBox>();Capacity->SetMinValue(2);Capacity->SetMaxValue(16);Capacity->SetMinSliderValue(2);Capacity->SetMaxSliderValue(16);Capacity->SetDelta(1);Capacity->SetValue(8);
    Capacity->SetMinFractionalDigits(0);Capacity->SetMaxFractionalDigits(0);Capacity->SetFont(MissionUIStyle::GetRowFont(18));Capacity->SetForegroundColor(FSlateColor(FLinearColor::White));
    FSpinBoxStyle SpinStyle=Capacity->GetWidgetStyle();SpinStyle.BackgroundBrush=InputBrush;SpinStyle.HoveredBackgroundBrush=InputBrush;SpinStyle.InactiveFillBrush=InputBrush;SpinStyle.ActiveFillBrush=InputBrush;Capacity->SetWidgetStyle(SpinStyle);CapacitySize->SetContent(Capacity);
    if (const auto* GI = GetGameInstance())
    {
        if (const auto* Data = GI->GetSubsystem<UStarshatterGameDataSubsystem>())
        {
            const auto& Missions = Data->GetMultiplayerMissionScenarios();
            if (!Missions.IsEmpty())
            {
                Form->AddChildToVerticalBox(Label(TEXT("MULTIPLAYER MISSION CATALOG"),14))->SetPadding(FMargin(0,16,0,8));
                auto* Catalog = WidgetTree->ConstructWidget<UScrollBox>();
                Form->AddChildToVerticalBox(Catalog)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                for (const auto& Mission : Missions)
                    Catalog->AddChild(Label(Mission.MissionName,16));
            }
        }
    }
    CreateButton=Button(TEXT("CREATE GAME"),HostPanel);CreateButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Create);
    JoinPanel=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(JoinPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    SearchButton=Button(TEXT("FIND GAMES / REFRESH"),JoinPanel);SearchButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Search);
    auto* Columns=WidgetTree->ConstructWidget<UHorizontalBox>();JoinPanel->AddChildToVerticalBox(Columns)->SetPadding(FMargin(16,12,16,8));
    Columns->AddChildToHorizontalBox(Label(TEXT("SESSION"),14))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    for(const TCHAR* Caption:{TEXT("PLAYERS"),TEXT("PING")}){auto* Cell=WidgetTree->ConstructWidget<USizeBox>();Cell->SetWidthOverride(120);Cell->SetContent(Label(Caption,14));Columns->AddChildToHorizontalBox(Cell);}
    auto* Browser=WidgetTree->ConstructWidget<UBorder>();Browser->SetBrushColor(FLinearColor(.02f,.055f,.08f));Browser->SetPadding(FMargin(10));
    JoinPanel->AddChildToVerticalBox(Browser)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Browser->SetContent(Scroll);
    Rows=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Rows);
    JoinButton=Button(TEXT("JOIN SELECTED GAME"),JoinPanel);JoinButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Join);
    auto* StatusBar=WidgetTree->ConstructWidget<UBorder>();StatusBar->SetPadding(FMargin(12));StatusBar->SetBrushColor(FLinearColor(.012f,.033f,.05f));
    Main->AddChildToVerticalBox(StatusBar)->SetPadding(FMargin(0,12,0,8));StatusLabel=Label(TEXT("Create a session or search for games."),14);StatusLabel->SetAutoWrapText(true);StatusBar->SetContent(StatusLabel);
    auto* Footer=WidgetTree->ConstructWidget<UHorizontalBox>();Main->AddChildToVerticalBox(Footer);
    auto FooterGroup=[&](){auto* Group=WidgetTree->ConstructWidget<UVerticalBox>();auto* FooterSlot=Footer->AddChildToHorizontalBox(Group);FooterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));FooterSlot->SetPadding(FMargin(6,0));return Group;};
    CancelSearchButton=Button(TEXT("CANCEL SEARCH"),FooterGroup());CancelSearchButton->OnClicked.AddUniqueDynamic(this,&ThisClass::CancelSearch);
    LeaveButton=Button(TEXT("LEAVE SESSION"),FooterGroup());LeaveButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Leave);
    Button(TEXT("BACK"),FooterGroup())->OnClicked.AddUniqueDynamic(this,&ThisClass::Back);RefreshState();
}
void UMultiplayerDlg::NativeConstruct(){Super::NativeConstruct();BuildSessionScreen();if(auto* S=Service())S->OnChanged.AddUniqueDynamic(this,&ThisClass::RefreshState);RefreshState();}
void UMultiplayerDlg::NativeDestruct(){if(auto* S=Service())S->OnChanged.RemoveDynamic(this,&ThisClass::RefreshState);Super::NativeDestruct();}
void UMultiplayerDlg::Show(){Super::Show();BuildSessionScreen();RefreshState();}
void UMultiplayerDlg::RefreshState(){
 if(!HostPanel||!JoinPanel)return;auto* S=Service();const bool Busy=S&&S->IsBusy();const bool Active=S&&S->HasSession();
 HostPanel->SetVisibility(bHostMode?ESlateVisibility::Visible:ESlateVisibility::Collapsed);JoinPanel->SetVisibility(bHostMode?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
 CreateButton->SetIsEnabled(S&&!Busy&&!Active);SearchButton->SetIsEnabled(S&&!Busy&&!Active);LAN->SetIsEnabled(!Busy&&!Active);SessionName->SetIsEnabled(!Busy&&!Active);Capacity->SetIsEnabled(!Busy&&!Active);
 if(S && !S->Results.IsValidIndex(Selected))Selected=INDEX_NONE;
 JoinButton->SetIsEnabled(S&&!Busy&&!Active&&S->Results.IsValidIndex(Selected)&&S->Results[Selected].Players<S->Results[Selected].Capacity);
 LeaveButton->SetIsEnabled(!Busy);LeaveButton->SetVisibility(Active?ESlateVisibility::Visible:ESlateVisibility::Collapsed);CancelSearchButton->SetVisibility(S&&S->State==ESSWSessionState::Searching?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 if(HostTab&&JoinTab){
  for(UButton* Tab:{HostTab.Get(),JoinTab.Get()}){MissionUIStyle::ApplyActionButtonStyle(Tab,TEXT("Cyan"));if((Tab==HostTab&&bHostMode)||(Tab==JoinTab&&!bHostMode)){FButtonStyle Style=Tab->GetStyle();Style.Normal=Style.Pressed;Tab->SetStyle(Style);}}
 }
 StatusLabel->SetText(FText::FromString(S?S->Status:TEXT("Session service unavailable.")));Rows->ClearChildren();
 if(!S||S->Results.IsEmpty())Rows->AddChildToVerticalBox(Label(TEXT("No games listed. Select Find games to refresh."),16))->SetPadding(FMargin(12,20));
 if(S)for(int I=0;I<S->Results.Num();++I){
  const auto& R=S->Results[I];auto* B=WidgetTree->ConstructWidget<USSWSessionRowButton>();B->Configure(this,I);
  auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();B->AddChild(Row);auto* Name=Label(R.Name,16);Name->SetAutoWrapText(false);Name->SetClipping(EWidgetClipping::ClipToBounds);
  auto* NameSlot=Row->AddChildToHorizontalBox(Name);NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));NameSlot->SetPadding(FMargin(16,10));
  for(const FString& Text:{FString::Printf(TEXT("%d / %d"),R.Players,R.Capacity),FString::Printf(TEXT("%d ms"),R.Ping)}){auto* Cell=WidgetTree->ConstructWidget<USizeBox>();Cell->SetWidthOverride(120);Cell->SetContent(Label(Text,16));Row->AddChildToHorizontalBox(Cell)->SetVerticalAlignment(VAlign_Center);}
  MissionUIStyle::ApplyActionButtonStyle(B,TEXT("Cyan"));if(I==Selected){FButtonStyle Style=B->GetStyle();Style.Normal=Style.Pressed;B->SetStyle(Style);}
  B->SetIsEnabled(!Busy&&!Active&&R.Players<R.Capacity);auto* Height=WidgetTree->ConstructWidget<USizeBox>();Height->SetHeightOverride(48);Height->SetContent(B);Rows->AddChildToVerticalBox(Height)->SetPadding(FMargin(0,4));
 }

}
void UMultiplayerDlg::SelectSession(int32 Index){Selected=Index;RefreshState();}
void UMultiplayerDlg::HostMode(){bHostMode=true;RefreshState();}
void UMultiplayerDlg::JoinMode(){bHostMode=false;RefreshState();}
void UMultiplayerDlg::Create(){if(auto* S=Service())S->CreateSession(SessionName->GetText().ToString(),FMath::RoundToInt(Capacity->GetValue()),LAN->IsChecked());}
void UMultiplayerDlg::Search(){Selected=INDEX_NONE;if(auto* S=Service())S->FindSessions(LAN->IsChecked());}
void UMultiplayerDlg::Join(){if(auto* S=Service())S->JoinSession(Selected);}
void UMultiplayerDlg::Leave(){if(auto* S=Service())S->LeaveSession();}
void UMultiplayerDlg::CancelSearch(){if(auto* S=Service())S->CancelSearch();}
void UMultiplayerDlg::Back(){if(auto* S=Service()){if(S->IsBusy()){S->CancelSearch();return;}if(S->HasSession()){S->LeaveSession();return;}}RemoveFromParent();if(auto* Manager=GetMenuManager())Manager->ShowMenuDlg();}
void UMultiplayerDlg::HandleAccept(){if(bHostMode)Create();else Join();}
void UMultiplayerDlg::HandleCancel(){Back();}
