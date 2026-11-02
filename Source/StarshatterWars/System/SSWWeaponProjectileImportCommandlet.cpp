#include "SSWWeaponProjectileImportCommandlet.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
USSWWeaponProjectileImportCommandlet::USSWWeaponProjectileImportCommandlet(){IsClient=true;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 USSWWeaponProjectileImportCommandlet::Main(const FString&)
{
#if WITH_EDITOR
    auto* Imported=NewObject<UDataTable>();Imported->RowStruct=FWeaponDesign::StaticStruct();
    auto* Importer=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());Importer->WeaponDesignDataTable=Imported;
    const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");
    const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadWeaponDesign(Ansi.Get());
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!Table)return 1;
    int32 Count=0;
    for(const auto& Pair:Imported->GetRowMap())
    {
        const auto& Source=*reinterpret_cast<const FWeaponDesign*>(Pair.Value);
        if(Source.Projectile.IsEmpty())continue;
        FWeaponDesign* Dest=nullptr;
        for(const auto& Target:Table->GetRowMap())
        {
            auto* Candidate=reinterpret_cast<FWeaponDesign*>(Target.Value);
            if(Candidate->Name==Source.Name){Dest=Candidate;break;}
        }
        if(!Dest){UE_LOG(LogTemp,Error,TEXT("Missing weapon row %s"),*Source.Name);return 2;}
        Dest->Projectile=Source.Projectile; // Preserve every other imported gameplay/model value.
        ++Count;
    }
    if(Count!=24){UE_LOG(LogTemp,Error,TEXT("Expected 24 bolt references; found %d"),Count);return 3;}
    Table->MarkPackageDirty();
    const FString Filename=FPackageName::LongPackageNameToFilename(Table->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    if(!UPackage::SavePackage(Table->GetOutermost(),Table,*Filename,Args))return 4;
    UE_LOG(LogTemp,Display,TEXT("Saved %d projectile fields; all other weapon fields preserved."),Count);
    return 0;
#else
    return 1;
#endif
}
