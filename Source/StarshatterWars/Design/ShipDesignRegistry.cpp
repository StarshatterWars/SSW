/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    FILE:         ShipDesignRegistry.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    ShipDesignRegistry implementation.

    - Stores Unreal FShipDesign rows
    - Converts them into legacy ShipDesign objects
    - Owns lifetime of legacy objects
    - Provides lookup for both systems
*/

#include "ShipDesignRegistry.h"

TMap<FName, FShipDesign> ShipDesignRegistry::DesignsByName;
TMap<FName, ShipDesign*> ShipDesignRegistry::LegacyDesignsByName;

//-------------------------------------------------------------
// Lifecycle
//-------------------------------------------------------------

void ShipDesignRegistry::Clear()
{
    for (TPair<FName, ShipDesign*>& Pair : LegacyDesignsByName)
    {
        delete Pair.Value;
    }

    LegacyDesignsByName.Empty();
    DesignsByName.Empty();
}

//-------------------------------------------------------------
// Registration
//-------------------------------------------------------------

void ShipDesignRegistry::RegisterDesign(const FName& RowName, const FShipDesign& Row)
{
    if (RowName.IsNone())
    {
        return;
    }

    DesignsByName.Add(RowName, Row);

    // Rebuild legacy version
    if (ShipDesign* Existing = LegacyDesignsByName.FindRef(RowName))
    {
        delete Existing;
        LegacyDesignsByName.Remove(RowName);
    }

    ShipDesign* Legacy = ConvertToLegacyDesign(RowName, Row);

    if (Legacy)
    {
        LegacyDesignsByName.Add(RowName, Legacy);
    }
}

void ShipDesignRegistry::RegisterDesign(const FString& Name, const FShipDesign& Row)
{
    if (!Name.IsEmpty())
    {
        RegisterDesign(FName(*Name), Row);
    }
}

//-------------------------------------------------------------
// Lookup (Unreal)
//-------------------------------------------------------------

const FShipDesign* ShipDesignRegistry::Find(const FName& Name)
{
    return Name.IsNone() ? nullptr : DesignsByName.Find(Name);
}

const FShipDesign* ShipDesignRegistry::Find(const FString& Name)
{
    return Name.IsEmpty() ? nullptr : DesignsByName.Find(FName(*Name));
}

const FShipDesign* ShipDesignRegistry::Find(const char* Name)
{
    if (!Name || !Name[0])
    {
        return nullptr;
    }

    return DesignsByName.Find(FName(ANSI_TO_TCHAR(Name)));
}

//-------------------------------------------------------------
// Lookup (Legacy)
//-------------------------------------------------------------

ShipDesign* ShipDesignRegistry::FindLegacy(const FName& Name)
{
    return Name.IsNone() ? nullptr : LegacyDesignsByName.FindRef(Name);
}

ShipDesign* ShipDesignRegistry::FindLegacy(const FString& Name)
{
    return Name.IsEmpty() ? nullptr : LegacyDesignsByName.FindRef(FName(*Name));
}

ShipDesign* ShipDesignRegistry::FindLegacy(const char* Name)
{
    if (!Name || !Name[0])
    {
        return nullptr;
    }

    return LegacyDesignsByName.FindRef(FName(ANSI_TO_TCHAR(Name)));
}

//-------------------------------------------------------------
// Utility
//-------------------------------------------------------------

bool ShipDesignRegistry::Has(const FName& Name)
{
    return !Name.IsNone() && DesignsByName.Contains(Name);
}

bool ShipDesignRegistry::Has(const FString& Name)
{
    return !Name.IsEmpty() && DesignsByName.Contains(FName(*Name));
}

bool ShipDesignRegistry::Has(const char* Name)
{
    return Name && Name[0] && DesignsByName.Contains(FName(ANSI_TO_TCHAR(Name)));
}

int32 ShipDesignRegistry::Num()
{
    return DesignsByName.Num();
}

const TMap<FName, FShipDesign>& ShipDesignRegistry::GetAll()
{
    return DesignsByName;
}

//-------------------------------------------------------------
// Conversion Helpers
//-------------------------------------------------------------

void ShipDesignRegistry::CopyStringToAnsi(char* Dest, int32 DestSize, const FString& Source)
{
    if (!Dest || DestSize <= 0)
    {
        return;
    }

    Dest[0] = '\0';

    FTCHARToUTF8 Converted(*Source);
    FCStringAnsi::Strncpy(Dest, Converted.Get(), DestSize);
    Dest[DestSize - 1] = '\0';
}

ShipDesign* ShipDesignRegistry::ConvertToLegacyDesign(const FName& RowName, const FShipDesign& Row)
{
    ShipDesign* Legacy = new ShipDesign();

    if (!Legacy)
    {
        return nullptr;
    }

    const FString NameStr = RowName.ToString();

    CopyStringToAnsi(Legacy->name, sizeof(Legacy->name), NameStr);
    CopyStringToAnsi(Legacy->display_name, sizeof(Legacy->display_name), Row.DisplayName);
    CopyStringToAnsi(Legacy->abrv, sizeof(Legacy->abrv), Row.Abrv);
    CopyStringToAnsi(Legacy->filename, sizeof(Legacy->filename), NameStr);
    CopyStringToAnsi(Legacy->path_name, sizeof(Legacy->path_name), TEXT(""));

    Legacy->valid = true;
    Legacy->secret = Row.Secret;

    //-------------------------------------------------------------
    // Core flight + physics
    //-------------------------------------------------------------
    Legacy->scale = Row.Scale;
    Legacy->vlimit = Row.Vlimit;
    Legacy->agility = Row.Agility;

    Legacy->mass = Row.Mass;
    Legacy->integrity = Row.Integrity;
    Legacy->radius = Row.Scale; // fallback (adjust later if needed)

    //-------------------------------------------------------------
    // Movement + handling
    //-------------------------------------------------------------
    Legacy->roll_rate = Row.RollRate;
    Legacy->pitch_rate = Row.PitchRate;
    Legacy->yaw_rate = Row.YawRate;
    Legacy->turn_bank = Row.TurnBank;

    Legacy->drag = Row.Drag;
    Legacy->arcade_drag = Row.ArcadeDrag;
    Legacy->roll_drag = Row.RollDrag;
    Legacy->pitch_drag = Row.PitchDrag;
    Legacy->yaw_drag = Row.YawDrag;

    //-------------------------------------------------------------
    // Sensors
    //-------------------------------------------------------------
    Legacy->pcs = Row.PCS;
    Legacy->acs = Row.ACS;
    Legacy->detet = Row.Detet;

    //-------------------------------------------------------------
    // AI
    //-------------------------------------------------------------
    Legacy->avoid_time = Row.AvoidTime;
    Legacy->avoid_fighter = Row.AvoidFighter;
    Legacy->avoid_strike = Row.AvoidStrike;
    Legacy->avoid_target = Row.AvoidTarget;
    Legacy->commit_range = Row.CommitRange;

    //-------------------------------------------------------------
    // Camera offsets
    //-------------------------------------------------------------
    Legacy->chase_vec = Row.ChaseVec;
    Legacy->bridge_vec = Row.BridgeVec;
    Legacy->beauty_cam = Row.BeautyCam;

    //-------------------------------------------------------------
    // Translation (important for strafing)
    //-------------------------------------------------------------
    Legacy->trans_x = Row.Trans.X;
    Legacy->trans_y = Row.Trans.Y;
    Legacy->trans_z = Row.Trans.Z;

    return Legacy;
}