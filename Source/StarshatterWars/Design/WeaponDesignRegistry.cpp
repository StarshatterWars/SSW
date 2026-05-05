#include "WeaponDesignRegistry.h"
#include "WeaponDesign.h"

#include "Logging/LogMacros.h"
#include "Math/UnrealMathUtility.h"

TMap<FName, FWeaponDesign> WeaponDesignRegistry::DesignsByName;
TMap<FName, WeaponDesign*> WeaponDesignRegistry::LegacyDesignsByName;

//-------------------------------------------------------------
// Clear
//-------------------------------------------------------------
void WeaponDesignRegistry::Clear()
{
    for (TPair<FName, WeaponDesign*>& Pair : LegacyDesignsByName)
    {
        delete Pair.Value;
    }

    LegacyDesignsByName.Empty();
    DesignsByName.Empty();
}

//-------------------------------------------------------------
// Vector validation
//-------------------------------------------------------------
bool WeaponDesignRegistry::IsBadVector(const FVector& V)
{
    return !FMath::IsFinite(V.X) ||
        !FMath::IsFinite(V.Y) ||
        !FMath::IsFinite(V.Z) ||
        FMath::Abs(V.X) > 1.0e9f ||
        FMath::Abs(V.Y) > 1.0e9f ||
        FMath::Abs(V.Z) > 1.0e9f;
}

//-------------------------------------------------------------
// Register design
//-------------------------------------------------------------
void WeaponDesignRegistry::RegisterDesign(const FName& RowName, const FWeaponDesign& Row)
{
    if (RowName.IsNone())
    {
        return;
    }

    FWeaponDesign Clean = Row;
    Clean.NormalizeFixedArrays();

    // Sanitize vectors
    for (FVector& V : Clean.MuzzlePoints)
    {
        if (IsBadVector(V))
        {
            V = FVector::ZeroVector;
        }
    }

    for (FVector& V : Clean.Attachments)
    {
        if (IsBadVector(V))
        {
            V = FVector::ZeroVector;
        }
    }

    //---------------------------------------------------------
    // Store UE version
    //---------------------------------------------------------
    DesignsByName.Add(RowName, Clean);

    if (!Clean.Name.IsEmpty())
    {
        DesignsByName.Add(FName(*Clean.Name), Clean);
    }

    //---------------------------------------------------------
    // Build legacy version
    //---------------------------------------------------------
    WeaponDesign* Legacy = ConvertToLegacyDesign(RowName, Clean);

    if (!Legacy)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[WeaponDesignRegistry] FAILED to build legacy WeaponDesign Row='%s' Name='%s'"),
            *RowName.ToString(),
            *Clean.Name);
        return;
    }

    //---------------------------------------------------------
    // Store legacy version
    //---------------------------------------------------------
    LegacyDesignsByName.Add(RowName, Legacy);

    if (!Clean.Name.IsEmpty())
    {
        LegacyDesignsByName.Add(FName(*Clean.Name), Legacy);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[WeaponDesignRegistry] Registered Row='%s' Name='%s' Legacy=%p"),
        *RowName.ToString(),
        *Clean.Name,
        Legacy);
}

void WeaponDesignRegistry::RegisterDesign(const FString& Name, const FWeaponDesign& Row)
{
    if (!Name.IsEmpty())
    {
        RegisterDesign(FName(*Name), Row);
    }
}

//-------------------------------------------------------------
// Find UE
//-------------------------------------------------------------
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

//-------------------------------------------------------------
// Find legacy
//-------------------------------------------------------------
WeaponDesign* WeaponDesignRegistry::FindLegacy(const FName& Name)
{
    return Name.IsNone() ? nullptr : LegacyDesignsByName.FindRef(Name);
}

WeaponDesign* WeaponDesignRegistry::FindLegacy(const FString& Name)
{
    return Name.IsEmpty() ? nullptr : LegacyDesignsByName.FindRef(FName(*Name));
}

WeaponDesign* WeaponDesignRegistry::FindLegacy(const char* Name)
{
    if (!Name || !Name[0])
    {
        return nullptr;
    }

    return LegacyDesignsByName.FindRef(FName(ANSI_TO_TCHAR(Name)));
}

//-------------------------------------------------------------
// Access
//-------------------------------------------------------------
const TMap<FName, FWeaponDesign>& WeaponDesignRegistry::GetAll()
{
    return DesignsByName;
}

//-------------------------------------------------------------
// Convert to legacy WeaponDesign
//-------------------------------------------------------------
WeaponDesign* WeaponDesignRegistry::ConvertToLegacyDesign(
    const FName& RowName,
    const FWeaponDesign& Row)
{
    WeaponDesign* Legacy = new WeaponDesign();

    if (!Legacy)
    {
        return nullptr;
    }

    //---------------------------------------------------------
    // Identity
    //---------------------------------------------------------
    Legacy->name = TCHAR_TO_ANSI(*Row.Name);
    Legacy->group = TCHAR_TO_ANSI(*Row.Group);

    //---------------------------------------------------------
    // Flags
    //---------------------------------------------------------
    Legacy->primary = Row.bPrimary;
    Legacy->beam = Row.bBeam;
    Legacy->flak = Row.bFlak;
    Legacy->guided = Row.Guided;
    Legacy->self_aiming = Row.bSelfAiming;

    //---------------------------------------------------------
    // Stats
    //---------------------------------------------------------
    Legacy->damage = Row.Damage;
    Legacy->speed = Row.Speed;
    Legacy->life = Row.Life;
    Legacy->mass = Row.Mass;
    Legacy->drag = Row.Drag;

    Legacy->min_range = Row.MinRange;
    Legacy->max_range = Row.MaxRange;

    //---------------------------------------------------------
    // Firing
    //---------------------------------------------------------
    Legacy->refire_delay = Row.RefireDelay;
    Legacy->recharge_rate = Row.RechargeRate;
    Legacy->ammo = Row.Ammo;
    Legacy->capacity = Row.Capacity;

    //---------------------------------------------------------
    // Aim limits
    //---------------------------------------------------------
    Legacy->aim_az_max = Row.AimAzMax;
    Legacy->aim_az_min = Row.AimAzMin;
    Legacy->aim_el_max = Row.AimElMax;
    Legacy->aim_el_min = Row.AimElMin;

    //---------------------------------------------------------
    // Geometry
    //---------------------------------------------------------
    for (int32 i = 0; i < Row.MuzzlePoints.Num(); i++)
    {
        Legacy->muzzle_pts[i] = Row.MuzzlePoints[i];
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[WeaponDesignRegistry] Legacy built Row='%s' Name='%s' Damage=%.1f Speed=%.1f"),
        *RowName.ToString(),
        *Row.Name,
        Row.Damage,
        Row.Speed);

    return Legacy;
}