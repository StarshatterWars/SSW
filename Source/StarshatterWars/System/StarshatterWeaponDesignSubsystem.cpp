/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterWeaponDesignSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Parses Content/GameData/Weapons/wep.def (WEAPON) and fills:
    - WeaponsByName runtime cache
    - Optional WeaponDesignDataTable via AddRow/RemoveRow
*/

#include "StarshatterWeaponDesignSubsystem.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Engine/DataTable.h"

#include "StarshatterAssetRegistrySubsystem.h"

// If you have constants like DEGREES (rad-per-degree), include where defined:
#include "FormatUtil.h" // for DEGREES if that’s where you defined it; otherwise move include appropriately.

// ---------------------------------------------------------------------
// Local helpers
// ---------------------------------------------------------------------

static EWeaponType WeaponKindFromDefName(const Text& DefName)
{
    // defname already case-insensitive in legacy; we mimic by comparing normalized text
    Text D = DefName;
    D.setSensitive(false);

    if (D == "primary") return EWeaponType::Primary;
    if (D == "missile") return EWeaponType::Missile;
    if (D == "drone")   return EWeaponType::Drone;
    if (D == "beam")    return EWeaponType::Beam;
    return EWeaponType::Unknown;
}

static bool IsWeaponRootDef(const Text& DefName)
{
    Text D = DefName;
    D.setSensitive(false);
    return (D == "primary" || D == "missile" || D == "drone" || D == "beam");
}

static void ClampStores(FWeaponDesign& W)
{
    // Legacy fixed MAX_STORES=16; keep counters sane.
#ifndef SSW_MAX_STORES
#define SSW_MAX_STORES 16
#endif
    W.NStores = FMath::Clamp(W.NStores, 0, SSW_MAX_STORES);
    W.NBarrels = FMath::Clamp(W.NBarrels, 0, SSW_MAX_STORES);

    // Force fixed arrays for tooling/runtime consistency:
    W.MuzzlePoints.SetNum(SSW_MAX_STORES);
    W.Attachments.SetNum(SSW_MAX_STORES);
}

// ---------------------------------------------------------------------
// Subsystem
// ---------------------------------------------------------------------

void UStarshatterWeaponDesignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("[WEAPONDESIGN] Initialize()"));

    /*------------------------------------------------------------------
    Resolve Ship Design DataTable via Asset Registry
------------------------------------------------------------------*/

    UStarshatterAssetRegistrySubsystem* Assets =
        GetGameInstance()->GetSubsystem<UStarshatterAssetRegistrySubsystem>();

    if (!Assets)
    {
        UE_LOG(LogTemp, Error, TEXT("[WEAPONDESIGN] Asset Registry missing"));
        return;
    }

    WeaponDesignDataTable =
        Assets->GetDataTable(TEXT("Data.WeaponDesignTable"), true);

    if (!WeaponDesignDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("[WEAPONDESIGN] WeaponDesignTable not found"));
        return;
    }

    if (bClearTables)
    {
        WeaponDesignDataTable->EmptyTable();
    }
}

void UStarshatterWeaponDesignSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("[WEAPON DESIGN] Deinitialize()"));
    DesignsByName.Empty();
    Super::Deinitialize();
}

void UStarshatterWeaponDesignSubsystem::LoadAll(bool bFull)
{
    UE_LOG(LogTemp, Log, TEXT("UStarshatterWeaponDesignSubsystem::LoadAll()"));
    if (!bFull)
        return;

    LoadWeaponDesigns();
}

void UStarshatterWeaponDesignSubsystem::LoadWeaponDesigns()
{
    // Adjust if your path differs:
    const FString WepDefPath = FPaths::ProjectContentDir() / TEXT("GameData/Weapons/wep.def");
    const FTCHARToUTF8 Utf8(*WepDefPath);
    LoadWeaponDesign(Utf8.Get());
}

