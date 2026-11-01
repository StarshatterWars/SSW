#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"

namespace WeaponBlueprintResolver
{
    inline UClass* Resolve(const FString& Model, const FString& WeaponName)
    {
        for (const FString& Source : {Model, WeaponName})
        {
            const FString Name = FPaths::GetBaseFilename(Source);
            if (Name.IsEmpty() || Name.Contains(TEXT(" "))) continue;
            const FString Package = FString::Printf(TEXT("/Game/Models/Weapons/%s/BP_%s"), *Name, *Name);
            if (!FPackageName::DoesPackageExist(Package)) continue;
            const FString ClassPath = Package + TEXT(".BP_") + Name + TEXT("_C");
            if (UClass* Class = LoadClass<AActor>(nullptr, *ClassPath)) return Class;
        }
        return nullptr;
    }
}
