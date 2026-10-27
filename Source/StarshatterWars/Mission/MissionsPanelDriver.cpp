#include "MissionsPanelDriver.h"
#include "MenuScreen.h"
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
#include "Engine/Texture2D.h"

void UMissionsPanelDriver::NativeConstruct()
{
    Super::NativeConstruct();
    BuildDriverLayout();
}

void UMissionsPanelDriver::BuildDriverLayout()
{
    if (!WidgetTree) return;
    // The root canvas may already be named MissionsDriver_Root.  Do NOT treat
    // its existence as evidence that the controls have been constructed.
    // Check a unique content widget instead.
    if (WidgetTree->FindWidget(TEXT("MissionsDriver_Contents"))) return;
    UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!Root) Root = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CanvasRoot")));
    if (!Root) {
        Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MissionsDriver_Root"));
        WidgetTree->RootWidget = Root;
    }
    UImage* Background = WidgetTree->ConstructWidget<UImage>();
    if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Panel.Panel"))) Background->SetBrushFromTexture(Tex, true);
    Background->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot* BgSlot = Root->AddChildToCanvas(Background);
    BgSlot->SetAnchors(FAnchors(0.f,0.f,1.f,1.f)); BgSlot->SetOffsets(FMargin(0)); BgSlot->SetZOrder(-1);
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionsDriver_Contents"));
    UCanvasPanelSlot* Main = Root->AddChildToCanvas(Column);
    Main->SetAnchors(FAnchors(0.f,0.f,1.f,1.f)); Main->SetOffsets(FMargin(38.f,25.f,38.f,25.f));
    auto Label = [this](const TCHAR* Value,int32 FontSize,FLinearColor Tint) -> UTextBlock* {
        UTextBlock* Txt=WidgetTree->ConstructWidget<UTextBlock>();
        Txt->SetText(FText::FromString(Value)); Txt->SetColorAndOpacity(FSlateColor(Tint));
        FSlateFontInfo Font=Txt->GetFont(); Font.Size=FontSize; Txt->SetFont(Font);return Txt;};
    Column->AddChildToVerticalBox(Label(TEXT("MISSIONS"),38,FLinearColor(0.95f,0.96f,0.98f)))->SetPadding(FMargin(0,0,0,10));
    Column->AddChildToVerticalBox(Label(TEXT("SELECT OPERATION TYPE"),18,FLinearColor(0.72f,0.79f,0.85f)))->SetPadding(FMargin(0,0,0,22));
    UHorizontalBox* Tabs=WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0,0,0,12));
    auto Tab = [this,Tabs,Label](const TCHAR* Caption)->UButton* {
        UButton* Button=WidgetTree->ConstructWidget<UButton>();
        Button->SetContent(Label(Caption,21,FLinearColor(0.94f,0.95f,0.97f)));
        UHorizontalBoxSlot* Slot=Tabs->AddChildToHorizontalBox(Button);
        FSlateChildSize Fill;Fill.SizeRule=ESlateSizeRule::Fill; Fill.Value=1.f;Slot->SetSize(Fill);
        Slot->SetPadding(FMargin(0,0,12,0));return Button;};
    Tab(TEXT("CAMPAIGN"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenCampaign);
    Tab(TEXT("SINGLE"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenSingle);
    Tab(TEXT("CUSTOM"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenCustom);
    Tab(TEXT("MULTIPLAYER"))->OnClicked.AddDynamic(this,&UMissionsPanelDriver::OpenMultiplayer);
    Column->AddChildToVerticalBox(Label(TEXT("Campaign operations  |  Standalone missions  |  Custom scenarios  |  Multiplayer operations"),16,FLinearColor(0.78f,0.82f,0.87f)));
    UButton* Return=WidgetTree->ConstructWidget<UButton>();
    Return->SetContent(Label(TEXT("RETURN TO MAIN MENU"),18,FLinearColor(0.90f,0.92f,0.95f)));
    Column->AddChildToVerticalBox(Return)->SetPadding(FMargin(0,45,0,0));
    Return->OnClicked.AddDynamic(this,&UMissionsPanelDriver::Back);
}
void UMissionsPanelDriver::Show()
{
    BuildDriverLayout();
    SetVisibility(ESlateVisibility::Visible);
}
void UMissionsPanelDriver::HandleCancel(){Back();}
void UMissionsPanelDriver::OpenCampaign(){if(Manager)Manager->ShowCampaignSelectDlg();}
void UMissionsPanelDriver::OpenSingle(){if(Manager)Manager->ShowSingleMissionDlg();}
void UMissionsPanelDriver::OpenCustom(){if(Manager)Manager->ShowCustomMissionDlg();}
void UMissionsPanelDriver::OpenMultiplayer(){if(Manager)Manager->ShowMultiplayerMissionDlg();}
void UMissionsPanelDriver::Back(){if(Manager)Manager->ShowMenuDlg();}
