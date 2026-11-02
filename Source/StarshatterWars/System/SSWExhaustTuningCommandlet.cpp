#include "SSWExhaustTuningCommandlet.h"
#include "FighterExhaustTuning.h"
#if WITH_EDITOR
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEffectType.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#endif
USSWExhaustTuningCommandlet::USSWExhaustTuningCommandlet(){IsClient=true;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 USSWExhaustTuningCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    auto* Source=LoadObject<UNiagaraSystem>(nullptr,FighterExhaustTuning::StockSystem);
    if(!Source)return 1;
    Source->EnsureFullyLoaded();
    UE_LOG(LogTemp,Display,TEXT("Exhaust source handles=%d class=%s"),Source->GetEmitterHandles().Num(),*Source->GetClass()->GetPathName());
    const FString PackageName=TEXT("/Game/Effects/Engines/NS_SSW_FighterExhaust_Blue");
    auto* System=LoadObject<UNiagaraSystem>(nullptr,FighterExhaustTuning::FighterSystem);
    if(!System)
    {
        auto* Package=CreatePackage(*PackageName);
        System=DuplicateObject<UNiagaraSystem>(Source,Package,TEXT("NS_SSW_FighterExhaust_Blue"));
        if(!System)return 2;
        System->SetFlags(RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(System);
    }
    System->EnsureFullyLoaded();
    UE_LOG(LogTemp,Display,TEXT("Exhaust recipe loaded %d emitter handles"),System->GetEmitterHandles().Num());
    int32 Changed=0;
    for(auto& Handle:System->GetEmitterHandles())
    {
        const auto Instance=Handle.GetInstance();
        auto* Data=Handle.GetEmitterData();
        if(!Instance.Emitter || !Data)continue;
        if(!Instance.Emitter->IsIn(System)){UE_LOG(LogTemp,Error,TEXT("Exhaust recipe refuses to edit a shared emitter"));return 3;}
        FNiagaraEmitterScalabilityOverride Override;
        Override.Platforms=FNiagaraPlatformSet(INDEX_NONE);
        Override.bOverrideSpawnCountScale=true;Override.bScaleSpawnCount=true;
        Override.SpawnCountScale=FighterExhaustTuning::SpawnScale;
        Data->ScalabilityOverrides.Overrides.Reset();Data->ScalabilityOverrides.Overrides.Add(Override);
        Instance.Emitter->PostEditChange();++Changed;
    }
    if(!Changed)return 4;
    System->PostEditChange();System->RequestCompile(true);System->WaitForCompilationComplete(false,false);
    auto* Package=System->GetOutermost();Package->MarkPackageDirty();
    const FString Filename=FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    if(!UPackage::SavePackage(Package,System,*Filename,Args))return 5;
    UE_LOG(LogTemp,Display,TEXT("Fighter exhaust saved: %d emitter(s), %.0f%% spawn count"),Changed,FighterExhaustTuning::SpawnScale*100);
    return 0;
#else
    return 1;
#endif
}
