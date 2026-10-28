#include "MissionsPanelDriver.h"
#include "MenuScreen.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
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
#include "Components/ButtonSlot.h"
#include "Styling/CoreStyle.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "MissionUIStyle.h"
#include "GameFramework/PlayerController.h"
#include "Campaign.h"
#include "CombatGroup.h"
#include "TimerSubsystem.h"
#include "CampaignSave.h"
#include "SSWGameInstance.h"
#include "Misc/DateTime.h"

TSharedRef<SWidget> UMissionsPanelDriver::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, UWidgetTree::StaticClass(), TEXT("WidgetTree"));
    BuildDriverLayout();
    return Super::RebuildWidget();
}
void UMissionsPanelDriver::NativeConstruct()
{
    Super::NativeConstruct();
    BuildDriverLayout();
    BindHeaderClock();
    RefreshHeaderStatus();
}
void UMissionsPanelDriver::BindHeaderClock()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;
    USSWGameInstance* SSW = Cast<USSWGameInstance>(GI);
    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    if (!Timer) return;
    if (Timer->UniverseBaseUnixSeconds <= 0 && SSW)
    {
        SSW->LoadOrCreateUniverse();
        Timer = GI->GetSubsystem<UTimerSubsystem>();
        if (!Timer) return;
    }
    if (SSW && SSW->CampaignSave && !Timer->HasCampaignSave())
        Timer->SetCampaignSave(SSW->CampaignSave);
    if (!bHeaderClockBound)
    {
        Timer->OnUniverseSecond.AddUObject(this, &UMissionsPanelDriver::HandleHeaderClock);
        Timer->OnCampaignTPlusChanged.AddUObject(this, &UMissionsPanelDriver::HandleCampaignClock);
        bHeaderClockBound = true;
    }
    HandleHeaderClock(Timer->GetUniverseTimeSeconds());
}
void UMissionsPanelDriver::HandleHeaderClock(uint64 UniverseSecondsNow)
{
    RefreshHeaderStatus();
}
void UMissionsPanelDriver::HandleCampaignClock(uint64 UniverseSecondsNow, uint64 TPlusSeconds)
{
    RefreshHeaderStatus();
}
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
    UBorder* StatusBar = WidgetTree->ConstructWidget<UBorder>();
    StatusBar->SetBrushColor(FLinearColor(0.07f, 0.08f, 0.09f, 1.f));
    StatusBar->SetPadding(FMargin(12.f, 8.f));
    UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto AddStatus = [this, StatusRow](const TCHAR* Caption, TObjectPtr<UTextBlock>& Value)
    {
        UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>();
        UTextBlock* CaptionText = WidgetTree->ConstructWidget<UTextBlock>();
        CaptionText->SetText(FText::FromString(Caption));
        CaptionText->SetFont(MissionUIStyle::GetInfoLabelFont(12));
        CaptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.16f, 0.77f, 0.91f, 1.f)));
        Cell->AddChildToVerticalBox(CaptionText);
        Value = WidgetTree->ConstructWidget<UTextBlock>();
        Value->SetText(FText::FromString(TEXT("--")));
        Value->SetFont(MissionUIStyle::GetInfoValueFont(16));
        Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f, 1.f)));
        if (UVerticalBoxSlot* ValueSlot = Cell->AddChildToVerticalBox(Value))
            ValueSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
        if (UHorizontalBoxSlot* CellSlot = StatusRow->AddChildToHorizontalBox(Cell))
        {
            CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CellSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
        }
    };
    AddStatus(TEXT("TIME"), CurrentTimeText);
    AddStatus(TEXT("CAMPAIGN"), CampaignTimeText);
    AddStatus(TEXT("PILOT"), PlayerNameText);
    AddStatus(TEXT("UNIT"), PlayerGroupText);
    StatusBar->SetContent(StatusRow);
    Column->AddChildToVerticalBox(StatusBar)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    UBorder* DarkHeaderRule=WidgetTree->ConstructWidget<UBorder>();
    DarkHeaderRule->SetBrushColor(FLinearColor(0.20f,0.21f,0.23f,1.f));
    DarkHeaderRule->SetPadding(FMargin(0.f));
    USizeBox* DarkHeaderStrip=WidgetTree->ConstructWidget<USizeBox>();
    DarkHeaderStrip->SetHeightOverride(5.f);
    DarkHeaderRule->SetContent(DarkHeaderStrip);
    Column->AddChildToVerticalBox(DarkHeaderRule)->SetPadding(FMargin(0.f,0.f,0.f,12.f));
    // MENU on the left, four modes, BRIEFING on the right.
    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
    auto MakeIconTab = [this, Tabs](const TCHAR* IconPath)->UButton*
    {
        UButton* NewButton = WidgetTree->ConstructWidget<UButton>();
        USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(40.f);
        IconBox->SetHeightOverride(40.f);
        UImage* Icon = WidgetTree->ConstructWidget<UImage>();
        if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, IconPath))
            Icon->SetBrushFromTexture(Tex, true);
        else
            UE_LOG(LogTemp, Warning, TEXT("[MissionsDriver] Icon missing: %s"), IconPath);
        IconBox->SetContent(Icon);
        NewButton->SetContent(IconBox);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(IconBox->Slot))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Center);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
            ContentSlot->SetPadding(FMargin(4.f));
        }
        return NewButton;
    };
    MenuTab = MakeIconTab(TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Menu_256x256.T_Menu_256x256"));
    UHorizontalBoxSlot* MenuSlot = Tabs->AddChildToHorizontalBox(MenuTab);
    MenuSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    MenuSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
    MenuSlot->SetVerticalAlignment(VAlign_Center);
    auto MakeTab = [this, Tabs](const TCHAR* Name)->UButton*
    {
        UButton* NewButton = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
        Caption->SetText(FText::FromString(Name));
        Caption->SetFont(MissionUIStyle::GetHeaderFont(22));
        Caption->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Caption->SetJustification(ETextJustify::Center);
        Caption->SetAutoWrapText(false);
        NewButton->SetContent(Caption);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Caption->Slot))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Center);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
            ContentSlot->SetPadding(FMargin(10.f, 4.f));
        }
        UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(NewButton);
        TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TabSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
        return NewButton;
    };
    CampaignTab = MakeTab(TEXT("CAMPAIGN"));
    SingleTab = MakeTab(TEXT("SINGLE"));
    CustomTab = MakeTab(TEXT("CUSTOM"));
    MultiplayerTab = MakeTab(TEXT("MULTIPLAYER"));
    BriefingTab = MakeIconTab(TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Briefing_256x256.T_Briefing_256x256"));
    BriefingTab->SetIsEnabled(false);
    UHorizontalBoxSlot* BriefingSlot = Tabs->AddChildToHorizontalBox(BriefingTab);
    BriefingSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    BriefingSlot->SetPadding(FMargin(0.f));
    BriefingSlot->SetVerticalAlignment(VAlign_Center);
    CampaignTab->OnClicked.AddUniqueDynamic(this, &UMissionsPanelDriver::OpenCampaign);
    SingleTab->OnClicked.AddUniqueDynamic(this, &UMissionsPanelDriver::OpenSingle);
    CustomTab->OnClicked.AddUniqueDynamic(this, &UMissionsPanelDriver::OpenCustom);
    MultiplayerTab->OnClicked.AddUniqueDynamic(this, &UMissionsPanelDriver::OpenMultiplayer);
    MenuTab->OnClicked.AddUniqueDynamic(this, &UMissionsPanelDriver::Back);
    BriefingTab->OnClicked.AddUniqueDynamic(this, &UMissionsPanelDriver::OpenBriefing);
    RefreshTabStyles();
    ContentHost=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MissionsDriver_ContentHost"));
    UVerticalBoxSlot* S=Column->AddChildToVerticalBox(ContentHost); FSlateChildSize Fill;Fill.SizeRule=ESlateSizeRule::Fill;Fill.Value=1.f;S->SetSize(Fill);S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);

}
void UMissionsPanelDriver::RefreshTabStyles()
{
    // Active tab is blue, inactive tabs cyan, MENU always red.
    const TCHAR* BlueN = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Blue_n.T_Btn01Blue_n");
    const TCHAR* BlueP = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Blue_p.T_Btn01Blue_p");
    const TCHAR* CyanN = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Cyan_n.T_Btn01Cyan_n");
    const TCHAR* CyanP = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Cyan_p.T_Btn01Cyan_p");
    const TCHAR* RedN = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Red_n.T_Btn01Red_n");
    const TCHAR* RedP = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Red_p.T_Btn01Red_p");
    const TCHAR* GreenN = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Green_n.T_Btn01Green_n");
    const TCHAR* GreenP = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Green_p.T_Btn01Green_p");

    auto StyleTab = [](UButton* Button, const TCHAR* NormalPath, const TCHAR* PressedPath)
    {
        if (!Button) return;
        UTexture2D* NormalTexture = LoadObject<UTexture2D>(nullptr, NormalPath);
        UTexture2D* PressedTexture = LoadObject<UTexture2D>(nullptr, PressedPath);
        if (!NormalTexture || !PressedTexture)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MissionsDriver] Tab texture missing: %s or %s"), NormalPath, PressedPath);
        }
        FButtonStyle Style = Button->GetStyle();
        if (NormalTexture)
        {
            Style.Normal.SetResourceObject(NormalTexture);
            Style.Normal.DrawAs = ESlateBrushDrawType::Image;
            Style.Normal.TintColor = FSlateColor(FLinearColor::White);
        }
        if (PressedTexture)
        {
            Style.Hovered.SetResourceObject(PressedTexture);
            Style.Pressed.SetResourceObject(PressedTexture);
            Style.Hovered.DrawAs = ESlateBrushDrawType::Image;
            Style.Pressed.DrawAs = ESlateBrushDrawType::Image;
            Style.Hovered.TintColor = FSlateColor(FLinearColor::White);
            Style.Pressed.TintColor = FSlateColor(FLinearColor::White);
        }
        Style.NormalPadding = FMargin(0.f);
        Style.PressedPadding = FMargin(0.f);
        Button->SetStyle(Style);
    };
    StyleTab(CampaignTab, ActiveMode == EMissionsNavigationMode::Campaign ? BlueN : CyanN,
        ActiveMode == EMissionsNavigationMode::Campaign ? BlueP : CyanP);
    StyleTab(SingleTab, ActiveMode == EMissionsNavigationMode::Single ? BlueN : CyanN,
        ActiveMode == EMissionsNavigationMode::Single ? BlueP : CyanP);
    StyleTab(CustomTab, ActiveMode == EMissionsNavigationMode::Custom ? BlueN : CyanN,
        ActiveMode == EMissionsNavigationMode::Custom ? BlueP : CyanP);
    StyleTab(MultiplayerTab, ActiveMode == EMissionsNavigationMode::Multiplayer ? BlueN : CyanN,
        ActiveMode == EMissionsNavigationMode::Multiplayer ? BlueP : CyanP);
    StyleTab(MenuTab, RedN, RedP);
    StyleTab(BriefingTab, GreenN, GreenP);
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
    if (Child == CampaignChild) ActiveMode = EMissionsNavigationMode::Campaign;
    else if (Child == SingleChild) ActiveMode = EMissionsNavigationMode::Single;
    else if (Child == CustomChild) ActiveMode = EMissionsNavigationMode::Custom;
    else if (Child == MultiplayerChild) ActiveMode = EMissionsNavigationMode::Multiplayer;
    RefreshTabStyles();
    UpdateBriefingEnabled();
    UE_LOG(LogTemp,Warning,TEXT("[MissionsDriver] Embedded %s class=%s"),InModeTitle,*GetNameSafe(Child->GetClass()));
}
void UMissionsPanelDriver::ShowCampaignMode()
{
    // Resolve the active player-save campaign display name on each visit.
    // No campaign selection or mission generation takes place in this UI method.
    FString CampaignHeading = TEXT("CAMPAIGN MISSIONS");
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        UStarshatterPlayerSubsystem* PlayerSS = GameInstance->GetSubsystem<UStarshatterPlayerSubsystem>();
        UStarshatterGameDataSubsystem* DataSS = GameInstance->GetSubsystem<UStarshatterGameDataSubsystem>();
        if (PlayerSS && DataSS)
        {
            if (!PlayerSS->HasLoaded()) PlayerSS->LoadPlayer();
            const FS_PlayerGameInfo& SavedPlayer = PlayerSS->GetPlayerInfo();
            if (SavedPlayer.Campaign > 0)
            {
                if (const FS_Campaign* Row = DataSS->GetCampaignByIndex1Based(SavedPlayer.Campaign))
                {
                    if (!Row->Name.IsEmpty())
                    {
                        CampaignHeading = Row->Name.ToUpper() + TEXT(" CAMPAIGN MISSIONS");
                    }
                }
            }
        }
    }

    APlayerController* PC=GetOwningPlayer(); if (!PC)return;
    if (!CampaignChild) {UCampaignMissionDlg* W=CreateWidget<UCampaignMissionDlg>(PC,UCampaignMissionDlg::StaticClass());CampaignChild=W;if (W){W->SetMenuManager(Manager);W->InitializeDlg(Manager);}}
    if (UCampaignMissionDlg* W=Cast<UCampaignMissionDlg>(CampaignChild)){ W->Show();SelectChild(W,*CampaignHeading); }
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
void UMissionsPanelDriver::RefreshHeaderStatus()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>())
        {
            if (!PlayerSS->HasLoaded()) PlayerSS->LoadPlayer();
            if (PlayerNameText) PlayerNameText->SetText(FText::FromString(PlayerSS->GetPlayerInfo().Name));
        }
        if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>())
        {
            const uint64 Now = Timer->GetUniverseTimeSeconds();
            int64 BaseUnixSeconds = Timer->UniverseBaseUnixSeconds;
            if (BaseUnixSeconds <= 0)
                BaseUnixSeconds = FDateTime(2228, 1, 1).ToUnixTimestamp();
            if (CurrentTimeText)
            {
                const FDateTime UniverseDateTime = FDateTime::FromUnixTimestamp(BaseUnixSeconds + static_cast<int64>(Now));
                CurrentTimeText->SetText(FText::FromString(UniverseDateTime.ToString(TEXT("%Y-%m-%d %H:%M:%S"))));
            }
            if (CampaignTimeText)
            {
                uint64 TPlusSeconds = Timer->GetCampaignTPlusSeconds();
                UCampaignSave* Save = Timer->GetCampaignSave();
                if (!Save)
                    if (USSWGameInstance* SSW = Cast<USSWGameInstance>(GI))
                        Save = SSW->CampaignSave;
                if (Save)
                    TPlusSeconds = Save->GetTPlusSeconds(Now);
                const uint64 Days = TPlusSeconds / 86400ULL;
                const uint64 Rest = TPlusSeconds % 86400ULL;
                CampaignTimeText->SetText(FText::FromString(FString::Printf(
                    TEXT("Day %02llu  %02llu:%02llu:%02llu"),
                    (unsigned long long)(Days + 1ULL),
                    (unsigned long long)(Rest / 3600ULL),
                    (unsigned long long)((Rest % 3600ULL) / 60ULL),
                    (unsigned long long)(Rest % 60ULL))));
            }
        }
    }
    if (PlayerGroupText)
    {
        FString GroupName = TEXT("UNASSIGNED");
        if (Campaign* Live = Campaign::GetCampaign())
            if (CombatGroup* Group = Live->GetPlayerGroup())
                GroupName = Group->GetDescription();
        PlayerGroupText->SetText(FText::FromString(GroupName));
    }
}
void UMissionsPanelDriver::UpdateBriefingEnabled()
{
    bool bHasSelection = false;
    if (USingleMissionDlg* Single = Cast<USingleMissionDlg>(ActiveChild))
        bHasSelection = Single->HasSelectedMission();
    else if (UMultiplayerDlg* Multi = Cast<UMultiplayerDlg>(ActiveChild))
        bHasSelection = Multi->HasSelectedMission();
    if (BriefingTab) BriefingTab->SetIsEnabled(bHasSelection);
}
void UMissionsPanelDriver::OpenBriefing()
{
    if (!BriefingTab || !BriefingTab->GetIsEnabled()) return;
    if (USingleMissionDlg* Single = Cast<USingleMissionDlg>(ActiveChild))
        Single->HandleAccept();
    else if (UMultiplayerDlg* Multi = Cast<UMultiplayerDlg>(ActiveChild))
        Multi->HandleAccept();
}
