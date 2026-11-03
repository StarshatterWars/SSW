#include "TurretBaseCatalog.h"
#include "TurretCatalog.h"
#include "WeaponDesignRegistry.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "LCATurretBaseActor.h"
#include "BlockadeTurretBaseActor.h"
#include "Orca2TurretBaseActor.h"
#include "Orca2InvertedTurretBaseActor.h"
#include "VanguardTurretBaseActor.h"
#include "GraserTurretBaseActor.h"
#include "AAATurretBaseActor.h"
#include "QuadAAATurretBaseActor.h"
#include "SAM1TurretBaseActor.h"
TSubclassOf<ATurretBaseActor> TurretBaseCatalog::NativeClass(const FString& Model)
{
    const FString Name=FPaths::GetBaseFilename(Model);
    if(Name.Equals(TEXT("base"),ESearchCase::IgnoreCase) && Model.Contains(TEXT("Graser")))return AGraserTurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("LCaTurretBase"),ESearchCase::IgnoreCase))return ALCATurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("mine_base"),ESearchCase::IgnoreCase))return ABlockadeTurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("t2_base"),ESearchCase::IgnoreCase))return AOrca2TurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("t2_base_inv"),ESearchCase::IgnoreCase))return AOrca2InvertedTurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("Base1opt"),ESearchCase::IgnoreCase))return AVanguardTurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("graserbase"),ESearchCase::IgnoreCase))return AGraserTurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("AAA2Turret"),ESearchCase::IgnoreCase))return AAAATurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("AAA4Turret"),ESearchCase::IgnoreCase))return AQuadAAATurretBaseActor::StaticClass();
    if(Name.Equals(TEXT("SAMTurret"),ESearchCase::IgnoreCase))return ASAM1TurretBaseActor::StaticClass();
    return Model.IsEmpty()?nullptr:ATurretBaseActor::StaticClass();
}
TSubclassOf<ATurretBaseActor> TurretBaseCatalog::VisualClass(const FString& Model)
{
    UClass* Native=NativeClass(Model).Get();
    if(!Native || Native==ATurretBaseActor::StaticClass())return Native;
    const FString Name=Native->GetName().LeftChop(5);
    const FString Folder=Name.LeftChop(10); // TurretBase suffix.
    const FString Package=FString::Printf(TEXT("/Game/Models/Weapons/TurretBase/%s/BP_%s"),*Folder,*Name);
    if(FPackageName::DoesPackageExist(Package))
        if(UClass* BP=LoadClass<ATurretBaseActor>(nullptr,*(Package+TEXT(".BP_")+Name+TEXT("_C"))))return BP;
    return Native;
}
void TurretBaseCatalog::Definition(FName WeaponType,FString& Model,float& Scale)
{
    Model.Empty();Scale=1.f;
    if(const auto* Row=WeaponDesignRegistry::Find(WeaponType)){Model=Row->TurretBase;Scale=Row->Scale;return;}
    if(WeaponType==FName(TEXT("LCA Turret"))){Model=TEXT("LCaTurretBase");Scale=0.12f;return;}
    if(WeaponType==FName(TEXT("Blockade Cannon"))){Model=TEXT("mine_base");Scale=1.0f;return;}
    if(WeaponType==FName(TEXT("Orca 2 Cannon"))){Model=TEXT("t2_base");Scale=1.0f;return;}
    if(WeaponType==FName(TEXT("Orca 2 Inverted"))){Model=TEXT("t2_base_inv");Scale=1.0f;return;}
    if(WeaponType==FName(TEXT("Vanguard"))){Model=TEXT("Base1opt");Scale=1.2f;return;}
    if(WeaponType==FName(TEXT("Graser Turret"))){Model=TEXT("graserbase");Scale=2.0f;return;}
    if(WeaponType==FName(TEXT("AAA Cannon"))){Model=TEXT("AAA2Turret");Scale=2.0f;return;}
    if(WeaponType==FName(TEXT("Quad AAA"))){Model=TEXT("AAA4Turret");Scale=2.0f;return;}
    if(WeaponType==FName(TEXT("SAM-1"))){Model=TEXT("SAMTurret");Scale=2.0f;return;}
    if(UClass* Type=TurretCatalog::FindClass(WeaponType).Get())
    {
        const auto* Default=Type->GetDefaultObject<ATurretActor>();
        Model=Default->LegacyBaseModel;Scale=Default->LegacyModelScale;
    }
}
