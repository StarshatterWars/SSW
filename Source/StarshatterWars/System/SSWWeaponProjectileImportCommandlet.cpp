#include "SSWWeaponProjectileImportCommandlet.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
USSWWeaponProjectileImportCommandlet::USSWWeaponProjectileImportCommandlet(){IsClient=true;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 USSWWeaponProjectileImportCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    auto* Imported=NewObject<UDataTable>();Imported->RowStruct=FWeaponDesign::StaticStruct();
    const FString Directory = FPaths::ProjectContentDir() / TEXT("GameData/Weapons");
    const FString JsonPath = Directory / TEXT("wep.json");
    if (FParse::Param(*Params, TEXT("ExportJson")))
    {
        auto* Importer = NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());
        Importer->WeaponDesignDataTable = Imported;
        // The former boot path overlaid DEF rows on the authored table.
        // Preserve any designs that exist only in that table during migration.
        if (const auto* Existing = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign")))
            Existing->ForeachRow<FWeaponDesign>(TEXT("Weapon catalog migration"), [Imported](FName, const FWeaponDesign& Design)
            {
                const FString Name = Design.Name.TrimStartAndEnd();
                if (!Name.IsEmpty()) Imported->AddRow(FName(*Name), Design);
            });

        const auto LegacyPath = StringCast<ANSICHAR>(*(Directory / TEXT("Wep.def")));
        Importer->LoadWeaponDesign(LegacyPath.Get());
        if (Imported->GetRowMap().IsEmpty()) return 1;

        Imported->ImportKeyField = TEXT("Row");
        const FString DefaultJson = Imported->GetTableAsJSON(EDataTableExportFlags::UseJsonObjectsForStructs);
        TArray<TSharedPtr<FJsonValue>> JsonRows;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(DefaultJson), JsonRows)) return 2;

        // The stock DataTable writer emits float fields with six significant digits.
        // Write those values as doubles to preserve the DEF importer's float bits.
        for (const auto& JsonRow : JsonRows)
        {
            const auto& Object = JsonRow->AsObject();
            const auto* Original = Imported->FindRowUnchecked(FName(*Object->GetStringField(TEXT("Row"))));
            if (!Original) return 2;
            for (TFieldIterator<FFloatProperty> Property(FWeaponDesign::StaticStruct()); Property; ++Property)
                Object->SetNumberField(Property->GetName(), Property->GetFloatingPointPropertyValue(
                    Property->ContainerPtrToValuePtr<void>(Original)));
        }

        FString Json;
        auto Writer = TJsonWriterFactory<>::Create(&Json);
        if (!FJsonSerializer::Serialize(JsonRows, Writer)) return 2;
        const FString VerificationPath = FPaths::ProjectSavedDir() / TEXT("WeaponJsonRoundTrip.json");
        if (!FFileHelper::SaveStringToFile(Json, *VerificationPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return 2;

        auto* RoundTrip = NewObject<UDataTable>();
        if (!UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*RoundTrip, VerificationPath) ||
            RoundTrip->GetRowMap().Num() != Imported->GetRowMap().Num()) return 2;

        for (const auto& Pair : Imported->GetRowMap())
        {
            const void* Copy = RoundTrip->FindRowUnchecked(Pair.Key);
            if (!Copy || !FWeaponDesign::StaticStruct()->CompareScriptStruct(Pair.Value, Copy, 0))
            {
                UE_LOG(LogTemp, Error, TEXT("Weapon JSON round trip changed row '%s'"), *Pair.Key.ToString());
                if (Copy)
                    for (TFieldIterator<FProperty> Property(FWeaponDesign::StaticStruct()); Property; ++Property)
                        if (!Property->Identical_InContainer(Pair.Value, Copy))
                            UE_LOG(LogTemp, Error, TEXT("Changed field: %s"), *Property->GetName());
                return 3;
            }
        }
        if (!FFileHelper::SaveStringToFile(Json, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return 4;
        UE_LOG(LogTemp, Display, TEXT("Verified and exported %d weapon designs to %s"), Imported->GetRowMap().Num(), *JsonPath);
        return 0;
    }
    if (!UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*Imported, JsonPath)) return 1;
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!Table)return 1;
    int32 Count=0;
    for(const auto& Pair:Imported->GetRowMap())
    {
        const auto& Source=*reinterpret_cast<const FWeaponDesign*>(Pair.Value);

        FWeaponDesign* Dest=nullptr;
        for(const auto& Target:Table->GetRowMap())
        {
            auto* Candidate=reinterpret_cast<FWeaponDesign*>(Target.Value);
            if(Candidate->Name==Source.Name){Dest=Candidate;break;}
        }
        if (Dest) *Dest = Source;
        else Table->AddRow(Pair.Key, Source);
        ++Count;
    }
    if(Count!=Imported->GetRowMap().Num() || Count==0)return 3;
    Table->MarkPackageDirty();
    const FString Filename=FPackageName::LongPackageNameToFilename(Table->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    if(!UPackage::SavePackage(Table->GetOutermost(),Table,*Filename,Args))return 4;
    UE_LOG(LogTemp,Display,TEXT("Saved %d complete weapon designs from wep.json."),Count);
    return 0;
#else
    return 1;
#endif
}
