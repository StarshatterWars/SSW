#include "MultiplayerDlg.h"
#include "SSWSessionSubsystem.h"
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
#include "Engine/GameInstance.h"
#include "MissionUIStyle.h"
#include "StarshatterUIStyleSubsystem.h"
#include "MenuScreen.h"
void USSWSessionRowButton::Configure(UMultiplayerDlg* Screen,int32 Row){Dialog=Screen;Index=Row;OnClicked.AddUniqueDynamic(this,&ThisClass::Clicked);}
void USSWSessionRowButton::Clicked(){if(Dialog.IsValid())Dialog->SelectSession(Index);}
UMultiplayerDlg::UMultiplayerDlg(const FObjectInitializer& I):Super(I){SetIsFocusable(true);}
USSWSessionSubsystem* UMultiplayerDlg::Service()const{return GetGameInstance()?GetGameInstance()->GetSubsystem<USSWSessionSubsystem>():nullptr;}
TSharedRef<SWidget> UMultiplayerDlg::RebuildWidget(){if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);BuildSessionScreen();return Super::RebuildWidget();}
UTextBlock* UMultiplayerDlg::Label(const FString& Text,int32 Size){auto* W=WidgetTree->ConstructWidget<UTextBlock>();W->SetText(FText::FromString(Text));W->SetFont(MissionUIStyle::GetHeaderFont(Size));W->SetColorAndOpacity(FSlateColor(FLinearColor(.7,.9,1)));W->SetAutoWrapText(true);return W;}
UButton* UMultiplayerDlg::Button(const FString& Text,UVerticalBox* Parent){auto* B=WidgetTree->ConstructWidget<UButton>();B->AddChild(Label(Text));B->SetToolTipText(FText::FromString(Text));MissionUIStyle::ApplyMenuButtonStyle(B,GetGameInstance());auto* ActionSlot=Parent->AddChildToVerticalBox(B);ActionSlot->SetPadding(FMargin(0,7));return B;}
void UMultiplayerDlg::BuildSessionScreen(){
 if(WidgetTree->RootWidget && WidgetTree->RootWidget->GetFName()==TEXT("SSWNativeSessionScreen"))return;
 auto* Frame=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("SSWNativeSessionScreen"));Frame->SetBrushColor(FLinearColor(.012,.025,.045,.98));Frame->SetPadding(FMargin(40));WidgetTree->RootWidget=Frame;
 auto* Main=WidgetTree->ConstructWidget<UVerticalBox>();Frame->SetContent(Main);Main->AddChildToVerticalBox(Label(TEXT("MULTIPLAYER"),32));Main->AddChildToVerticalBox(Label(TEXT("Create a game or find a session.")));
 auto* Modes=WidgetTree->ConstructWidget<UHorizontalBox>();Main->AddChildToVerticalBox(Modes);
 auto* Left=WidgetTree->ConstructWidget<UVerticalBox>();auto* Right=WidgetTree->ConstructWidget<UVerticalBox>();Modes->AddChildToHorizontalBox(Left)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Modes->AddChildToHorizontalBox(Right)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 Button(TEXT("CREATE SESSION"),Left)->OnClicked.AddUniqueDynamic(this,&ThisClass::HostMode);Button(TEXT("JOIN SESSION"),Right)->OnClicked.AddUniqueDynamic(this,&ThisClass::JoinMode);
 auto* Network=WidgetTree->ConstructWidget<UHorizontalBox>();Main->AddChildToVerticalBox(Network);LAN=WidgetTree->ConstructWidget<UCheckBox>();LAN->SetIsChecked(true);Network->AddChildToHorizontalBox(LAN);Network->AddChildToHorizontalBox(Label(TEXT("LAN game (local network)")));
 HostPanel=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(HostPanel);
 HostPanel->AddChildToVerticalBox(Label(TEXT("SESSION NAME")));SessionName=WidgetTree->ConstructWidget<UEditableTextBox>();SessionName->SetText(FText::FromString(TEXT("Starshatter Wars")));HostPanel->AddChildToVerticalBox(SessionName);
 HostPanel->AddChildToVerticalBox(Label(TEXT("PLAYER LIMIT")));Capacity=WidgetTree->ConstructWidget<USpinBox>();Capacity->SetMinValue(2);Capacity->SetMaxValue(16);Capacity->SetMinSliderValue(2);Capacity->SetMaxSliderValue(16);Capacity->SetDelta(1);Capacity->SetValue(8);HostPanel->AddChildToVerticalBox(Capacity);
 HostPanel->AddChildToVerticalBox(Label(TEXT("Space lobby")));CreateButton=Button(TEXT("CREATE GAME"),HostPanel);CreateButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Create);
 JoinPanel=WidgetTree->ConstructWidget<UVerticalBox>();Main->AddChildToVerticalBox(JoinPanel);SearchButton=Button(TEXT("FIND GAMES / REFRESH"),JoinPanel);SearchButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Search);
 auto* Bounds=WidgetTree->ConstructWidget<USizeBox>();Bounds->SetHeightOverride(300);JoinPanel->AddChildToVerticalBox(Bounds);auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Bounds->SetContent(Scroll);Rows=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Rows);
 JoinButton=Button(TEXT("JOIN SELECTED GAME"),JoinPanel);JoinButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Join);
 StatusLabel=Label(TEXT("Create a session or search for games."));Main->AddChildToVerticalBox(StatusLabel);
 CancelSearchButton=Button(TEXT("CANCEL SEARCH"),Main);CancelSearchButton->OnClicked.AddUniqueDynamic(this,&ThisClass::CancelSearch);
 LeaveButton=Button(TEXT("LEAVE SESSION"),Main);LeaveButton->OnClicked.AddUniqueDynamic(this,&ThisClass::Leave);
 Button(TEXT("BACK"),Main)->OnClicked.AddUniqueDynamic(this,&ThisClass::Back);RefreshState();
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
 StatusLabel->SetText(FText::FromString(S?S->Status:TEXT("Session service unavailable.")));Rows->ClearChildren();
 if(S)for(int I=0;I<S->Results.Num();++I){const auto& R=S->Results[I];auto* B=WidgetTree->ConstructWidget<USSWSessionRowButton>();B->Configure(this,I);MissionUIStyle::ApplyMenuButtonStyle(B,GetGameInstance());B->AddChild(Label(FString::Printf(TEXT("%s%s   %d/%d players   %d ms"),I==Selected?TEXT("> "):TEXT(""),*R.Name,R.Players,R.Capacity,R.Ping)));B->SetIsEnabled(!Busy&&!Active&&R.Players<R.Capacity);Rows->AddChildToVerticalBox(B)->SetPadding(FMargin(0,4));}
}
void UMultiplayerDlg::SelectSession(int32 Index){Selected=Index;RefreshState();}
void UMultiplayerDlg::HostMode(){bHostMode=true;RefreshState();}
void UMultiplayerDlg::JoinMode(){bHostMode=false;RefreshState();}
void UMultiplayerDlg::Create(){if(auto* S=Service())S->CreateSession(SessionName->GetText().ToString(),FMath::RoundToInt(Capacity->GetValue()),LAN->IsChecked());}
void UMultiplayerDlg::Search(){Selected=INDEX_NONE;if(auto* S=Service())S->FindSessions(LAN->IsChecked());}
void UMultiplayerDlg::Join(){if(auto* S=Service())S->JoinSession(Selected);}
void UMultiplayerDlg::Leave(){if(auto* S=Service())S->LeaveSession();}
void UMultiplayerDlg::CancelSearch(){if(auto* S=Service())S->CancelSearch();}
void UMultiplayerDlg::Back(){if(auto* S=Service()){if(S->IsBusy()){S->CancelSearch();return;}if(S->HasSession()){S->LeaveSession();return;}}RemoveFromParent();if(auto* Manager=GetMenuManager())Manager->ShowMissionSelectDlg();}
void UMultiplayerDlg::HandleAccept(){if(bHostMode)Create();else Join();}
void UMultiplayerDlg::HandleCancel(){Back();}
