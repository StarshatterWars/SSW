#pragma once

#include "CoreMinimal.h"
#include "GameStructs_System.h"

class STARSHATTERWARS_API WeaponDesignRegistry
{
public:
    static void Clear();

    static void RegisterDesign(const FName& RowName, const FWeaponDesign& Row);
    static void RegisterDesign(const FString& Name, const FWeaponDesign& Row);

    static const FWeaponDesign* Find(const FName& Name);
    static const FWeaponDesign* Find(const FString& Name);
    static const FWeaponDesign* Find(const char* Name);

    static const TMap<FName, FWeaponDesign>& GetAll();

private:
    static TMap<FName, FWeaponDesign> DesignsByName;
};