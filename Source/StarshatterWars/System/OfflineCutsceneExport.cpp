#include "OfflineCutsceneExport.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"
#if WITH_EDITOR
#include "MenuScreen.h"
#include "SSWGameInstance.h"
#include "StarshatterGameDataSubsystem.h"
#include "PlayerCharacter.h"
#include "Campaign.h"
#include "Engine/GameEngine.h"
#include "Engine/World.h"
#include "MovieSceneCapture.h"
#include "MovieSceneCaptureModule.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace
{
    UMovieSceneCapture* Capture=nullptr;
    FString OutputDirectory;
    FString SceneName;
    double StartSeconds=0;
    int32 FrameRate=30;
    bool bFinishing=false;
    bool bScreenMessagesSuppressed=false;
    TUniquePtr<FS_Campaign> ExportCampaign;
}
#endif
bool OfflineCutsceneExport::IsRequested()
{
#if WITH_EDITOR
    FString Scene;
    return FParse::Value(FCommandLine::Get(),TEXT("SSWRenderScene="),Scene) && !Scene.IsEmpty();
#else
    return false;
#endif
}
bool OfflineCutsceneExport::TryStart(UMenuScreen* Menu)
{
#if WITH_EDITOR
    if(!IsRequested() || !Menu)return false;
    FString Requested;FParse::Value(FCommandLine::Get(),TEXT("SSWRenderScene="),Requested);
    int32 Number=1;FParse::Value(FCommandLine::Get(),TEXT("SSWRenderCampaign="),Number);
    auto* Game=Cast<USSWGameInstance>(Menu->GetGameInstance());
    auto* Data=Game?Game->GetSubsystem<UStarshatterGameDataSubsystem>():nullptr;
    const auto* Row=Data?Data->GetCampaignByIndex1Based(Number):nullptr;
    FString Source;const bool Custom=FParse::Value(FCommandLine::Get(),TEXT("SSWRenderSource="),Source);
    if (Custom && Row)
    {
        FS_CampaignMission Scene;
        if (!Data->ReadLooseMission(Source,Scene,!Source.EndsWith(TEXT(".def"),ESearchCase::IgnoreCase)) || !Scene.Scene.Equals(Requested,ESearchCase::IgnoreCase) ||
            !Scene.Event.ContainsByPredicate([](const FS_MissionEvent& Event){return Event.EventType==MISSIONEVENT_TYPE::END_SCENE;}))
        { UE_LOG(LogTemp,Error,TEXT("[OfflineCutscene] Invalid isolated render source %s"),*Source);FPlatformMisc::RequestExit(false);return true; }
        ExportCampaign=MakeUnique<FS_Campaign>(*Row);
        ExportCampaign->Missions.RemoveAll([&](const FS_CampaignMission& Item){return Item.Scene.Equals(Requested,ESearchCase::IgnoreCase);});
        ExportCampaign->Missions.Add(MoveTemp(Scene));
        Row=ExportCampaign.Get();
    }
    if(!Row || !Row->Missions.ContainsByPredicate([&](const FS_CampaignMission& Item){return Item.Scene.Equals(Requested,ESearchCase::IgnoreCase);}) || !Campaign::SelectFromData(*Row))
    { UE_LOG(LogTemp,Error,TEXT("[OfflineCutscene] Unknown campaign/scene %d/%s"),Number,*Requested);FPlatformMisc::RequestExit(false);return true; }
    Campaign::GetCampaign()->Start();Data->SetActiveCampaign(*Row);Data->CampaignIndex=Number-1;Data->SelectedCampaignRowName=Custom?NAME_None:Row->RowName;
    Game->SelectedCampaignIndex=Number;Game->SelectedCampaignRowName=Row->RowName;Game->SelectedCampaignDisplayName=Row->Name;
    FS_PlayerGameInfo Pilot;Pilot.Name=TEXT("Cutscene Export");Pilot.Callsign=TEXT("RENDER");Pilot.Campaign=Number;Pilot.CampaignRowName=Row->RowName;Pilot.Rank=6;Pilot.bTrainingGraduate=true;
    PlayerCharacter::Initialize();PlayerCharacter::GetCurrentPlayer()->FromPlayerInfo(Pilot);
    Menu->ReturnFromMissionBriefing();return true;
#else
    return false;
#endif
}
void OfflineCutsceneExport::Begin(UWorld* World,const FString& Scene)
{
#if WITH_EDITOR
    if(!IsRequested() || Capture || !World)return;
    auto* Engine=Cast<UGameEngine>(GEngine);
    if(!Engine || !Engine->SceneViewport.IsValid())return;
    FParse::Value(FCommandLine::Get(),TEXT("SSWRenderOutput="),OutputDirectory);
    if(OutputDirectory.IsEmpty())OutputDirectory=FPaths::ProjectSavedDir()/TEXT("CutsceneRenders")/Scene;
    OutputDirectory=FPaths::ConvertRelativePathToFull(OutputDirectory);IFileManager::Get().MakeDirectory(*OutputDirectory,true);
    FParse::Value(FCommandLine::Get(),TEXT("SSWRenderFPS="),FrameRate);FrameRate=FMath::Clamp(FrameRate,24,60);
    SceneName=Scene;StartSeconds=World->GetRealTimeSeconds();bFinishing=false;
    FModuleManager::LoadModuleChecked<IMovieSceneCaptureModule>(TEXT("MovieSceneCapture"));
    Capture=NewObject<UMovieSceneCapture>();Capture->AddToRoot();
    Capture->Settings.OutputDirectory.Path=OutputDirectory;Capture->Settings.OutputFormat=TEXT("frame_{frame}");
    Capture->Settings.ZeroPadFrameNumbers=6;Capture->Settings.FrameRate=FFrameRate(FrameRate,1);
    Capture->Settings.bUseCustomFrameRate=true;Capture->Settings.CustomFrameRate=FFrameRate(FrameRate,1);
    Capture->Settings.Resolution=FCaptureResolution(1920,1080);Capture->Settings.bOverwriteExisting=false;
    Capture->ImageCaptureProtocolType=FSoftClassPath(TEXT("/Script/MovieSceneCapture.ImageSequenceProtocol_PNG"));
    bScreenMessagesSuppressed=GEngine->Exec(World,TEXT("DisableAllScreenMessages"));
    Capture->Initialize(Engine->SceneViewport);Capture->StartCapture();
    // Image capture normally mutes the app. This export records the master submix alongside it.
    FApp::SetVolumeMultiplier(1);FApp::SetUnfocusedVolumeMultiplier(1);
    UAudioMixerBlueprintLibrary::StartRecordingOutput(World,180);
    GEngine->Exec(World,*FString::Printf(TEXT("t.MaxFPS %d"),FrameRate));
    Capture->OnCaptureFinished().AddLambda([]()
    {
        UE_LOG(LogTemp,Display,TEXT("[OfflineCutscene] Capture finalized: %s"),*OutputDirectory);
        if(Capture){Capture->RemoveFromRoot();Capture=nullptr;}
        FPlatformMisc::RequestExit(false);
    });
    UE_LOG(LogTemp,Display,TEXT("[OfflineCutscene] Recording original scene '%s' to %s"),*Scene,*OutputDirectory);
#endif
}
bool OfflineCutsceneExport::Finish(UWorld* World)
{
#if WITH_EDITOR
    if(!IsRequested())return false;
    if(bFinishing)return true;
    bFinishing=true;
    if(!Capture){UE_LOG(LogTemp,Error,TEXT("[OfflineCutscene] Capture never started"));FPlatformMisc::RequestExit(false);return true;}
    UAudioMixerBlueprintLibrary::StopRecordingOutput(World,EAudioRecordingExportType::WavFile,TEXT("audio"),OutputDirectory);
    auto Data=MakeShared<FJsonObject>();Data->SetStringField(TEXT("Scene"),SceneName);Data->SetNumberField(TEXT("FrameRate"),FrameRate);
    Data->SetNumberField(TEXT("Duration"),World?World->GetRealTimeSeconds()-StartSeconds:0);
    Data->SetBoolField(TEXT("ScreenMessagesSuppressed"),bScreenMessagesSuppressed);
    FString Json;FJsonSerializer::Serialize(Data,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(OutputDirectory/TEXT("capture.json")));
    Capture->FinalizeWhenReady();return true;
#else
    return false;
#endif
}
