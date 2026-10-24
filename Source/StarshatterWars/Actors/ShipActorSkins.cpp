#include "ShipActor.h"
#include "Skin.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "GameStructs_System.h"
#include "Polygon.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"

void AShipActor::RestoreHullSkin()
{
    if(HullMesh) for(const auto& Pair:OriginalHullSkinMaterials)
        HullMesh->SetMaterial(Pair.Key,PresentedSkinMesh.Get()==HullMesh->GetStaticMesh()?Pair.Value.Get():nullptr);
    OriginalHullSkinMaterials.Reset();PresentedHullSkin=nullptr;PresentedSkinMesh.Reset();
}
void AShipActor::RefreshHullSkin() { RestoreHullSkin();SyncHullSkin(); }
void AShipActor::SyncHullSkin()
{
    const Skin* Selected=RuntimeShip?RuntimeShip->GetSkin():nullptr;
    if(Selected!=PresentedHullSkin || (HullMesh && HullMesh->GetStaticMesh()!=PresentedSkinMesh.Get())) ApplyHullSkin(Selected);
}
UTexture2D* AShipActor::ResolveHullSkinTexture(const FString& TextureName,const FString& SourceFile)
{
    if(TextureName.IsEmpty()) return nullptr;
    const FString Name=FPaths::GetBaseFilename(TextureName);
    if(const auto* Override=HullSkinTextureOverrides.Find(FName(*Name))) return Override->Get();
    if(TextureName.StartsWith(TEXT("/Game/"))) return LoadObject<UTexture2D>(nullptr,*TextureName);
    // Imported assets are selected only when the source name is unambiguous within this hull's folder.
    FString Design=RuntimeShip&&RuntimeShip->Design()?ANSI_TO_TCHAR(RuntimeShip->Design()->name):FPaths::GetBaseFilename(SourceFile);
    if(!Design.IsEmpty())
    {
        FARFilter Filter;Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(FName(*(TEXT("/Game/Models/")+Design)));Filter.bRecursivePaths=true;
        TArray<FAssetData> Assets;FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter,Assets);
        UTexture2D* Found=nullptr;
        for(const auto& Asset:Assets) if(Asset.AssetName==FName(*Name))
        { if(Found) return nullptr;Found=Cast<UTexture2D>(Asset.GetAsset()); }
        if(Found) return Found;
    }
    // F10 supports raw staged legacy bitmap files as a rendering fallback.
    if(DataLoader* Loader=DataLoader::GetLoader())
    {
        const FString OldPath=ANSI_TO_TCHAR(Loader->GetDataPath());
        Loader->SetDataPath("");Bitmap* Image=nullptr;
        TArray<FString> Candidates;
        Candidates.Add(FPaths::ProjectContentDir()/TEXT("GameData/Ships")/TextureName);
        Candidates.Add(FPaths::ProjectContentDir()/TEXT("GameData")/FPaths::GetPath(SourceFile)/TextureName);
        for(const FString& Candidate:Candidates)
        {
            for(const TCHAR* Extension:{TEXT(""),TEXT(".pcx"),TEXT(".bmp"),TEXT(".png"),TEXT(".jpg")})
            {
                const FString File=Candidate+Extension;
                if(IFileManager::Get().FileExists(*File)) {Loader->LoadTexture(TCHAR_TO_ANSI(*File),Image);break;}
            }
            if(Image) break;
        }
        Loader->SetDataPath(TCHAR_TO_ANSI(*OldPath));
        if(Image) return Image->GetTexture();
    }
    return nullptr;
}
void AShipActor::ApplyHullSkin(const Skin* Selected)
{
    RestoreHullSkin();
    if(!HullMesh || !HullMesh->GetStaticMesh()) return;
    PresentedHullSkin=Selected;PresentedSkinMesh=HullMesh->GetStaticMesh();
    if(!Selected) return;
    const FShipSkin* Definition=Selected->GetNativeDefinition();
    const auto& Slots=HullMesh->GetStaticMesh()->GetStaticMaterials();
    for(int32 Index=0;Index<HullMesh->GetNumMaterials();++Index)
    {
        UMaterialInterface* Original=HullMesh->GetMaterial(Index);if(!Original) continue;
        FName Slot=Slots.IsValidIndex(Index)?Slots[Index].MaterialSlotName:NAME_None;
        FName LegacyName=HullSkinSlotAliases.Contains(Slot)?HullSkinSlotAliases[Slot]:Slot;
        const FSkinMtlCell* NativeCell=nullptr;const Material* LegacyCell=nullptr;
        if(Definition) for(const auto& Cell:Definition->Cells)
            if((Cell.CellIndex!=INDEX_NONE&&Cell.CellIndex==Index) ||
               (!Cell.MaterialName.IsEmpty() && (FName(*Cell.MaterialName)==LegacyName ||
                (Slots.IsValidIndex(Index)&&FName(*Cell.MaterialName)==Slots[Index].ImportedMaterialSlotName))))
                {NativeCell=&Cell;LegacyName=FName(*Cell.MaterialName);break;}
        if(!Definition) for(int32 I=0;I<Selected->NumCells();++I)
            if(const SkinCell* Cell=Selected->GetCell(I))
                if(FName(ANSI_TO_TCHAR(Cell->Name()))==LegacyName) {LegacyCell=Cell->Skin();break;}
        if(!NativeCell && !LegacyCell) continue;
        OriginalHullSkinMaterials.Add(Index,Original);
        const FName OverrideKey(*(FString(ANSI_TO_TCHAR(Selected->Name()))+TEXT(".")+LegacyName.ToString()));
        if(const auto* Override=HullSkinMaterialOverrides.Find(OverrideKey))
            if(Override->Get()) {HullMesh->SetMaterial(Index,Override->Get());continue;}
        auto* MID=UMaterialInstanceDynamic::Create(Original,this);if(!MID) continue;
        HullMesh->SetMaterial(Index,MID);
        auto Texture=[&](FName Param,const FString& Name,Bitmap* LegacyBitmap)
        { UTexture2D* Image=LegacyBitmap?LegacyBitmap->GetTexture():ResolveHullSkinTexture(Name,Definition?Definition->SourceFile:FString());
          if(Image) MID->SetTextureParameterValue(Param,Image); };
        if(NativeCell)
        {
            Texture(TEXT("BaseColorTexture"),NativeCell->TextureName,nullptr);
            Texture(TEXT("SpecularTexture"),NativeCell->SpecularTextureName.IsEmpty()?NativeCell->AuxTextureName:NativeCell->SpecularTextureName,nullptr);
            Texture(TEXT("BumpTexture"),NativeCell->BumpTextureName,nullptr);
            Texture(TEXT("EmissiveTexture"),NativeCell->EmissiveTextureName,nullptr);
            if(NativeCell->bHasAmbientColor) MID->SetVectorParameterValue(TEXT("AmbientColor"),NativeCell->AmbientColor);
            if(NativeCell->bHasDiffuseColor) MID->SetVectorParameterValue(TEXT("BaseColor"),NativeCell->DiffuseColor);
            if(NativeCell->bHasSpecularColor) MID->SetVectorParameterValue(TEXT("SpecularColor"),NativeCell->SpecularColor);
            if(NativeCell->bHasEmissiveColor) MID->SetVectorParameterValue(TEXT("EmissiveColor"),NativeCell->EmissiveColor);
            if(NativeCell->bHasSpecularPower) MID->SetScalarParameterValue(TEXT("SpecularPower"),NativeCell->SpecularPower);
            if(NativeCell->bHasBumpScale) MID->SetScalarParameterValue(TEXT("BumpScale"),NativeCell->BumpScale);
        }
        else
        {
            Texture(TEXT("BaseColorTexture"),FString(),LegacyCell->tex_diffuse);
            Texture(TEXT("SpecularTexture"),FString(),LegacyCell->tex_specular);
            Texture(TEXT("BumpTexture"),FString(),LegacyCell->tex_bumpmap);
            Texture(TEXT("EmissiveTexture"),FString(),LegacyCell->tex_emissive);
            MID->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor::FromSRGBColor(LegacyCell->Kd));
            MID->SetVectorParameterValue(TEXT("AmbientColor"),FLinearColor::FromSRGBColor(LegacyCell->Ka));
            MID->SetVectorParameterValue(TEXT("SpecularColor"),FLinearColor::FromSRGBColor(LegacyCell->Ks));
            MID->SetVectorParameterValue(TEXT("EmissiveColor"),FLinearColor::FromSRGBColor(LegacyCell->Ke));
            MID->SetScalarParameterValue(TEXT("SpecularPower"),LegacyCell->power);
            MID->SetScalarParameterValue(TEXT("BumpScale"),LegacyCell->bump);
        }
    }
}
