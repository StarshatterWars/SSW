// Copyright Epic Games, Inc. All Rights Reserved.
#include "StarshatterWars.h"
#include "PerformanceCapture.h"
#include "Modules/ModuleManager.h"
#if UE_SERVER && defined(SSW_FRACTALVERSE)
#include "Misc/ConfigCacheIni.h"
#include "UObject/UnrealType.h"
#include "UObject/SoftObjectPath.h"
#endif
#if WITH_EDITOR
#include "GameDelegates.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#endif

class FStarshatterWarsModule final : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
        SSWPerformanceCapture::Initialize();
#if UE_SERVER && defined(SSW_FRACTALVERSE)
        constexpr const TCHAR* NativeInstance=TEXT("/Script/StarshatterWars.SSWGameInstance");
        if(GConfig) GConfig->SetString(TEXT("/Script/EngineSettings.GameMapsSettings"),
            TEXT("GameInstanceClass"),NativeInstance,GEngineIni);
        // Engine settings can have a cached default object before this module loads.
        if(auto* SettingsClass=FindObject<UClass>(nullptr,TEXT("/Script/EngineSettings.GameMapsSettings")))
            if(auto* Property=FindFProperty<FStructProperty>(SettingsClass,TEXT("GameInstanceClass"));
                Property && Property->Struct->GetFName()==TEXT("SoftClassPath"))
                *Property->ContainerPtrToValuePtr<FSoftClassPath>(SettingsClass->GetDefaultObject())=FSoftClassPath(NativeInstance);
#endif
#if WITH_EDITOR
        FString Variant;
        if(IsRunningCommandlet() && FParse::Value(FCommandLine::Get(),TEXT("CustomConfig="),Variant) && Variant==TEXT("FractalVerse"))
            FGameDelegates::Get().GetModifyCookDelegate().AddLambda(
                [](TConstArrayView<const ITargetPlatform*>,TArray<FName>& Packages,TArray<FName>&)
                {
                    TArray<FAssetData> Assets;
                    FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByPath(TEXT("/Game/Game"),Assets,true);
                    int32 Count=0;
                    for(const auto& Asset:Assets)
                        if(Asset.AssetClassPath.GetAssetName()==TEXT("DataTable"))
                        {Packages.AddUnique(Asset.PackageName);++Count;}
                    UE_LOG(LogTemp,Display,TEXT("[FractalVerseCook] Explicit native table roots: %d; UI assets excluded."),Count);
                });
#endif
    }
    virtual void ShutdownModule() override
    {
        SSWPerformanceCapture::Shutdown();
        FDefaultGameModuleImpl::ShutdownModule();
    }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FStarshatterWarsModule, StarshatterWars, "StarshatterWars");
