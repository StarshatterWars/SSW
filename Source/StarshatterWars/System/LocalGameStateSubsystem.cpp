#include "LocalGameStateSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "SSWGameInstance.h"
#include "Sim.h"
#include "Mission.h"
#include "AgentAI.h"
#include "PlayerCharacter.h"
#include "Starshatter.h"
#include "OfflineCutsceneExport.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/SecureHash.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
namespace
{
USaveGame* LoadExistingCheckpoint(const FString& Slot,int32 User)
{
    return UGameplayStatics::DoesSaveGameExist(Slot,User)?UGameplayStatics::LoadGameFromSlot(Slot,User):nullptr;
}
FString ProfileKey(UStarshatterPlayerSubsystem& Players){const FString Active=Players.GetActiveSavedPlayerSlot();return Active.IsEmpty()?Players.GetSlotName():Active;}
bool LiveCareer(const ULocalGameStateSubsystem& Service)
{
    const auto* World=Service.GetWorld();const auto* Game=Cast<USSWGameInstance>(Service.GetGameInstance());
    return !IsRunningCommandlet() && World && (World->WorldType==EWorldType::Game || World->WorldType==EWorldType::PIE) && Game && !Game->IsTrainingTestSession() && (!Sim::GetSim() || !Sim::GetSim()->IsNetworked());
}
}
FString ULocalGameStateSubsystem::ManifestSlot() const
{
    return PendingManifest?TEXT("State_")+FMD5::HashAnsiString(*PendingManifest->ProfileSlot)+TEXT("_manifest"):FString();
}
bool ULocalGameStateSubsystem::ValidateGeneration(const ULocalStateManifest& M,const TArray<ULocalStateArea*>& Areas)
{
    if(M.Version!=1 || !M.Generation.IsValid() || M.ProfileSlot.IsEmpty() || M.Slots.Num()!=4 || Areas.Num()!=4)return false;
    const FString Prefix=TEXT("State_")+FMD5::HashAnsiString(*M.ProfileSlot)+TEXT("_")+M.Generation.ToString(EGuidFormats::Digits)+TEXT("_");
    static const TCHAR* Names[]={TEXT("pilot"),TEXT("economy"),TEXT("skills"),TEXT("universe")};
    for(int32 I=0;I<4;++I)if(M.Slots[I]!=Prefix+Names[I])return false;
    for(const auto* Area:Areas)if(!Area || Area->Version!=1 || Area->Generation!=M.Generation || Area->ProfileSlot!=M.ProfileSlot)return false;
    return Areas[0]->IsA<ULocalPilotState>() && Areas[1]->IsA<ULocalEconomyState>() && Areas[2]->IsA<ULocalSkillState>() && Areas[3]->IsA<ULocalUniverseState>();
}
bool ULocalGameStateSubsystem::CaptureGeneration()
{
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();if(!Players || !Players->HasLoaded() || !LiveCareer(*this))return false;
    UserIndex=Players->GetUserIndex();PendingAreas.Reset();PendingManifest=NewObject<ULocalStateManifest>(this);
    PendingManifest->Generation=FGuid::NewGuid();PendingManifest->ProfileSlot=ProfileKey(*Players);PendingManifest->SavedUtc=FDateTime::UtcNow().ToUnixTimestamp();PendingManifest->PlayerRevision=Players->GetPlayerInfo().LocalStateRevision;
    auto* Pilot=NewObject<ULocalPilotState>(this);Pilot->Pilot=Players->GetPlayerInfo();
    if(auto* Game=Cast<USSWGameInstance>(GetGameInstance()))Pilot->Pilot.AgentJournalJson=Game->GetPersistentTrainingAgent().SaveJournal();
    auto* Economy=NewObject<ULocalEconomyState>(this);Economy->Economy=Pilot->Pilot.Economy;
    auto* Skills=NewObject<ULocalSkillState>(this);Skills->Skills=Pilot->Pilot.Skills;Skills->Queue=Pilot->Pilot.SkillQueue;
    Pilot->Pilot.Economy={};Pilot->Pilot.Skills.Reset();Pilot->Pilot.SkillQueue.Reset();
    auto* Universe=CachedWorld?DuplicateObject<ULocalUniverseState>(CachedWorld,this):NewObject<ULocalUniverseState>(this);
    if(auto* Simulation=Sim::GetSim();Simulation && Simulation->GetMission() && !(Starshatter::GetInstance() && Starshatter::GetInstance()->InCutscene()) && !OfflineCutsceneExport::IsRequested())FLocalWorldStateCodec::Capture(*Simulation,*Universe);
    CachedWorld=Universe;PendingAreas={Pilot,Economy,Skills,Universe};
    static const TCHAR* Names[]={TEXT("pilot"),TEXT("economy"),TEXT("skills"),TEXT("universe")};
    for(int32 I=0;I<PendingAreas.Num();++I){PendingAreas[I]->Generation=PendingManifest->Generation;PendingAreas[I]->ProfileSlot=PendingManifest->ProfileSlot;
        PendingManifest->Slots.Add(TEXT("State_")+FMD5::HashAnsiString(*PendingManifest->ProfileSlot)+TEXT("_")+PendingManifest->Generation.ToString(EGuidFormats::Digits)+TEXT("_")+Names[I]);}
    AreaIndex=0;return true;
}
bool ULocalGameStateSubsystem::BeginAutosave()
{
    if(bSaving || bExiting || !CaptureGeneration())return false;bSaving=true;bIOPending=false;return true;
}
void ULocalGameStateSubsystem::WriteNext()
{
    if(bIOPending || !bSaving || !PendingManifest)return;
    if(AreaIndex>=PendingAreas.Num()){CommitManifest();bSaving=false;return;}
    bIOPending=true;UGameplayStatics::AsyncSaveGameToSlot(PendingAreas[AreaIndex],PendingManifest->Slots[AreaIndex],UserIndex,
        FAsyncSaveGameToSlotDelegate::CreateUObject(this,&ULocalGameStateSubsystem::OnAreaSaved));
}
void ULocalGameStateSubsystem::OnAreaSaved(const FString& Slot,int32,bool Success)
{
    // Old background writes cannot commit after an exit-save starts a new generation.
    if(bExiting || !bSaving || !PendingManifest || !PendingManifest->Slots.IsValidIndex(AreaIndex) || Slot!=PendingManifest->Slots[AreaIndex])return;
    bIOPending=false;if(!Success){bSaving=false;UE_LOG(LogTemp,Warning,TEXT("[LocalState] Area save failed. Previous generation retained."));return;}++AreaIndex;
}
bool ULocalGameStateSubsystem::CommitManifest()
{
    if(!PendingManifest || AreaIndex!=4)return false;
    const FString Slot=ManifestSlot();auto* Obsolete=Cast<ULocalStateManifest>(LoadExistingCheckpoint(Slot+TEXT("_previous"),UserIndex));auto* Previous=Cast<ULocalStateManifest>(LoadExistingCheckpoint(Slot,UserIndex));
    if(Previous && !UGameplayStatics::SaveGameToSlot(Previous,Slot+TEXT("_previous"),UserIndex))return false;
    const bool Saved=UGameplayStatics::SaveGameToSlot(PendingManifest,Slot,UserIndex);
    if(!Saved && Previous)UGameplayStatics::SaveGameToSlot(Previous,Slot,UserIndex);
    if(Saved && Obsolete && Obsolete->ProfileSlot==PendingManifest->ProfileSlot && Obsolete->Generation!=PendingManifest->Generation && (!Previous || Obsolete->Generation!=Previous->Generation))
    {
        const FString Prefix=TEXT("State_")+FMD5::HashAnsiString(*Obsolete->ProfileSlot)+TEXT("_")+Obsolete->Generation.ToString(EGuidFormats::Digits)+TEXT("_");
        for(const auto& OldSlot:Obsolete->Slots)if(OldSlot==Prefix+TEXT("pilot") || OldSlot==Prefix+TEXT("economy") || OldSlot==Prefix+TEXT("skills") || OldSlot==Prefix+TEXT("universe"))UGameplayStatics::DeleteGameInSlot(OldSlot,UserIndex);
    }
    UE_LOG(LogTemp,Display,TEXT("[LocalState] Generation=%s Areas=4 Ships=%d Complete=%d"),*PendingManifest->Generation.ToString(),CachedWorld?CachedWorld->Ships.Num():0,Saved?1:0);return Saved;
}
void ULocalGameStateSubsystem::Tick(float)
{
    if(bExiting || !LiveCareer(*this))return;
    if(bSaving){WriteNext();return;}
    const double Now=FPlatformTime::Seconds();if(LastSaveClock==0){LastSaveClock=Now;return;}
    if(Now-LastSaveClock>=300){LastSaveClock=Now;BeginAutosave();}
}
bool ULocalGameStateSubsystem::SaveCheckpointNow()
{
    if(bExiting || !LiveCareer(*this))return false;
    // Supersede unfinished async writes using a new generation. Their callbacks
    // cannot match these new slot names, and normal autosaving remains enabled.
    bSaving=false;bIOPending=false;
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Players || !Players->HasLoaded() || !Players->SaveActivePlayerProgress() || !CaptureGeneration())return false;
    const bool Saved=FlushPendingSynchronously();
    if(Saved)LastSaveClock=FPlatformTime::Seconds();
    return Saved;
}
bool ULocalGameStateSubsystem::SaveAtExit()
{
    if(bExiting)return bExitSaveSucceeded;
    if(!LiveCareer(*this))return false;
    // Exit commits fresh unique filenames; an unfinished autosave stays unreferenced.
    bExiting=true;bSaving=false;bIOPending=false;
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Players || !Players->HasLoaded() || !Players->SaveActivePlayerProgress() || !CaptureGeneration())return false;
    bExitSaveSucceeded=FlushPendingSynchronously();return bExitSaveSucceeded;
}
bool ULocalGameStateSubsystem::FlushPendingSynchronously()
{
    if(!PendingManifest || PendingAreas.Num()!=4 || !ValidateGeneration(*PendingManifest,{PendingAreas[0],PendingAreas[1],PendingAreas[2],PendingAreas[3]}))return false;
    for(AreaIndex=0;AreaIndex<PendingAreas.Num();++AreaIndex)
        if(!UGameplayStatics::SaveGameToSlot(PendingAreas[AreaIndex],PendingManifest->Slots[AreaIndex],UserIndex))return false;
    return CommitManifest();
}
void ULocalGameStateSubsystem::CaptureBeforeMissionUnload()
{
    if(!LiveCareer(*this))return;
    if(!CachedWorld)CachedWorld=NewObject<ULocalUniverseState>(this);
    if(auto* Simulation=Sim::GetSim();Simulation && Simulation->GetMission() && !(Starshatter::GetInstance() && Starshatter::GetInstance()->InCutscene()) && !OfflineCutsceneExport::IsRequested())FLocalWorldStateCodec::Capture(*Simulation,*CachedWorld);
    bWorldRestored=false;
}
bool ULocalGameStateSubsystem::RestoreAccount(FS_PlayerGameInfo& Player,const FString& Profile)
{
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();if(!Players)return false;
    UserIndex=Players->GetUserIndex();bWorldRestored=false;ULocalUniverseState* World=nullptr;
    const bool Result=ReadCheckpoint(Player,Profile,UserIndex,World);CachedWorld=World;return Result;
}
bool ULocalGameStateSubsystem::ReadCheckpoint(FS_PlayerGameInfo& Player,const FString& Profile,int32 UserIndex,ULocalUniverseState*& World)
{
    World=nullptr;
    const FString Slot=TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_manifest");
    for(const FString Candidate:{Slot,Slot+TEXT("_previous")})
    {
        auto* M=Cast<ULocalStateManifest>(LoadExistingCheckpoint(Candidate,UserIndex));if(!M || M->ProfileSlot!=Profile || M->Slots.Num()!=4)continue;
        TArray<ULocalStateArea*> Areas;for(const auto& S:M->Slots)Areas.Add(Cast<ULocalStateArea>(LoadExistingCheckpoint(S,UserIndex)));
        if(!ValidateGeneration(*M,Areas))continue;
        World=M->PlayerRevision>=Player.LocalStateRevision?CastChecked<ULocalUniverseState>(Areas[3]):nullptr;
        // A later atomic pilot transaction takes precedence over an older autosave.
        if(M->PlayerRevision>=Player.LocalStateRevision)
        {Player=CastChecked<ULocalPilotState>(Areas[0])->Pilot;Player.Economy=CastChecked<ULocalEconomyState>(Areas[1])->Economy;Player.Skills=CastChecked<ULocalSkillState>(Areas[2])->Skills;Player.SkillQueue=CastChecked<ULocalSkillState>(Areas[2])->Queue;}
        UE_LOG(LogTemp,Display,TEXT("[LocalState] Restored generation=%s Profile=%s Ships=%d"),*M->Generation.ToString(),*Profile,World?World->Ships.Num():0);return true;
    }
    return false;
}
bool ULocalGameStateSubsystem::RestoreWorld(Sim& Simulation)
{
    if(bWorldRestored || !CachedWorld)return false;
    const bool Result=FLocalWorldStateCodec::Restore(Simulation,*CachedWorld);if(Result)bWorldRestored=true;return Result;
}
int32 ULocalGameStateSubsystem::ResumeMissionId() const{return CachedWorld && !CachedWorld->bMissionComplete?(CachedWorld->AgentAssignmentId!=INDEX_NONE?CachedWorld->AgentAssignmentId:CachedWorld->MissionId):INDEX_NONE;}
FGuid ULocalGameStateSubsystem::ResolveHullIdentity(const FString& Name,FName Design) const
{
 if(!CachedWorld)return {};
 const FLocalShipState* Match=nullptr;
 for(const auto& Row:CachedWorld->Ships)if(Row.Name==Name && FName(*Row.Design)==Design && !Row.bDestroyed)
 {if(Match)return {};Match=&Row;}
 return Match?(Match->HullId.IsValid()?Match->HullId:Match->CargoId):FGuid();
}
bool ULocalGameStateSubsystem::PrepareHullRegistry(FHullRegistry& Registry) const
{
 if(!CachedWorld)return true;
 FString Error;return Registry.Import(CachedWorld->HullRegistry,Error);
}
