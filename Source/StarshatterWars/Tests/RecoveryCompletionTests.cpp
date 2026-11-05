#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissionCompletionStatus.h"
#include "CampaignPresentationRules.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "MissionCameraRig.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionCompletionPersistenceTest,"Starshatter.Mission.CompletionPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionCompletionPersistenceTest::RunTest(const FString&)
{
    auto* CampaignSave=NewObject<UCampaignSave>();CampaignSave->CampaignIndex=1;
    auto* PlayerSave=NewObject<UPlayerSaveGame>();PlayerSave->PlayerInfo.Name=TEXT("Completion fixture");
    TestFalse(TEXT("Fresh assignment is not complete"),MissionCompletionStatus::IsCompleted(CampaignSave,&PlayerSave->PlayerInfo,1,101));
    CampaignSave->RecordCompletedMission(101);
    TestTrue(TEXT("Older campaign history imports into player progress"),MissionCompletionStatus::ImportCampaign(PlayerSave->PlayerInfo,*CampaignSave,1));
    TestFalse(TEXT("Import is idempotent"),MissionCompletionStatus::ImportCampaign(PlayerSave->PlayerInfo,*CampaignSave,1));
    TestFalse(TEXT("Same mission ID in another campaign stays incomplete"),MissionCompletionStatus::IsCompleted(CampaignSave,&PlayerSave->PlayerInfo,2,101));
    TestFalse(TEXT("Other assignments stay incomplete"),MissionCompletionStatus::IsCompleted(CampaignSave,&PlayerSave->PlayerInfo,1,102));
    TestFalse(TEXT("Invalid completion ignored"),PlayerSave->PlayerInfo.MarkMissionCompleted(1,0));
    PlayerSave->PlayerInfo.MarkMissionCompleted(1,103);
    PlayerSave->PlayerInfo.bTrainingGraduate=true;CampaignSave->bTrainingGraduate=true;
    const FString Prefix=TEXT("Automation_Completion_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString PlayerSlot=Prefix+TEXT("_Player"),CampaignSlot=Prefix+TEXT("_Campaign");
    ON_SCOPE_EXIT {UGameplayStatics::DeleteGameInSlot(PlayerSlot,0);UGameplayStatics::DeleteGameInSlot(CampaignSlot,0);};
    TestTrue(TEXT("Player .sav writes"),UGameplayStatics::SaveGameToSlot(PlayerSave,PlayerSlot,0));
    TestTrue(TEXT("Campaign .sav writes"),UGameplayStatics::SaveGameToSlot(CampaignSave,CampaignSlot,0));
    auto* LoadedPlayer=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(PlayerSlot,0));
    auto* LoadedCampaign=Cast<UCampaignSave>(UGameplayStatics::LoadGameFromSlot(CampaignSlot,0));
    if(TestNotNull(TEXT("Player .sav reloads"),LoadedPlayer)&&TestNotNull(TEXT("Campaign .sav reloads"),LoadedCampaign)){
        TestTrue(TEXT("Player save independently preserves completion"),MissionCompletionStatus::IsCompleted(nullptr,&LoadedPlayer->PlayerInfo,1,101));
        TestTrue(TEXT("Campaign save independently preserves completion"),MissionCompletionStatus::IsCompleted(LoadedCampaign,nullptr,1,101));
        TestTrue(TEXT("Graduation persists in player save"),LoadedPlayer->PlayerInfo.bTrainingGraduate);
        TestTrue(TEXT("Graduation persists in campaign save"),LoadedCampaign->bTrainingGraduate);
        TestTrue(TEXT("Flight School unlock survives player reload"),
            CampaignPresentationRules::HasPassedFlightSchool(LoadedPlayer->PlayerInfo));
        TestTrue(TEXT("Graduate can select a campaign after reload"),
            CampaignPresentationRules::CanSelectCampaign(LoadedPlayer->PlayerInfo));
        TestEqual(TEXT("Completion does not duplicate"),LoadedPlayer->PlayerInfo.CompletedMissions.Num(),2);
        TestEqual(TEXT("Other player data preserved"),LoadedPlayer->PlayerInfo.Name,FString(TEXT("Completion fixture")));
    }
    auto* EnsignSave = NewObject<UPlayerSaveGame>();
    EnsignSave->PlayerInfo.bStartedAsEnsign = true;
    EnsignSave->PlayerInfo.Campaign = 2;
    TestTrue(TEXT("First-run Ensign .sav writes"), UGameplayStatics::SaveGameToSlot(EnsignSave, PlayerSlot, 0));
    const auto* LoadedEnsign = Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(PlayerSlot, 0));
    if (TestNotNull(TEXT("First-run Ensign .sav reloads"), LoadedEnsign))
    {
        TestTrue(TEXT("First-run Ensign can select a campaign after reload"),
            CampaignPresentationRules::CanSelectCampaign(LoadedEnsign->PlayerInfo));
        TestFalse(TEXT("First-run Ensign still requires Flight School after reload"),
            CampaignPresentationRules::HasPassedFlightSchool(LoadedEnsign->PlayerInfo));
        TestEqual(TEXT("Selected story campaign survives player reload"), LoadedEnsign->PlayerInfo.Campaign, 2);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryCameraHoldTest,"Starshatter.Mission.RecoveryCameraHold",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryCameraHoldTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Preview world"),World))return false;
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Fighter=World->SpawnActor<AFighterActor>();auto* Camera=World->SpawnActor<ACameraActor>();
    const FTransform Pose(FRotator(-15,45,3),FVector(100,200,300));Camera->SetActorTransform(Pose);
    const FTransform ParkedPose=Fighter->GetActorTransform();
    Camera->AttachToActor(Fighter,FAttachmentTransformRules::KeepWorldTransform);
    FMissionCameraRig Rig;Rig.PlayerActor=Fighter;Rig.HoldRecoveryView(nullptr,Camera);
    TestNull(TEXT("Recovery camera releases moving hull"),Camera->GetAttachParentActor());
    Fighter->SetActorLocationAndRotation(FVector(8000,-6000,1200),FRotator(30,170,-60));
    TestTrue(TEXT("Docked hull motion cannot jitter held camera"),Camera->GetActorTransform().Equals(Pose,.001));
    Camera->SetActorLocation(FVector(1,2,3));Rig.HoldRecoveryView(nullptr,Camera);
    TestTrue(TEXT("Docking updates cannot shake the parked fighter"),Fighter->GetActorTransform().Equals(ParkedPose,.001));
    TestTrue(TEXT("Later recovery callbacks retain the original pose"),Camera->GetActorTransform().Equals(Pose,.001));
    Rig.Reset(nullptr);TestFalse(TEXT("Next mission resets recovery hold"),Rig.bRecoveryViewHeld);
    TestFalse(TEXT("Next mission releases the parked fighter pose"),Fighter->bRecoveryPresentationHeld);
    return true;
}
#endif
