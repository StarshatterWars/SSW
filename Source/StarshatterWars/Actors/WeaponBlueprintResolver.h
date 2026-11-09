#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"

namespace WeaponBlueprintResolver
{
    inline FString ProjectileClassPath(const FString& Reference)
    {
        FString Path=FPackageName::ExportTextPathToObjectPath(Reference.TrimStartAndEnd());
        if (Path.IsEmpty()) return FString();
        if (!Path.StartsWith(TEXT("/")))
        {
            FString Name=Path;
            Name.RemoveFromStart(TEXT("BP_")); Name.RemoveFromEnd(TEXT("_C"));
            const FString Folder=Name.EndsWith(TEXT("Beam"))
                ? TEXT("/Game/Models/Weapons/Beams/")+Name.LeftChop(4)
                : Name.EndsWith(TEXT("Bolt"))
                ? TEXT("/Game/Models/Weapons/Bolt/")+Name.LeftChop(4)
                : TEXT("/Game/Models/Weapons/")+Name;
            Path=Folder+TEXT("/BP_")+Name;
        }
        if (!Path.Contains(TEXT("."))) Path+=TEXT(".")+FPaths::GetBaseFilename(Path);
        if (!Path.EndsWith(TEXT("_C"))) Path+=TEXT("_C");
        return Path;
    }
    inline UClass* ResolveProjectile(const FString& Reference)
    {
        const FString Path=ProjectileClassPath(Reference);
        if (Path.IsEmpty()) return nullptr;
        if (UClass* Loaded = FindObject<UClass>(nullptr, *Path); Loaded && Loaded->IsChildOf(AActor::StaticClass()))
            return Loaded;
        return LoadClass<AActor>(nullptr, *Path);
    }

    inline UClass* Resolve(const FString& Model, const FString& WeaponName)
    {
        for (const FString& Source : {Model, WeaponName})
        {
            const FString Name = FPaths::GetBaseFilename(Source);
            if (Name.IsEmpty() || Name.Contains(TEXT(" "))) continue;
            const FString Package = FString::Printf(TEXT("/Game/Models/Weapons/%s/BP_%s"), *Name, *Name);
            const FString ClassPath = Package + TEXT(".BP_") + Name + TEXT("_C");
            if (UClass* Loaded = FindObject<UClass>(nullptr, *ClassPath); Loaded && Loaded->IsChildOf(AActor::StaticClass()))
                return Loaded;
            if (!FPackageName::DoesPackageExist(Package)) continue;
            if (UClass* Class = LoadClass<AActor>(nullptr, *ClassPath)) return Class;
        }
        return nullptr;
    }
}
