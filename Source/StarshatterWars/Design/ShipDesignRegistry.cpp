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
#include "Power.h"
#include "Drive.h"
#include "Thruster.h"
#include "NavSystem.h"

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

    Legacy->reactors.clear();
    Legacy->drives.clear();
    Legacy->thrusters.clear();

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

    for (const FShipPower& Src : Row.Power)
    {
        if (Src.Type == EPowerSource::NONE)
        {
            continue;
        }

        PowerSource* Reactor = new PowerSource(
            Src.Type,
            Src.Output,
            Src.Fuel
        );

        Reactor->SetName(TCHAR_TO_ANSI(*Src.PName));
        Reactor->SetAbbreviation(TCHAR_TO_ANSI(*Src.PAbrv));

        Legacy->reactors.append(Reactor);
    }

    //-------------------------------------------------------------
    // Drives
    //-------------------------------------------------------------

    for (const FShipDrive& Src : Row.Drive)
    {
        if (Src.Type == EDriveType::UNKNOWN)
        {
            continue;
        }

        Drive* NewDrive = new Drive(
            Src.Type,
            Src.Thrust,
            Src.Augmenter,
            Src.bShowTrail
        );

        if (!Src.DesignName.IsEmpty())
        {
            NewDrive->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        }

        if (!Src.Abbrev.IsEmpty())
        {
            NewDrive->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        }

        for (const FDrivePort& Port : Src.Ports)
        {
            NewDrive->CreatePort(Port.Location, Port.FlareScale);
        }

        Legacy->drives.append(NewDrive);
    }

    Legacy->main_drive = Legacy->drives.size() > 0 ? 0 : -1;
    //-------------------------------------------------------------
    // Thrusters
    //-------------------------------------------------------------
    for (const FShipThruster& Src : Row.Thruster)
    {
        if (Src.Type == EDriveType::UNKNOWN)
        {
            continue;
        }

        Thruster* NewThruster = new Thruster(
            static_cast<int>(Src.Type),
            Src.Thrust,
            Src.ThrusterScale
        );

        if (!Src.DesignName.IsEmpty())
        {
            NewThruster->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        }

        NewThruster->SetSourceIndex(Src.SourceIndex);
        NewThruster->SetHullFactor(Src.HullFactor);

        for (const FThrusterPort& Port : Src.Ports)
        {
            NewThruster->CreatePort(
                static_cast<int>(Port.Direction),
                Port.Location,
                static_cast<DWORD>(Port.Fire),
                Port.PortScale
            );

            UE_LOG(LogTemp, Warning,
                TEXT("[ShipDesignRegistry] Thruster built Row='%s' Thruster=%p Ports=%d"),
                *RowName.ToString(),
                NewThruster,
                NewThruster->NumThrusters());

            for (int i = 0; i < NewThruster->NumThrusters(); i++)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[ShipDesignRegistry]   Port[%d] Type=%d Flare=%p Trail=%p"),
                    i,
                    i,
                    NewThruster->Flare(i),
                    NewThruster->Trail(i));
            }
        }

        Legacy->thrusters.append(NewThruster);
    }

    Legacy->thruster = Legacy->thrusters.size() > 0 ? Legacy->thrusters[0] : nullptr;
    
    //-------------------------------------------------------------
    // Nav system
    //-------------------------------------------------------------
    Legacy->navsys = nullptr;

    for (const FShipNavSystem& Src : Row.NavSys)
    {
        if (Src.DesignName.IsEmpty())
        {
            continue;
        }

        NavSystem* NewNavSys = new NavSystem();

        NewNavSys->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        NewNavSys->SetSourceIndex(Src.SourceIndex);
        NewNavSys->SetHullFactor(Src.HullFactor);

        Legacy->navsys = NewNavSys;

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] NavSys built Row='%s' NavSys=%p Name='%s' SourceIndex=%d HullFactor=%.2f"),
            *RowName.ToString(),
            Legacy->navsys,
            *Src.DesignName,
            Src.SourceIndex,
            Src.HullFactor);

        Legacy->navsystems.append(NewNavSys);
    }

    Legacy->navsys = Legacy->navsystems.size() > 0 ? Legacy->navsystems[0] : nullptr;
    return Legacy;
}


    