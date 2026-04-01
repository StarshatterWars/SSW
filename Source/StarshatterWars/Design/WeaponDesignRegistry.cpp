#include "WeaponDesignRegistry.h"

TMap<FName, FWeaponDesign> WeaponDesignRegistry::DesignsByName;

void WeaponDesignRegistry::Clear()
{
    DesignsByName.Empty();
}

void WeaponDesignRegistry::RegisterDesign(const FName& RowName, const FWeaponDesign& Row)
{
    if (!RowName.IsNone())
    {
        DesignsByName.Add(RowName, Row);
    }

    if (!Row.Name.IsEmpty())
    {
        DesignsByName.Add(FName(*Row.Name), Row);
    }

    if (!Row.Group.IsEmpty())
    {
        DesignsByName.Add(FName(*Row.Group), Row);
    }
}

void WeaponDesignRegistry::RegisterDesign(const FString& Name, const FWeaponDesign& Row)
{
    if (!Name.IsEmpty())
    {
        DesignsByName.Add(FName(*Name), Row);
    }

    if (!Row.Name.IsEmpty())
    {
        DesignsByName.Add(FName(*Row.Name), Row);
    }

    if (!Row.Group.IsEmpty())
    {
        DesignsByName.Add(FName(*Row.Group), Row);
    }
}

const FWeaponDesign* WeaponDesignRegistry::Find(const FName& Name)
{
    return Name.IsNone() ? nullptr : DesignsByName.Find(Name);
}

const FWeaponDesign* WeaponDesignRegistry::Find(const FString& Name)
{
    return Name.IsEmpty() ? nullptr : DesignsByName.Find(FName(*Name));
}

const FWeaponDesign* WeaponDesignRegistry::Find(const char* Name)
{
    if (!Name || !Name[0])
    {
        return nullptr;
    }

    return DesignsByName.Find(FName(ANSI_TO_TCHAR(Name)));
}

const TMap<FName, FWeaponDesign>& WeaponDesignRegistry::GetAll()
{
    return DesignsByName;
}