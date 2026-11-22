#include "ModEditorDlg.h"
#include "JsonObjectConverter.h"
#include "MenuScreen.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Styling/CoreStyle.h"
#include "StationServices.h"
#include "SSWMissionMods.h"
#include "StarshatterGameDataSubsystem.h"
#include "SSWRuntimeSubsystem.h"
#include "Sim.h"
#include "Mission.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

bool UModEditorDlg::CanUseDockedEditor() { return UMenuScreen::CanOpenMainMenu(); }
bool UModEditorDlg::IsEditableMissionPath(const FString& Path)
{
    FString Full=FPaths::ConvertRelativePathToFull(Path),Root=SSWMissionMods::Directory();
    FPaths::NormalizeFilename(Full);FPaths::NormalizeFilename(Root);
    if(!FPaths::CollapseRelativeDirectories(Full))return false;
    return Full.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase) &&
        Full.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase);
}
bool UModEditorDlg::BeginEditingSession()
{
    if(!CanUseDockedEditor())return false;
    auto* Runtime=GetGameInstance()?GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>():nullptr;
    PausedSimulation=Sim::GetSim();PausedMission=PausedSimulation?PausedSimulation->GetMission():nullptr;
    bOwnsPause=Runtime && !Runtime->IsPaused();if(Runtime)Runtime->SetPaused(true);return true;
}
void UModEditorDlg::EndEditingSession()
{
    if(bOwnsPause && Sim::GetSim()==PausedSimulation && (!PausedSimulation || PausedSimulation->GetMission()==PausedMission))
        if(auto* Runtime=GetGameInstance()?GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>():nullptr)Runtime->SetPaused(false);
    bOwnsPause=false;PausedSimulation=nullptr;PausedMission=nullptr;
}
void UModEditorDlg::NativeDestruct() {EndEditingSession();Super::NativeDestruct();}
void UModEditorDlg::HandleCancel() {if(auto* Menu=GetMenuManager())Menu->ShowMenuDlg();}
void UModEditorDlg::NativeTick(const FGeometry& Geometry,float Seconds)
{
    Super::NativeTick(Geometry,Seconds);
    if(!CanUseDockedEditor())
    {
        EndEditingSession();SetVisibility(ESlateVisibility::Collapsed);SetDialogInputEnabled(false);
    }
}
namespace
{
struct FModRow {FString Pack,File;bool Enabled=false;};
using FModRowPtr=TSharedPtr<FModRow>;
struct FModWorkspace
{
    TArray<FModRowPtr> Rows;
    TWeakPtr<SListView<FModRowPtr>> List;
    TWeakPtr<SMultiLineEditableTextBox> Editor;
    TWeakObjectPtr<UGameInstance> Instance;
    FModRowPtr Selected;
    FString Original;
    FText Message;
    void Refresh()
    {
        Rows.Empty();Selected.Reset();Original.Empty();
        if(auto Text=Editor.Pin())Text->SetText(FText::GetEmpty());
        for(const auto& Pack:SSWMissionMods::Discover())
        {
            if(Pack.Missions.IsEmpty())Rows.Add(MakeShared<FModRow>(FModRow{Pack.Name,TEXT(""),Pack.bEnabled}));
            for(const auto& File:Pack.Missions)Rows.Add(MakeShared<FModRow>(FModRow{Pack.Name,File,Pack.bEnabled}));
        }
        if(auto View=List.Pin()){View->ClearSelection();View->RequestListRefresh();}
    }
    bool CanEdit() const
    {return UModEditorDlg::CanUseDockedEditor() && Selected && UModEditorDlg::IsEditableMissionPath(Selected->File);}
    void Select(FModRowPtr Row)
    {
        Selected=Row;Original.Empty();
        if(Row && !Row->File.IsEmpty() && IFileManager::Get().FileSize(*Row->File)<=4*1024*1024)
            FFileHelper::LoadFileToString(Original,*Row->File);
        if(auto Text=Editor.Pin())Text->SetText(FText::FromString(Original));
        Message=Row && !Row->File.IsEmpty() && !CanEdit()?INVTEXT("THIS MISSION IS READ ONLY. SELECT ITS JSON SOURCE OR CREATE AN ISOLATED COPY."):FText::GetEmpty();
    }
    bool Validate(const FString& Path)
    {
        auto* GI=Instance.Get();auto* Data=GI?GI->GetSubsystem<UStarshatterGameDataSubsystem>():nullptr;
        FS_CampaignMission Parsed;
        if(!Data || !Data->ReadLooseMission(Path,Parsed))
        {Message=INVTEXT("MISSION VALIDATION FAILED. THE EXISTING FILE WAS NOT REPLACED.");return false;}
        return true;
    }
    void Save()
    {
        if(!CanEdit())return;
        auto Text=Editor.Pin();if(!Text)return;
        const FString Source=Text->GetText().ToString();
        if(Source.Len()>4*1024*1024){Message=INVTEXT("MISSION SOURCE EXCEEDS THE EDITOR SIZE LIMIT.");return;}
        const FString Path=Selected->File;
        const FString Temp=FPaths::ChangeExtension(Path,TEXT(""))+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".")+FPaths::GetExtension(Path);
        if(!FFileHelper::SaveStringToFile(Source,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {Message=INVTEXT("COULD NOT WRITE THE MISSION. CHECK FOLDER PERMISSIONS.");return;}
        if(!Validate(Temp)){IFileManager::Get().Delete(*Temp);return;}
        const FString Backup=Path+TEXT(".bak");
        if(IFileManager::Get().Copy(*Backup,*Path,true,false)!=COPY_OK ||
            !IFileManager::Get().Move(*Path,*Temp,true,false,false,true))
        {IFileManager::Get().Delete(*Temp);Message=INVTEXT("SAVE FAILED. ORIGINAL MISSION OR ITS BACKUP HAS BEEN RETAINED.");return;}
        Original=Source;Message=INVTEXT("MISSION SAVED AND VALIDATED. CHANGES APPLY WHEN THIS CUSTOM MISSION IS LOADED; THE CURRENT MISSION IS UNCHANGED.");
    }
    void Snapshot()
    {
        auto* Simulation=Sim::GetSim();auto* Live=Simulation?Simulation->GetMission():nullptr;
        if(!UModEditorDlg::CanUseDockedEditor())return;
        const FString Folder=SSWMissionMods::Directory()/TEXT("LocalAuthoring/Missions");
        if(!IFileManager::Get().MakeDirectory(*Folder,true)){Message=INVTEXT("COULD NOT CREATE THE LOCAL AUTHORING FOLDER.");return;}
        const FString Path=Folder/(TEXT("Mission_")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".def"));
        FString Source;
        if(UMenuScreen::HasLivePlayerSession())Source=UTF8_TO_TCHAR(Live->Serialize(nullptr,0,false).data());
        else if(!FFileHelper::LoadFileToString(Source,*(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/T1.def"))))
        {Message=INVTEXT("THE STARTER MISSION COULD NOT BE READ.");return;}
        if(!FFileHelper::SaveStringToFile(Source,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {Message=INVTEXT("COULD NOT SAVE THE MISSION COPY.");return;}
        auto* GI=Instance.Get();auto* Data=GI?GI->GetSubsystem<UStarshatterGameDataSubsystem>():nullptr;
        FS_CampaignMission Parsed; FString Json;
        if(!Data || !Data->ReadLooseMission(Path,Parsed,false) || !FJsonObjectConverter::UStructToJsonObjectString(Parsed,Json) ||
            !FFileHelper::SaveStringToFile(Json,*FPaths::ChangeExtension(Path,TEXT("json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {IFileManager::Get().Delete(*Path);Message=INVTEXT("COULD NOT CREATE JSON MISSION SOURCE.");return;}
        IFileManager::Get().Delete(*Path);
        Refresh();Message=INVTEXT("ISOLATED COPY CREATED IN LOCALAUTHORING. ENABLE THE PACK FOR THE NEXT GAME START. THE RUNNING MISSION HAS NOT CHANGED.");
    }
};
}
TSharedRef<SWidget> UModEditorDlg::RebuildWidget()
{
    const auto State=MakeShared<FModWorkspace>();State->Instance=GetGameInstance();
    const auto Font=MissionUIStyle::GetRowFont(12);
    auto Editor=SNew(SMultiLineEditableTextBox).Font(Font).AutoWrapText(false)
        .IsReadOnly_Lambda([State](){return !State->CanEdit();});
    State->Editor=Editor;
    auto List=SNew(SListView<FModRowPtr>).ListItemsSource(&State->Rows).SelectionMode(ESelectionMode::Single)
        .OnSelectionChanged_Lambda([State](FModRowPtr Row,ESelectInfo::Type){State->Select(Row);})
        .OnGenerateRow_Lambda([State,Font](FModRowPtr Row,const TSharedRef<STableViewBase>& Owner)->TSharedRef<ITableRow>
        {
            return SNew(STableRow<FModRowPtr>,Owner).Style(&MissionUIStyle::FlightTableRowStyle())
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,3)
                    [SNew(SCheckBox).ToolTipText(INVTEXT("Enable this mod pack for the next game start. Single-player and campaigns only."))
                        .IsEnabled_Lambda([](){return UModEditorDlg::CanUseDockedEditor();})
                        .IsChecked_Lambda([Row](){return Row->Enabled?ECheckBoxState::Checked:ECheckBoxState::Unchecked;})
                        .OnCheckStateChanged_Lambda([State,Row](ECheckBoxState Value)
                        {
                            if(!UModEditorDlg::CanUseDockedEditor())return;
                            SSWMissionMods::SetEnabled(Row->Pack,Value==ECheckBoxState::Checked);
                            for(const auto& Other:State->Rows)if(Other->Pack==Row->Pack)Other->Enabled=Value==ECheckBoxState::Checked;
                            State->Message=INVTEXT("MOD PREFERENCE SAVED FOR NEXT START. MULTIPLAYER NEVER USES THESE PACKS.");
                        })]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(6,3)
                    [SNew(STextBlock).Font(Font).Text(FText::FromString(Row->Pack).ToUpper())]
                    + SHorizontalBox::Slot().FillWidth(2).VAlign(VAlign_Center).Padding(6,3)
                    [SNew(STextBlock).Font(Font).Text(FText::FromString(Row->File.IsEmpty()?TEXT("ARCHIVE / CAMPAIGN PACK"):FPaths::GetCleanFilename(Row->File)).ToUpper())]];
        });
    State->List=List;State->Refresh();
    const auto Action=[State](EShipSystemIcon Icon,const FText& Tooltip,TFunction<void()> Callback,TFunction<bool()> Enabled)->TSharedRef<SWidget>
    {
        return SNew(SButton).ToolTipText(Tooltip).IsEnabled_Lambda(MoveTemp(Enabled))
            .OnClicked_Lambda([Callback=MoveTemp(Callback)](){Callback();return FReply::Handled();})[SNew(SShipSystemIcon).Icon(Icon)];
    };
    auto Body=SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(20,18,20,8)
        [SNew(STextBlock).Font(Font).AutoWrapText(true).Text(INVTEXT("SINGLE PLAYER / CAMPAIGN MODS\nEDIT ISOLATED MISSION SOURCE. A LIVE DOCKED SESSION IS PAUSED WHILE EDITING. PACK CHANGES APPLY ON NEXT START."))]
        + SVerticalBox::Slot().FillHeight(.35).Padding(20,0,20,8)[MissionUIStyle::MakeDataPane(List,FMargin(0))]
        + SVerticalBox::Slot().AutoHeight().Padding(20,0,20,6)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[Action(EShipSystemIcon::Refresh,INVTEXT("Refresh installed mod packs"),[State](){State->Refresh();},[](){return UModEditorDlg::CanUseDockedEditor();})]
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[Action(EShipSystemIcon::MissionPackage,INVTEXT("Create an isolated mission copy, or a starter mission when no game is active"),[State](){State->Snapshot();},[](){return UModEditorDlg::CanUseDockedEditor();})]
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[Action(EShipSystemIcon::Accept,INVTEXT("Validate and save source; retains a .bak copy"),[State](){State->Save();},[State](){return State->CanEdit();})]
            + SHorizontalBox::Slot().AutoWidth()[Action(EShipSystemIcon::Hangar,INVTEXT("Open the user Mods folder"),[](){FPlatformProcess::ExploreFolder(*SSWMissionMods::Directory());},[](){return UModEditorDlg::CanUseDockedEditor();})]
            + SHorizontalBox::Slot().AutoWidth().Padding(6,0)[Action(EShipSystemIcon::SettingsGear,INVTEXT("Open the existing mission designer. Available from the main menu with no live pilot session."),[this](){if(auto* Menu=GetMenuManager())Menu->OpenModMissionTools();},[](){return UModEditorDlg::CanUseDockedEditor() && !UMenuScreen::HasLivePlayerSession();})]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(10,0)
            [SNew(STextBlock).Font(Font).Text_Lambda([State](){return State->Selected?FText::FromString(State->Selected->File):INVTEXT("SELECT A MISSION SOURCE FILE");}).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]]
        + SVerticalBox::Slot().FillHeight(.65).Padding(20,0,20,8)[MissionUIStyle::MakeDataPane(Editor,FMargin(0))]
        + SVerticalBox::Slot().AutoHeight().Padding(20,0,20,18)
        [SNew(STextBlock).Font(Font).AutoWrapText(true).Text_Lambda([State](){return State->Message;})];
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.015f,.025f,.04f,.96f))
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [SNew(SBox).WidthOverride(1340).HeightOverride(850)
                [SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1).Padding(20,10)
                        [SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(22)).Text(INVTEXT("MOD EDITOR"))]
                        + SHorizontalBox::Slot().AutoWidth().Padding(10)
                        [SNew(SButton).ToolTipText(INVTEXT("Return to main menu")).OnClicked_Lambda([this](){HandleCancel();return FReply::Handled();})
                            [SNew(STextBlock).Text(INVTEXT("\u2630")).Font(FCoreStyle::GetDefaultFontStyle("Regular",22))]]]
                    + SVerticalBox::Slot().FillHeight(1)[Body]]]];
}
