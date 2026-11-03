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
    // Row keys and display-name aliases may refer to the same owned design.
    TSet<WeaponDesign*> Deleted;
    for (TPair<FName, WeaponDesign*>& Pair : LegacyDesignsByName)
    {
        if (Pair.Value && !Deleted.Contains(Pair.Value))
        {
            Deleted.Add(Pair.Value);
            delete Pair.Value;
        }
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
    // Every parsed value is retained; no axis or angle conversion occurs here.
    Legacy->type = Row.Type;
    Legacy->name = TCHAR_TO_ANSI(*Row.Name);
    Legacy->group = TCHAR_TO_ANSI(*Row.Group);
    Legacy->description = TCHAR_TO_ANSI(*Row.Description);
    Legacy->secret = Row.bSecret;
    Legacy->drone = Row.bDrone;
    Legacy->primary = Row.bPrimary;
    Legacy->beam = Row.bBeam;
    Legacy->flak = Row.bFlak;
    Legacy->guided = Row.Guided;
    Legacy->self_aiming = Row.bSelfAiming;
    Legacy->syncro = Row.bSyncro;
    Legacy->value = Row.Value;
    Legacy->decoy_type = Row.DecoyType;
    Legacy->probe = Row.bProbe;
    Legacy->target_type = Row.TargetType;
    Legacy->visible_stores = Row.bVisibleStores;
    Legacy->nstores = FMath::Clamp(Row.NStores,0,int32(WeaponDesign::MAX_STORES));
    Legacy->nbarrels = FMath::Clamp(Row.NBarrels,0,int32(WeaponDesign::MAX_STORES));
    Legacy->recharge_rate = Row.RechargeRate;
    Legacy->refire_delay = Row.RefireDelay;
    Legacy->salvo_delay = Row.SalvoDelay;
    Legacy->charge = Row.Charge;
    Legacy->min_charge = Row.MinCharge;
    Legacy->ammo = Row.Ammo;
    Legacy->ripple_count = Row.RippleCount;
    Legacy->capacity = Row.Capacity;
    Legacy->carry_mass = Row.CarryMass;
    Legacy->carry_resist = Row.CarryResist;
    Legacy->damage = Row.Damage;
    Legacy->damage_type = Row.DamageType;
    Legacy->penetration = Row.Penetration;
    Legacy->lethal_radius = Row.LethalRadius;
    Legacy->integrity = Row.Integrity;
    Legacy->speed = Row.Speed;
    Legacy->life = Row.Life;
    Legacy->mass = Row.Mass;
    Legacy->drag = Row.Drag;
    Legacy->thrust = Row.Thrust;
    Legacy->roll_rate = Row.RollRate;
    Legacy->pitch_rate = Row.PitchRate;
    Legacy->yaw_rate = Row.YawRate;
    Legacy->roll_drag = Row.RollDrag;
    Legacy->pitch_drag = Row.PitchDrag;
    Legacy->yaw_drag = Row.YawDrag;
    Legacy->min_range = Row.MinRange;
    Legacy->max_range = Row.MaxRange;
    Legacy->max_track = Row.MaxTrack;
    Legacy->graphic_type = Row.GraphicType;
    Legacy->width = Row.Width;
    Legacy->length = Row.Length;
    Legacy->scale = Row.Scale;
    Legacy->explosion_scale = Row.ExplosionScale;
    Legacy->light = Row.Light;
    Legacy->light_color = Row.LightColor;
    Legacy->flash_scale = Row.FlashScale;
    Legacy->flare_scale = Row.FlareScale;
    Legacy->trail_length = static_cast<int>(Row.TrailLength);
    Legacy->trail_width = Row.TrailWidth;
    Legacy->trail_dim = static_cast<int>(Row.TrailDim);
    Legacy->beauty = TCHAR_TO_ANSI(*Row.Beauty);
    Legacy->bitmap = TCHAR_TO_ANSI(*Row.Bitmap);
    Legacy->turret = TCHAR_TO_ANSI(*Row.Turret);
    Legacy->turret_base = TCHAR_TO_ANSI(*Row.TurretBase);
    Legacy->model = TCHAR_TO_ANSI(*Row.Model);
    Legacy->projectile = TCHAR_TO_ANSI(*Row.Projectile);
    Legacy->trail = TCHAR_TO_ANSI(*Row.Trail);
    Legacy->flash = TCHAR_TO_ANSI(*Row.Flash);
    Legacy->flare = TCHAR_TO_ANSI(*Row.Flare);
    Legacy->sound = TCHAR_TO_ANSI(*Row.Sound);
    Legacy->sound_min_distance = Row.SoundMinDistance;
    Legacy->sound_max_distance = Row.SoundMaxDistance;
    Legacy->anim_length = FMath::Clamp(Row.AnimLength,0,FMath::Min(Row.AnimFrames.Num(),16));
    Legacy->firing_cone = Row.FiringCone;
    Legacy->aim_az_max = Row.AimAzMax;
    Legacy->aim_az_min = Row.AimAzMin;
    Legacy->aim_az_rest = Row.AimAzRest;
    Legacy->aim_el_max = Row.AimElMax;
    Legacy->aim_el_min = Row.AimElMin;
    Legacy->aim_el_rest = Row.AimElRest;
    Legacy->slew_rate = Row.SlewRate;
    Legacy->turret_axis = Row.TurretAxis;
    Legacy->spread_az = Row.SpreadAz;
    Legacy->spread_el = Row.SpreadEl;
    Legacy->det_range = Row.DetRange;
    Legacy->det_count = Row.DetCount;
    Legacy->det_spread = Row.DetSpread;
    Legacy->det_child = TCHAR_TO_ANSI(*Row.DetChild);
    Legacy->eject = Row.Eject;
    for(int32 I=0;I<FMath::Min(Row.MuzzlePoints.Num(),int32(WeaponDesign::MAX_STORES));++I)
        Legacy->muzzle_pts[I]=Row.MuzzlePoints[I];
    for(int32 I=0;I<FMath::Min(Row.Attachments.Num(),int32(WeaponDesign::MAX_STORES));++I)
        Legacy->attachments[I]=Row.Attachments[I];
    for(int32 I=0;I<FMath::Min(Row.AnimFrames.Num(),16);++I)
        Legacy->anim_frames[I]=TCHAR_TO_ANSI(*Row.AnimFrames[I]);
    return Legacy;
}