void UStarshatterWeaponDesignSubsystem::LoadWeaponDesign(const char* Filename)
{
    if (!Filename || !*Filename)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WEAPON] LoadWeaponDesign: null/empty filename"));
        return;
    }

    const FString FilePath = ANSI_TO_TCHAR(Filename);
    UE_LOG(LogTemp, Log, TEXT("[WEAPON] Loading Weapon Designs '%s'"), *FilePath);

    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[WEAPON] file not found: %s"), *FilePath);
        return;
    }

    if (bClearTables)
    {
        DesignsByName.Empty();
        UE_LOG(LogTemp, Log, TEXT("[WEAPON] bClearTables=true: cleared in-memory cache (DT rows overwritten as parsed)"));
    }

    // Load file into bytes for legacy parser (char* buffer expected)
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("[WEAPON] failed to read: %s"), *FilePath);
        return;
    }
    Bytes.Add(0);

    // Stable UTF-8 filename for legacy helpers
    const FTCHARToUTF8 Utf8Path(*FilePath);
    const char* fn = Utf8Path.Get();

    Parser ParserObj(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
    Term* TermObj = ParserObj.ParseTerm();

    if (!TermObj)
    {
        UE_LOG(LogTemp, Error, TEXT("[WEAPON] ERROR: could not parse '%s'"), *FilePath);
        return;
    }

    // Header check
    {
        TermText* FileType = TermObj->isText();
        if (!FileType || FileType->value() != "WEAPON")
        {
            UE_LOG(LogTemp, Error, TEXT("[WEAPON] ERROR: invalid weapon design file '%s' (expected WEAPON)"), *FilePath);
            delete TermObj;
            return;
        }
    }

    // Legacy auto-increment:
    int32 NextType = 1;

    // Legacy global degrees toggle (per-file semantics):
    bool bDegrees = false;

    int32 ParsedWeapons = 0;

    do
    {
        delete TermObj;
        TermObj = ParserObj.ParseTerm();

        if (!TermObj)
            break;

        TermDef* Def = TermObj->isDef();
        if (!Def)
            continue;

        Text DefName = Def->name()->value();
        DefName.setSensitive(false);

        if (IsWeaponRootDef(DefName))
        {
            if (!Def->term() || !Def->term()->isStruct())
            {
                UE_LOG(LogTemp, Warning, TEXT("[WEAPON] WARNING: weapon struct missing in '%s'"), *FilePath);
                continue;
            }

            TermStruct* Val = Def->term()->isStruct();

            FWeaponDesign W;
            W.SourceFile = FilePath;

            W.Type = NextType++;
            W.Kind = WeaponKindFromDefName(DefName);

            // Legacy defaults by kind:
            if (W.Kind == EWeaponType::Primary)
            {
                W.bPrimary = true;
            }
            else if (W.Kind == EWeaponType::Beam)
            {
                W.bPrimary = true;
                W.bBeam = true;
                W.Guided = 1;

                W.SpreadAz = 0.15f;
                W.SpreadEl = 0.15f;
            }
            else if (W.Kind == EWeaponType::Drone)
            {
                W.bDrone = true;
                W.Penetration = 5.0f;
            }
            else if (W.Kind == EWeaponType::Missile)
            {
                W.Penetration = 5.0f;
            }

            // Legacy fixed arrays:
            W.MuzzlePoints.SetNum(16);
            W.Attachments.SetNum(16);
            W.NBarrels = 0;
            W.NStores = 0;

            // Defaults mirrored from your C++ ctor:
            W.AimAzMax = 1.5f;
            W.AimAzMin = -1.5f;
            W.AimAzRest = 0.0f;
            W.AimElMax = 1.5f;
            W.AimElMin = -1.5f;
            W.AimElRest = 0.0f;
            W.SlewRate = (float)(60 * DEGREES);

            W.Eject = FVector(0.0f, -100.0f, 0.0f);
            W.DetSpread = (float)(PI / 8);

            // Parse weapon { ... } parameters
            const int32 ElemCount = (int32)Val->elements()->size();
            for (int32 i = 0; i < ElemCount; ++i)
            {
                TermDef* PDef = Val->elements()->at(i)->isDef();
                if (!PDef)
                    continue;

                Text Key = PDef->name()->value();
                Key.setSensitive(false);

                // Identity
                if (Key == "name") { Text T = ""; GetDefText(T, PDef, fn); W.Name = FString(T); }
                else if (Key == "group") { Text T = ""; GetDefText(T, PDef, fn); W.Group = FString(T); }
                else if (Key == "description") { Text T = ""; GetDefText(T, PDef, fn); W.Description = FString(T); }

                // Flags / ints
                else if (Key == "guided") { int32 N = 0; GetDefNumber(N, PDef, fn); W.Guided = N; }
                else if (Key == "self_aiming") { bool B = false; GetDefBool(B, PDef, fn); W.bSelfAiming = B; }
                else if (Key == "flak") { bool B = false; GetDefBool(B, PDef, fn); W.bFlak = B; }
                else if (Key == "syncro") { bool B = false; GetDefBool(B, PDef, fn); W.bSyncro = B; }
                else if (Key == "visible_stores") { bool B = false; GetDefBool(B, PDef, fn); W.bVisibleStores = B; }
                else if (Key == "value") { int32 N = 0; GetDefNumber(N, PDef, fn); W.Value = N; }
                else if (Key == "probe") { bool B = false; GetDefBool(B, PDef, fn); W.bProbe = B; }
                else if (Key == "target_type") { int32 N = 0; GetDefNumber(N, PDef, fn); W.TargetType = N; }

                // Timing / ammo / energy
                else if (Key == "capacity") { GetDefNumber(W.Capacity, PDef, fn); }
                else if (Key == "recharge_rate") { GetDefNumber(W.RechargeRate, PDef, fn); }
                else if (Key == "refire_delay") { GetDefNumber(W.RefireDelay, PDef, fn); }
                else if (Key == "salvo_delay") { GetDefNumber(W.SalvoDelay, PDef, fn); }
                else if (Key == "ammo") { GetDefNumber(W.Ammo, PDef, fn); }
                else if (Key == "ripple_count") { GetDefNumber(W.RippleCount, PDef, fn); }
                else if (Key == "charge") { GetDefNumber(W.Charge, PDef, fn); }
                else if (Key == "min_charge") { GetDefNumber(W.MinCharge, PDef, fn); }

                // Carry / damage
                else if (Key == "carry_mass") { GetDefNumber(W.CarryMass, PDef, fn); }
                else if (Key == "carry_resist") { GetDefNumber(W.CarryResist, PDef, fn); }
                else if (Key == "damage") { GetDefNumber(W.Damage, PDef, fn); }
                else if (Key == "penetration") { GetDefNumber(W.Penetration, PDef, fn); }
                else if (Key == "lethal_radius") { GetDefNumber(W.LethalRadius, PDef, fn); }
                else if (Key == "integrity") { GetDefNumber(W.Integrity, PDef, fn); }

                // Flight
                else if (Key == "speed") { GetDefNumber(W.Speed, PDef, fn); }
                else if (Key == "life") { GetDefNumber(W.Life, PDef, fn); }
                else if (Key == "mass") { GetDefNumber(W.Mass, PDef, fn); }
                else if (Key == "drag") { GetDefNumber(W.Drag, PDef, fn); }
                else if (Key == "thrust") { GetDefNumber(W.Thrust, PDef, fn); }
                else if (Key == "roll_rate") { GetDefNumber(W.RollRate, PDef, fn); }
                else if (Key == "pitch_rate") { GetDefNumber(W.PitchRate, PDef, fn); }
                else if (Key == "yaw_rate") { GetDefNumber(W.YawRate, PDef, fn); }
                else if (Key == "roll_drag") { GetDefNumber(W.RollDrag, PDef, fn); }
                else if (Key == "pitch_drag") { GetDefNumber(W.PitchDrag, PDef, fn); }
                else if (Key == "yaw_drag") { GetDefNumber(W.YawDrag, PDef, fn); }

                // Ranges
                else if (Key == "min_range") { GetDefNumber(W.MinRange, PDef, fn); }
                else if (Key == "max_range") { GetDefNumber(W.MaxRange, PDef, fn); }
                else if (Key == "max_track") { GetDefNumber(W.MaxTrack, PDef, fn); }

                // Visuals
                else if (Key == "graphic_type") { GetDefNumber(W.GraphicType, PDef, fn); }
                else if (Key == "width") { GetDefNumber(W.Width, PDef, fn); }
                else if (Key == "length") { GetDefNumber(W.Length, PDef, fn); }
                else if (Key == "scale") { GetDefNumber(W.Scale, PDef, fn); }
                else if (Key == "explosion_scale") { GetDefNumber(W.ExplosionScale, PDef, fn); }
                else if (Key == "light") { GetDefNumber(W.Light, PDef, fn); }
                else if (Key == "flash_scale") { GetDefNumber(W.FlashScale, PDef, fn); }
                else if (Key == "flare_scale") { GetDefNumber(W.FlareScale, PDef, fn); }
                else if (Key == "trail_length") { GetDefNumber(W.TrailLength, PDef, fn); }
                else if (Key == "trail_width") { GetDefNumber(W.TrailWidth, PDef, fn); }
                else if (Key == "trail_dim") { GetDefNumber(W.TrailDim, PDef, fn); }

                // Asset ids
                else if (Key == "beauty") { Text T = ""; GetDefText(T, PDef, fn); W.Beauty = FString(T); }
                else if (Key == "bitmap") { Text T = ""; GetDefText(T, PDef, fn); W.Bitmap = FString(T); }
                else if (Key == "turret") { Text T = ""; GetDefText(T, PDef, fn); W.Turret = FString(T); }
                else if (Key == "turret_base") { Text T = ""; GetDefText(T, PDef, fn); W.TurretBase = FString(T); }
                else if (Key == "model") { Text T = ""; GetDefText(T, PDef, fn); W.Model = FString(T); }
                else if (Key == "trail") { Text T = ""; GetDefText(T, PDef, fn); W.Trail = FString(T); }
                else if (Key == "flash") { Text T = ""; GetDefText(T, PDef, fn); W.Flash = FString(T); }
                else if (Key == "flare") { Text T = ""; GetDefText(T, PDef, fn); W.Flare = FString(T); }
                else if (Key == "sound") { Text T = ""; GetDefText(T, PDef, fn); W.Sound = FString(T); }

                // Special cases (legacy)
                else if (Key == "degrees")
                {
                    bool B = false;
                    GetDefBool(B, PDef, fn);
                    bDegrees = B;
                }
                else if (Key == "secret")
                {
                    bool B = false;
                    GetDefBool(B, PDef, fn);
                    W.bSecret = B;
                }
                else if (Key == "aim_az_max")
                {
                    GetDefNumber(W.AimAzMax, PDef, fn);
                    if (bDegrees) W.AimAzMax *= (float)DEGREES;
                    W.AimAzMin = 0.0f - W.AimAzMax;
                }
                else if (Key == "aim_el_max")
                {
                    GetDefNumber(W.AimElMax, PDef, fn);
                    if (bDegrees) W.AimElMax *= (float)DEGREES;
                    W.AimElMin = 0.0f - W.AimElMax;
                }
                else if (Key == "aim_az_min")
                {
                    GetDefNumber(W.AimAzMin, PDef, fn);
                    if (bDegrees) W.AimAzMin *= (float)DEGREES;
                }
                else if (Key == "aim_el_min")
                {
                    GetDefNumber(W.AimElMin, PDef, fn);
                    if (bDegrees) W.AimElMin *= (float)DEGREES;
                }
                else if (Key == "aim_az_rest")
                {
                    GetDefNumber(W.AimAzRest, PDef, fn);
                    if (bDegrees) W.AimAzRest *= (float)DEGREES;
                }
                else if (Key == "aim_el_rest")
                {
                    GetDefNumber(W.AimElRest, PDef, fn);
                    if (bDegrees) W.AimElRest *= (float)DEGREES;
                }
                else if (Key == "spread_az")
                {
                    GetDefNumber(W.SpreadAz, PDef, fn);
                    if (bDegrees) W.SpreadAz *= (float)DEGREES;
                }
                else if (Key == "spread_el")
                {
                    GetDefNumber(W.SpreadEl, PDef, fn);
                    if (bDegrees) W.SpreadEl *= (float)DEGREES;
                }
                else if (Key == "animation")
                {
                    Text T = "";
                    GetDefText(T, PDef, fn);
                    W.AnimFrames.Add(FString(T));
                    W.AnimLength = W.AnimFrames.Num();
                }
                else if (Key == "light_color")
                {
                    // You already used GetDefFColor in legacy; keep that helper.
                    GetDefFColor(W.LightColor, PDef, fn);
                }
                else if (Key == "muzzle")
                {
                    if (W.NBarrels < 16)
                    {
                        FVector V = FVector::ZeroVector;
                        GetDefVec(V, PDef, fn);
                        W.MuzzlePoints[W.NBarrels++] = V;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[WEAPON] WARNING: too many muzzles for weapon '%s' in '%s'"),
                            *W.Name, *FilePath);
                    }
                }
                else if (Key == "attachment")
                {
                    if (W.NStores < 16)
                    {
                        FVector V = FVector::ZeroVector;
                        GetDefVec(V, PDef, fn);
                        W.Attachments[W.NStores++] = V;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[WEAPON] WARNING: too many attachments for weapon '%s' in '%s'"),
                            *W.Name, *FilePath);
                    }
                }
                else if (Key == "eject")
                {
                    FVector V = FVector::ZeroVector;
                    GetDefVec(V, PDef, fn);
                    W.Eject = V;
                }
                else if (Key == "decoy")
                {
                    // Legacy: string -> ShipDesign::ClassForName -> int
                    // We keep it as int (DecoyType) but you decide how to map.
                    // If you already have a helper, call it here. Otherwise keep 0.
                    Text T = "";
                    GetDefText(T, PDef, fn);

                    // Placeholder: leave as 0 if you don't want coupling here.
                    // If you DO have UFormattingUtils::GetDesignClassFromName or similar, use it:
                    // W.DecoyType = UFormattingUtils::GetDesignClassFromName(T);
                    W.DecoyType = 0;
                }
                else if (Key == "damage_type")
                {
                    // Legacy parsed string into DMG_* ints.
                    Text T = "";
                    GetDefText(T, PDef, fn);

                    FString S(T.data());
                    S = S.ToLower();

                    if (S == TEXT("normal"))      W.DamageType = 0; // DMG_NORMAL
                    else if (S == TEXT("emp"))    W.DamageType = 1; // DMG_EMP
                    else if (S == TEXT("power"))  W.DamageType = 2; // DMG_POWER
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[WEAPON] WARNING: unknown damage_type '%s' in '%s'"),
                            *S, *FilePath);
                    }
                }
                else if (Key == "slew_rate")
                {
                    GetDefNumber(W.SlewRate, PDef, fn);
                    // legacy doesn’t degrees-convert slew_rate, it is already in radians or scaled; keep as-authored
                }
                else if (Key == "turret_axis")
                {
                    GetDefNumber(W.TurretAxis, PDef, fn);
                }
                else if (Key == "det_range")
                {
                    GetDefNumber(W.DetRange, PDef, fn);
                }
                else if (Key == "det_count")
                {
                    GetDefNumber(W.DetCount, PDef, fn);
                }
                else if (Key == "det_spread")
                {
                    GetDefNumber(W.DetSpread, PDef, fn);
                }
                else if (Key == "det_child")
                {
                    Text T = "";
                    GetDefText(T, PDef, fn);
                    W.DetChild = FString(T);
                }
                else
                {
                    UE_LOG(LogTemp, Verbose,
                        TEXT("[WEAPON] weapon param '%s' ignored in '%s'"),
                        ANSI_TO_TCHAR(Key.data()), *FilePath);
                }
            }

            // Post-fixups (legacy)
            if (W.MaxRange == 0.0f)
                W.MaxRange = W.Speed * W.Life;

            if (W.MaxTrack == 0.0f)
                W.MaxTrack = 3.0f * W.MaxRange;

            if (W.bProbe && W.LethalRadius < 1e3f)
                W.LethalRadius = 50e3f;

            if (W.bBeam)
                W.bFlak = false;

            // Self aiming expands firing cone in legacy:
            if (W.bSelfAiming)
            {
                auto AbsF = [](float X) { return (float)FMath::Abs(X); };

                if (AbsF(W.AimAzMax) > W.FiringCone) W.FiringCone = AbsF(W.AimAzMax);
                if (AbsF(W.AimAzMin) > W.FiringCone) W.FiringCone = AbsF(W.AimAzMin);
                if (AbsF(W.AimElMax) > W.FiringCone) W.FiringCone = AbsF(W.AimElMax);
                if (AbsF(W.AimElMin) > W.FiringCone) W.FiringCone = AbsF(W.AimElMin);
            }

            // Lock fixed arrays + clamp counters:
            ClampStores(W);

            if (W.Name.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("[WEAPON] WARNING: weapon with missing name ignored in '%s'"), *FilePath);
                continue;
            }

            FString CleanName = W.Name;
            CleanName.TrimStartAndEndInline();

            const FName RowName(*CleanName);

            // Cache:
            DesignsByName.Add(RowName, W);

            if (WeaponDesignDataTable)
{   
    if (FWeaponDesign* Existing =
        WeaponDesignDataTable->FindRow<FWeaponDesign>(RowName, TEXT("LoadWeaponDesign"), /*bWarnIfRowMissing=*/false))
    {
        *Existing = W; // overwrite row data in place (safe)
    }
    else
    {
        WeaponDesignDataTable->AddRow(RowName, W);
    }
}
            ++ParsedWeapons;
        }
        else
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("[WEAPON] WARNING: unknown definition '%s' in '%s'"),
                ANSI_TO_TCHAR(DefName.data()), *FilePath);
        }

    } while (TermObj);

    if (TermObj)
    {
        delete TermObj;
        TermObj = nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("[WEAPON] Loaded %d weapon designs (cache=%d) from '%s'"),
        ParsedWeapons, DesignsByName.Num(), *FilePath);
}
