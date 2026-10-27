#include "MissionsPanelDriver.h"
#include "MenuScreen.h"
#include "SingleMissionDlg.h"
#include "CustomMissionDlg.h"
#include "CampaignMissionDlg.h"
#include "MultiplayerDlg.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "MissionUIStyle.h"
#include "GameFramework/PlayerController.h"

TSharedRef<SWidget> UMissionsPanelDriver::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, UWidgetTree::StaticClass(), TEXT("WidgetTree"));
    BuildDriverLayout();
    return Super::RebuildWidget();
}
void UMissionsPanelDriver::NativeConstruct() { Super::NativeConstruct(); BuildDriverLayout(); }
void UMissionsPanelDriver::BuildDriverLayout()
{
    if (!WidgetTree || WidgetTree->FindWidget(TEXT("MissionsDriver_Contents"))) return;
    UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!Root) Root = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CanvasRoot")));
    if (!Root) { Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MissionsDriver_Root")); WidgetTree->RootWidget=Root; }
    UImage* Background=WidgetTree->ConstructWidget<UImage>();
    if (UTexture2D* Tex=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/Panel.Panel"))) Background->SetBrushFromTexture(Tex,true);
    Background->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot* Bg=Root->AddChildToCanvas(Background); Bg->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));Bg->SetOffsets(FMargin(0.f));Bg->SetZOrder(-1);
    UVerticalBox* Column=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("MissionsDriver_Contents"));
    UCanvasPanelSlot* Main=Root->AddChildToCanvas(Column);Main->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));Main->SetOffsets(FMargin(30.f,18.f,30.f,18.f));
    auto Label=[this](const TCHAR* Value,int32 Size,FLinearColor Color)->UTextBlock* {UTextBlock* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Value));Text->SetColorAndOpacity(FSlateColor(Color));FSlateFontInfo Font=Text->GetFont();Font.Size=Size;Text->SetFont(Font);return Text;};
    UBorder* Header=WidgetTree->ConstructWidget<UBorder>();Header->SetBrushColor(FLinearColor(0.105f,0.113f,0.125f,1.f));Header->SetPadding(FMargin(18.f,9.f));
    ModeTitle=Label(TEXT("MISSIONS"),38,FLinearColor(0.95f,0.97f,1.f));
    ModeTitle->SetFont(MissionUIStyle::GetHeaderFont(38));
    Header->SetContent(ModeTitle);
    Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f,0.f,0.f,0.f));
    UBorder* DarkHeaderRule=WidgetTree->ConstructWidget<UBorder>();
    DarkHeaderRule->SetBrushColor(FLinearColor(0.20f,0.21f,0.23f,1.f));
    DarkHeaderRule->SetPadding(FMargin(0.f));
    USizeBox* DarkHeaderStrip=WidgetTree->ConstructWidget<USizeBox>();
    DarkHeaderStrip->SetHeightOverride(5.f);
    DarkHeaderRule->SetContent(DarkHeaderStrip);
    Column->AddChildToVerticalBox(DarkHeaderRule)->SetPadding(FMargin(0.f,0.f,0.f,12.f));
    UHorizontalBox* Tabs=WidgetTree->ConstructWidget<UHorizontalBox>();Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f,0.f,0.f,12.f));
    auto Tab=[this,Tabs,Label](const TCHAR* Name)->UButton*{ UButton* B=WidgetTree->ConstructWidget<UButton>(); B->SetContent(Label(Name,20,FLinearColor::Black)); UHorizontalBoxSlot* S=Tabs->AddChildToHorizontalBox(B);FSlateChildSize F;F.SizeRule=ESlateSizeRule::Fill;F.Value=1.f;S->SetSize(F);S->SetPadding(FMargin(0.f,0.f,10.f,0.f)); return B; };
    Tab(TEXT("CAMPAIGN"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenCampaign);
    Tab(TEXT("SINGLE"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenSingle);
    Tab(TEXT("CUSTOM"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenCustom);
    Tab(TEXT("MULTIPLAYER"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenMultiplayer);
    ContentHost=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MissionsDriver_ContentHost"));
    UVerticalBoxSlot* S=Column->AddChildToVerticalBox(ContentHost); FSlateChildSize Fill;Fill.SizeRule=ESlateSizeRule::Fill;Fill.Value=1.f;S->SetSize(Fill);S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);
    UButton* Return=WidgetTree->ConstructWidget<UButton>();Return->SetContent(Label(TEXT("RETURN TO MAIN MENU"),16,FLinearColor::Black));Column->AddChildToVerticalBox(Return)->SetPadding(FMargin(0.f,8.f,0.f,0.f));Return->OnClicked.AddDynamic(this,&UMissionsPanelDriver::Back);
}
void UMissionsPanelDriver::HideChildHeader(UUserWidget* Child)
{
    if (!Child || !Child->WidgetTree) return;
    // The native Single/Custom/Multiplayer screens put their title/tabs in the
    // first VerticalBox child, inside Root -> Border -> ScaleBox -> SizeBox.
    UCanvasPanel* Root=Cast<UCanvasPanel>(Child->WidgetTree->RootWidget);
    if (!Root || Root->GetChildrenCount()==0) return;
    UBorder* Panel=Cast<UBorder>(Root->GetChildAt(0)); if (!Panel) return;
    UScaleBox* Scale=Cast<UScaleBox>(Panel->GetContent()); if (!Scale) return;
    USizeBox* Frame=Cast<USizeBox>(Scale->GetContent()); if (!Frame) return;
    UVerticalBox* Full=Cast<UVerticalBox>(Frame->GetContent()); if (!Full || !Full->GetChildrenCount()) return;
    if (UWidget* ChildHeader=Full->GetChildAt(0)) ChildHeader->SetVisibility(ESlateVisibility::Collapsed);
}
void UMissionsPanelDriver::SelectChild(UUserWidget* Child,const TCHAR* InModeTitle)
{
    BuildDriverLayout(); if (!ContentHost || !Child) {UE_LOG(LogTemp,Error,TEXT("[MissionsDriver] Missing host or child for %s"),InModeTitle);return;}
    if (ActiveChild && ActiveChild!=Child) {ActiveChild->SetVisibility(ESlateVisibility::Collapsed);ActiveChild->RemoveFromParent();}
    ActiveChild=Child;
    if (Child->GetParent()!=ContentHost) {
        Child->RemoveFromParent();UCanvasPanelSlot* ChildCanvasSlot=ContentHost->AddChildToCanvas(Child);
        ChildCanvasSlot->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));ChildCanvasSlot->SetOffsets(FMargin(0.f));
    }
    HideChildHeader(Child);
    Child->SetVisibility(ESlateVisibility::Visible);
    if (ModeTitle)ModeTitle->SetText(FText::FromString(InModeTitle));
    UE_LOG(LogTemp,Warning,TEXT("[MissionsDriver] Embedded %s class=%s"),InModeTitle,*GetNameSafe(Child->GetClass()));
}
void UMissionsPanelDriver::ShowCampaignMode()
{
    APlayerController* PC=GetOwningPlayer(); if (!PC)return;
    if (!CampaignChild) {UCampaignMissionDlg* W=CreateWidget<UCampaignMissionDlg>(PC,UCampaignMissionDlg::StaticClass());CampaignChild=W;if (W){W->SetMenuManager(Manager);W->InitializeDlg(Manager);}}
    if (UCampaignMissionDlg* W=Cast<UCampaignMissionDlg>(CampaignChild)){ W->Show();SelectChild(W,TEXT("CAMPAIGN MISSIONS")); }
}
void UMissionsPanelDriver::ShowSingleMode()
{
    APlayerController* PC=GetOwningPlayer(); if (!PC)return;
    if (!SingleChild) {USingleMissionDlg* W=CreateWidget<USingleMissionDlg>(PC,USingleMissionDlg::StaticClass());SingleChild=W;if (W){W->SetMenuManager(Manager);W->InitializeDlg(Manager);}}
    if (USingleMissionDlg* W=Cast<USingleMissionDlg>(SingleChild)){W->Show();SelectChild(W,TEXT("SINGLE MISSIONS"));}
}
void UMissionsPanelDriver::ShowCustomMode()
{
    APlayerController* PC=GetOwningPlayer(); if (!PC)return;
    if (!CustomChild) {UCustomMissionDlg* W=CreateWidget<UCustomMissionDlg>(PC,UCustomMissionDlg::StaticClass());CustomChild=W;if (W){W->SetMenuManager(Manager);W->InitializeDlg(Manager);}}
    if (UCustomMissionDlg* W=Cast<UCustomMissionDlg>(CustomChild)){W->Show();SelectChild(W,TEXT("CUSTOM MISSIONS"));}
}
void UMissionsPanelDriver::ShowMultiplayerMode()
{
    APlayerController* PC=GetOwningPlayer(); if (!PC)return;
    if (!MultiplayerChild) {UMultiplayerDlg* W=CreateWidget<UMultiplayerDlg>(PC,UMultiplayerDlg::StaticClass());MultiplayerChild=W;if (W){W->SetMenuManager(Manager);W->InitializeDlg(Manager);}}
    if (UMultiplayerDlg* W=Cast<UMultiplayerDlg>(MultiplayerChild)){W->Show();SelectChild(W,TEXT("MULTIPLAYER MISSIONS"));}
}
void UMissionsPanelDriver::Show()
{
    BuildDriverLayout();SetVisibility(ESlateVisibility::Visible);
    if (!ActiveChild) ShowSingleMode();
}
void UMissionsPanelDriver::HandleCancel(){Back();}
void UMissionsPanelDriver::OpenCampaign(){ShowCampaignMode();}
void UMissionsPanelDriver::OpenSingle(){ShowSingleMode();}
void UMissionsPanelDriver::OpenCustom(){ShowCustomMode();}
void UMissionsPanelDriver::OpenMultiplayer(){ShowMultiplayerMode();}
void UMissionsPanelDriver::Back(){if(Manager)Manager->ShowMenuDlg();}
