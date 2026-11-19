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
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#endif
USSWExhaustTuningCommandlet::USSWExhaustTuningCommandlet(){IsClient=true;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 USSWExhaustTuningCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    if(FParse::Param(*Params,TEXT("Bolts")))
    {
        for(const TCHAR* Variant:{TEXT("Blue"),TEXT("OrangeYellow"),TEXT("Green"),TEXT("Red")})
        {
            const FString SourceName=FString::Printf(TEXT("/Game/Effects/Engines/NS_SSW_Nozzle_%s.NS_SSW_Nozzle_%s"),Variant,Variant);
            auto* Source=LoadObject<UNiagaraSystem>(nullptr,*SourceName);if(!Source)return 30;Source->EnsureFullyLoaded();
            const FString Name=FString::Printf(TEXT("NS_SSW_Bolt_%s"),Variant),PackageName=TEXT("/Game/Effects/Weapons/")+Name;
            auto* FX=LoadObject<UNiagaraSystem>(nullptr,*(PackageName+TEXT(".")+Name));
            if(!FX){FX=DuplicateObject<UNiagaraSystem>(Source,CreatePackage(*PackageName),*Name);if(!FX)return 31;FX->SetFlags(RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(FX);}
            FX->EnsureFullyLoaded();auto& Store=FX->GetExposedParameters();bool Valid=true;
            const auto Float=[&](const TCHAR* Key,float Value){const FNiagaraVariable V(FNiagaraTypeDefinition::GetFloatDef(),FName(Key));if(!Store.GetParameterData(V)){Valid=false;return;}Store.SetParameterValue(Value,V);};
            const auto Size=[&](const TCHAR* Key,FVector2f Value){const FNiagaraVariable V(FNiagaraTypeDefinition::GetVec2Def(),FName(Key));if(!Store.GetParameterData(V)){Valid=false;return;}Store.SetParameterValue(Value,V);};
            Float(TEXT("User.Thrusters_Radius"),.35f);Float(TEXT("User.Thrusters_Life"),.045f);Float(TEXT("User.Thrusters_Speed"),240.f);
            Float(TEXT("User.Thrusters_AttractionPower"),0);Float(TEXT("User.Thrusters_NoiseStrength"),0);Float(TEXT("User.Thrusters_ConeAngle"),.5f);Float(TEXT("User.Thrusters_SpawnRate"),220.f);
            Size(TEXT("User.Thrusters_SpriteSize"),FVector2f(.65f,2.4f));Float(TEXT("User.EnergyCore_Radius"),.3f);Float(TEXT("User.EnergyCore_SpriteSize"),1.1f);
            Float(TEXT("User.EnergyCore_Speed"),0);Float(TEXT("User.EnergyCore_Life"),.035f);Float(TEXT("User.EnergyCore_SpawnRate"),140.f);
            Float(TEXT("User.Particulate_SpawnRate"),0);Float(TEXT("User.HeatHaze_SpawnRate"),0);Float(TEXT("User.Smoke_SpawnRate"),0);Float(TEXT("User.Emissive_Boost"),16.f);
            if(!Valid)return 32;FX->PostEditChange();FX->RequestCompile(true);FX->WaitForCompilationComplete(false,false);
            auto* Package=FX->GetOutermost();Package->MarkPackageDirty();const FString File=FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension());
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);FSavePackageArgs Save;Save.TopLevelFlags=RF_Public|RF_Standalone;Save.SaveFlags=SAVE_NoError;
            if(!UPackage::SavePackage(Package,FX,*File,Save))return 33;UE_LOG(LogTemp,Display,TEXT("Native HDR bolt saved %s"),*PackageName);
        }
        return 0;
    }
    if (FParse::Param(*Params,TEXT("Nozzles")))
    {
        for(const TCHAR* Variant:{TEXT("Blue"),TEXT("OrangeYellow"),TEXT("Green"),TEXT("Red")})
        {
            const FString SourceColor=FString(Variant)==TEXT("OrangeYellow")?TEXT("Yellow"):Variant;
            const FString SourcePath=FString::Printf(TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_%s.NS_RocketExhaust_%s"),*SourceColor,*SourceColor);
            auto* Original=LoadObject<UNiagaraSystem>(nullptr,*SourcePath);if(!Original)return 20;
            Original->EnsureFullyLoaded();
            const FString Name=FString::Printf(TEXT("NS_SSW_Nozzle_%s"),Variant);
            const FString PackageName=TEXT("/Game/Effects/Engines/")+Name;
            auto* FX=LoadObject<UNiagaraSystem>(nullptr,*(PackageName+TEXT(".")+Name));
            if(!FX)
            {
                FX=DuplicateObject<UNiagaraSystem>(Original,CreatePackage(*PackageName),*Name);
                if(!FX)return 21;FX->SetFlags(RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(FX);
            }
            FX->EnsureFullyLoaded();
            auto& Store=FX->GetExposedParameters();
            bool Valid=true;
            const auto Float=[&](const TCHAR* Key,float Value)
            {
                const FNiagaraVariable Variable(FNiagaraTypeDefinition::GetFloatDef(),FName(Key));
                if(!Store.GetParameterData(Variable)){Valid=false;UE_LOG(LogTemp,Error,TEXT("Missing nozzle parameter %s"),Key);return;}
                Store.SetParameterValue(Value,Variable);
            };
            const auto Size=[&](const TCHAR* Key,FVector2f Value)
            {
                const FNiagaraVariable Variable(FNiagaraTypeDefinition::GetVec2Def(),FName(Key));
                if(!Store.GetParameterData(Variable)){Valid=false;return;}
                Store.SetParameterValue(Value,Variable);
            };
            // Canonical one-unit nozzle radius. The native actor supplies the
            // measured world radius exactly once; throttle changes length/emission.
            Float(TEXT("User.Thrusters_Radius"),.65f);
            Float(TEXT("User.Thrusters_Life"),.14f);
            Float(TEXT("User.Thrusters_Speed"),32.f);
            Float(TEXT("User.Thrusters_AttractionPower"),0.f);
            Float(TEXT("User.Thrusters_NoiseStrength"),.5f);
            Float(TEXT("User.Thrusters_NoiseFrequency"),2.f);
            Float(TEXT("User.Thrusters_ConeAngle"),2.f);
            Float(TEXT("User.Thrusters_SpawnRate"),450.f);
            Size(TEXT("User.Thrusters_SpriteSize"),FVector2f(1.4f,2.0f));
            Float(TEXT("User.EnergyCore_Radius"),.45f);
            Float(TEXT("User.EnergyCore_SpriteSize"),1.2f);
            Float(TEXT("User.EnergyCore_Speed"),8.f);
            Float(TEXT("User.EnergyCore_Attraction"),0.f);
            Float(TEXT("User.EnergyCore_Life"),.10f);
            Float(TEXT("User.EnergyCore_SpawnRate"),180.f);
            Float(TEXT("User.Particulate_SpawnRate"),0.f);
            Float(TEXT("User.HeatHaze_SpawnRate"),0.f);
            Float(TEXT("User.Smoke_SpawnRate"),0.f);
            Float(TEXT("User.Emissive_Boost"),1.f);
            if(!Valid)return 22;
            for(auto& Handle:FX->GetEmitterHandles())if(auto* Data=Handle.GetEmitterData())
            {
                const auto Instance=Handle.GetInstance();
                if(!Instance.Emitter || !Instance.Emitter->IsIn(FX))return 23;
                Data->bLocalSpace=true;
                Instance.Emitter->PostEditChange();
            }
            FX->PostEditChange();FX->RequestCompile(true);FX->WaitForCompilationComplete(false,false);
            auto* Package=FX->GetOutermost();Package->MarkPackageDirty();
            const FString Filename=FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension());
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
            FSavePackageArgs Save;Save.TopLevelFlags=RF_Public|RF_Standalone;Save.SaveFlags=SAVE_NoError;
            if(!UPackage::SavePackage(Package,FX,*Filename,Save))return 24;
            UE_LOG(LogTemp,Display,TEXT("Native nozzle recipe saved %s"),*PackageName);
        }
        return 0;
    }
    if (FParse::Param(*Params,TEXT("Inspect")))
    {
        FString Report;
        for (const TCHAR* Color : {TEXT("Blue"),TEXT("Yellow"),TEXT("Green")})
        {
            const FString Path=FString::Printf(TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_%s.NS_RocketExhaust_%s"),Color,Color);
            auto* FX=LoadObject<UNiagaraSystem>(nullptr,*Path);if(!FX)return 10;
            FX->EnsureFullyLoaded();Report+=Path+TEXT("\n");
            TArray<FNiagaraVariable> Variables;FX->GetExposedParameters().GetParameters(Variables);
            for(const auto& Variable:Variables)
            {
                Report+=Variable.GetName().ToString()+TEXT(" ")+Variable.GetType().GetName();
                const uint8* Data=FX->GetExposedParameters().GetParameterData(Variable);
                if(Data && Variable.GetType()==FNiagaraTypeDefinition::GetFloatDef())Report+=FString::Printf(TEXT(" = %g"),*reinterpret_cast<const float*>(Data));
                Report+=TEXT("\n");
            }
            for(auto& Handle:FX->GetEmitterHandles())if(const auto* Data=Handle.GetEmitterData())
                Report+=FString::Printf(TEXT("Emitter %s LocalSpace=%d\n"),*Handle.GetName().ToString(),Data->bLocalSpace);
        }
        return FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("Analysis/NozzleNiagaraParameters.txt")))?0:11;
    }
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
