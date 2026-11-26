#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LocalGameStateSave.h"
#include "LocalGameStateSubsystem.generated.h"
UCLASS()
class STARSHATTERWARS_API ULocalGameStateSubsystem : public UGameInstanceSubsystem,public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Tick(float) override;
    bool IsTickable() const override {return !IsTemplate() && GetGameInstance()!=nullptr;}
    UWorld* GetTickableGameObjectWorld() const override {return GetWorld();}
    TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(ULocalGameStateSubsystem,STATGROUP_Tickables);}
    bool BeginAutosave();
    bool SaveAtExit();
    bool SaveCheckpointNow();
    void CaptureBeforeMissionUnload();
    bool RestoreAccount(FS_PlayerGameInfo&,const FString& ProfileSlot);
    bool RestoreWorld(class Sim&);
    int32 ResumeMissionId() const;
    FGuid ResolveHullIdentity(const FString& Name,FName Design) const;
    bool PrepareHullRegistry(FHullRegistry& Registry) const;
    static bool ReadCheckpoint(FS_PlayerGameInfo&,const FString& ProfileSlot,int32 UserIndex,ULocalUniverseState*& World);
    static bool ValidateGeneration(const ULocalStateManifest&,const TArray<ULocalStateArea*>&);
private:
    friend class FCheckpointInterruptionTest;
    bool FlushPendingSynchronously();
    bool CaptureGeneration();
    void WriteNext();
    void OnAreaSaved(const FString&,int32,bool);
    bool CommitManifest();
    FString ManifestSlot() const;
    double LastSaveClock=0;
    bool bSaving=false;
    bool bIOPending=false;
    bool bExiting=false;
    bool bExitSaveSucceeded=false;
    bool bWorldRestored=false;
    int32 AreaIndex=0;
    int32 UserIndex=0;
    UPROPERTY(Transient) TArray<TObjectPtr<ULocalStateArea>> PendingAreas;
    UPROPERTY(Transient) TObjectPtr<ULocalStateManifest> PendingManifest;
    UPROPERTY(Transient) TObjectPtr<ULocalUniverseState> CachedWorld;
};
