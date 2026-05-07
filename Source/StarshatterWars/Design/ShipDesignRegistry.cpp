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
#include "NavLight.h"
#include "NavSystem.h"
#include "Farcaster.h"
#include "QuantumDrive.h"
#include "Sensor.h"
#include "Shield.h"
#include "Computer.h"
#include "FlightComputer.h"
#include "HardPoint.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "LandingGear.h"
#include "FlightDeck.h"
#include "Hangar.h"
#include "ShipSquadron.h"

#include "WeaponDesignRegistry.h"

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

void
ShipDesignRegistry::ResetShipComponents(ShipDesign* Ship) {
    Ship->reactors.clear();
    Ship->drives.clear();
    Ship->thrusters.clear();
    Ship->quantum_drives.clear();
    Ship->farcasters.clear();
    Ship->navsystems.clear();
    Ship->sensors.clear();
    Ship->shields.clear();
    Ship->computers.clear();        
    Ship->hardpoints.clear();
    Ship->weapons.clear();
    Ship->navlights.clear();
    Ship->landing_gear.clear();
    Ship->flight_decks.clear();

    Ship->quantum_drive = nullptr;
    Ship->farcaster = nullptr;
    Ship->navsys = nullptr;
    Ship->thruster = nullptr;
    Ship->sensor = nullptr;
    Ship->shield = nullptr;
	Ship->navlight = nullptr;
	Ship->gear = nullptr;
}

ShipDesign* ShipDesignRegistry::ConvertToLegacyDesign(const FName& RowName, const FShipDesign& Row)
{
    ShipDesign* Legacy = new ShipDesign();

    if (!Legacy)
    {
        return nullptr;
    }

	ResetShipComponents(Legacy);

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
    
    //-------------------------------------------------------------
    // Sensors
    //-------------------------------------------------------------

    for (const FShipSensor& Src : Row.Sensor)
    {
        if (Src.DesignName.IsEmpty())
        {
            continue;
        }

        Sensor* NewSensor = new Sensor();

        NewSensor->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        NewSensor->SetSourceIndex(Src.SourceIndex);
        NewSensor->SetHullFactor(Src.HullFactor);
        NewSensor->SetRangeSettings(Src.Ranges);

        Legacy->sensors.append(NewSensor);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Sensor built Row='%s' Sensor=%p Name='%s' Ranges=%d SourceIndex=%d"),
            *RowName.ToString(),
            NewSensor,
            *Src.DesignName,
            Src.Ranges.Num(),
            Src.SourceIndex);
    }

    Legacy->sensor =
        Legacy->sensors.size() > 0 ? Legacy->sensors[0] : nullptr;

    //-------------------------------------------------------------
    // Shields
    //-------------------------------------------------------------

    for (const FShipShield& Src : Row.Shield)
    {
        if (Src.ShieldType == EShieldType::UNKNOWN)
        {
            continue;
        }

        Shield* NewShield = new Shield(Src.ShieldType);

        if (!Src.Name.IsEmpty())
        {
            NewShield->SetName(TCHAR_TO_ANSI(*Src.Name));
        }
        else if (!Src.DesignName.IsEmpty())
        {
            NewShield->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        }

        if (!Src.Abbrev.IsEmpty())
        {
            NewShield->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        }

        NewShield->SetSourceIndex(Src.SourceIndex);
        NewShield->SetHullFactor(Src.HullFactor);

        NewShield->SetCapacity(Src.Capacity);
        NewShield->SetConsumption(Src.Consumption);
        NewShield->SetShieldFactor(Src.Factor);
        NewShield->SetShieldCutoff(Src.Cutoff);
        NewShield->SetShieldCurve(Src.Curve);
        NewShield->SetDeflectionCost(Src.DeflectionCost);
        NewShield->SetShieldCapacitor(Src.bShieldCapacitor);
        NewShield->SetShieldBubble(Src.bShieldBubble);

        Legacy->shields.append(NewShield);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Shield built Row='%s' Shield=%p Type=%d Name='%s' Cap=%.0f Cons=%.0f Factor=%.4f SourceIndex=%d"),
            *RowName.ToString(),
            NewShield,
            (int32)Src.ShieldType,
            *Src.Name,
            Src.Capacity,
            Src.Consumption,
            Src.Factor,
            Src.SourceIndex);
    }

    Legacy->shield =
        Legacy->shields.size() > 0 ? Legacy->shields[0] : nullptr;
    
    //-------------------------------------------------------------
    // Computers
    //------------------------------------------------------------- 

    for (const FShipComputer& Src : Row.Computer)
    {
        if (Src.Type == EComputerType::UNKNOWN)
        {
            continue;
        }

        const FString CompName =
            !Src.Name.IsEmpty() ? Src.Name :
            !Src.DesignName.IsEmpty() ? Src.DesignName :
            TEXT("Computer");

        //---------------------------------------------------------
        // Create legacy Computer (stores type)
        //---------------------------------------------------------
        Computer* NewComputer = new Computer(
            Src.Type,
            TCHAR_TO_ANSI(*CompName)
        );

        //---------------------------------------------------------
        // Name 
        //---------------------------------------------------------
        if (!Src.Name.IsEmpty())
        {
            NewComputer->SetName(TCHAR_TO_ANSI(*Src.Name));
        }
        else if (!Src.DesignName.IsEmpty())
        {
            NewComputer->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        }

        //---------------------------------------------------------
        // Abbreviation
        //---------------------------------------------------------
        if (!Src.Abbrev.IsEmpty())
        {
            NewComputer->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        }

        //---------------------------------------------------------
        // Core properties
        //---------------------------------------------------------
        NewComputer->SetSourceIndex(Src.SourceIndex);
        NewComputer->SetHullFactor(Src.HullFactor);

        //---------------------------------------------------------
        // Register into legacy design
        //---------------------------------------------------------
        Legacy->computers.append(NewComputer);

        //---------------------------------------------------------
        // Debug (use ACTUAL runtime values)
        //---------------------------------------------------------
        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Computer built Row='%s' Computer=%p Type=%d Name='%hs' Abbrev='%hs' SourceIndex=%d"),
            *RowName.ToString(),
            NewComputer,
            (int32)NewComputer->GetComputerType(),
            NewComputer->Name(),
            NewComputer->Abbreviation(),
            Src.SourceIndex);
        }

    //-------------------------------------------------------------
    // Quantum drives
    //-------------------------------------------------------------

    for (const FShipQuantum& Src : Row.Quantum)
    {
        if (Src.DesignName.IsEmpty())
        {
            continue;
        }

        const QuantumDrive::SUBTYPE Subtype =
            (Src.Type == EQuantumDriveType::HYPER)
            ? QuantumDrive::HYPER
            : QuantumDrive::QUANTUM;

        QuantumDrive* NewQuantum = new QuantumDrive(
            Subtype,
            Src.Capacity,
            Src.Consumption
        );

        NewQuantum->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        NewQuantum->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        NewQuantum->SetSourceIndex(Src.SourceIndex);
        NewQuantum->SetHullFactor(Src.HullFactor);
        NewQuantum->SetCountdown(Src.Countdown);

        Legacy->quantum_drives.append(NewQuantum);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Quantum built Row='%s' Quantum=%p Name='%s' Type=%d Cap=%.0f Cons=%.0f Countdown=%.2f SourceIndex=%d"),
            *RowName.ToString(),
            NewQuantum,
            *Src.DesignName,
            (int32)Src.Type,
            Src.Capacity,
            Src.Consumption,
            Src.Countdown,
            Src.SourceIndex);
    }

    Legacy->quantum_drive =
        Legacy->quantum_drives.size() > 0 ? Legacy->quantum_drives[0] : nullptr;

    //-------------------------------------------------------------
    // Farcasters
    //-------------------------------------------------------------

    for (const FShipFarcaster& Src : Row.Farcaster)
    {
        if (Src.DesignName.IsEmpty())
        {
            continue;
        }

        Farcaster* NewFarcaster = new Farcaster(
            Src.Capacity,
            Src.Consumption
        );

        NewFarcaster->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        NewFarcaster->SetSourceIndex(Src.SourceIndex);
        NewFarcaster->SetHullFactor(Src.HullFactor);

        NewFarcaster->SetStartPoint(Src.StartPoint);
        NewFarcaster->SetEndPoint(Src.EndPoint);

        NewFarcaster->SetCycleTime(Src.CycleTime);

        for (int32 i = 0; i < Src.ApproachPoints.Num() && i < Farcaster::NUM_APPROACH_PTS; ++i)
        {
            NewFarcaster->SetApproachPoint(i, Src.ApproachPoints[i]);
        }

        Legacy->farcasters.append(NewFarcaster);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Farcaster built Row='%s' Farcaster=%p Name='%s' Cap=%.0f Cons=%.0f SourceIndex=%d ApproachPts=%d"),
            *RowName.ToString(),
            NewFarcaster,
            *Src.DesignName,
            Src.Capacity,
            Src.Consumption,
            Src.SourceIndex,
            Src.ApproachPoints.Num());
    }

    Legacy->farcaster =
        Legacy->farcasters.size() > 0 ? Legacy->farcasters[0] : nullptr;
  
    //-------------------------------------------------------------
    // Nav lights
    //-------------------------------------------------------------
    for (const FShipNavLight& Src : Row.Navlight)
    {
        if (Src.Beacons.Num() <= 0)
        {
            continue;
        }

        NavLight* NewNavLight = new NavLight(
            Src.Period,
            Src.Scale
        );

        if (!Src.Name.IsEmpty())
        {
            NewNavLight->SetName(TCHAR_TO_ANSI(*Src.Name));
        }
        else if (!Src.DesignName.IsEmpty())
        {
            NewNavLight->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        }
        else
        {
            NewNavLight->SetName("NavLight");
        }

        if (!Src.Abbrev.IsEmpty())
        {
            NewNavLight->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        }

        for (const FNavLightBeacon& Beacon : Src.Beacons)
        {
            NewNavLight->AddBeacon(
                Beacon.Name,
                Beacon.Location,
                Beacon.LocalRotation,
                Beacon.Color,
                Beacon.Intensity,
                Beacon.Radius,
                Beacon.Mode,
                Beacon.BlinkInterval,
                Beacon.PhaseOffset
            );
        }

        Legacy->navlights.append(NewNavLight);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] NavLight built Row='%s' NavLight=%p Name='%hs' Period=%.2f Scale=%.2f Beacons=%d"),
            *RowName.ToString(),
            NewNavLight,
            NewNavLight->Name(),
            Src.Period,
            Src.Scale,
            Src.Beacons.Num());
    }

    //-------------------------------------------------------------
    // Weapons
    //-------------------------------------------------------------
    for (const FShipWeapon& Src : Row.Weapon)
    {
        if (Src.WeaponType.IsEmpty())
        {
            continue;
        }

        WeaponDesign* WpnDesign =
            WeaponDesignRegistry::FindLegacy(Src.WeaponType);

        if (!WpnDesign)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ShipDesignRegistry] WeaponDesign NOT FOUND Row='%s' Name='%s' Type='%s'"),
                *RowName.ToString(),
                *Src.Name,
                *Src.WeaponType);
            continue;
        }

        FVector Muzzles[Weapon::MAX_BARRELS];

        int32 MuzzleCount = FMath::Clamp(
            Src.Muzzles.Num(),
            0,
            Weapon::MAX_BARRELS
        );

        if (MuzzleCount > 0)
        {
            for (int32 i = 0; i < MuzzleCount; ++i)
            {
                Muzzles[i] = Src.Muzzles[i];
            }
        }
        else
        {
            MuzzleCount = 1;
            Muzzles[0] = Src.Location;

            UE_LOG(LogTemp, Warning,
                TEXT("[ShipDesignRegistry] Weapon '%s' has no muzzles; using Location fallback"),
                *Src.Name);
        }

        Weapon* NewWeapon = new Weapon(
            WpnDesign,
            MuzzleCount,
            Muzzles,
            Src.AzimuthRadians,
            Src.ElevationRadians
        );

        if (!Src.GroupName.IsEmpty())
        {
            NewWeapon->SetGroup(TCHAR_TO_ANSI(*Src.GroupName));
        }

        if (Src.Aim.bHasAzMax)
        {
            NewWeapon->SetAzimuthMax(Src.Aim.AzMax);
        }

        if (Src.Aim.bHasAzMin)
        {
            NewWeapon->SetAzimuthMin(Src.Aim.AzMin);
        }

        if (Src.Aim.bHasElMax)
        {
            NewWeapon->SetElevationMax(Src.Aim.ElMax);
        }

        if (Src.Aim.bHasElMin)
        {
            NewWeapon->SetElevationMin(Src.Aim.ElMin);
        }

        if (Src.Aim.bHasAzRest)
        {
            NewWeapon->SetRestAzimuth(Src.Aim.AzRest);
        }

        if (Src.Aim.bHasElRest)
        {
            NewWeapon->SetRestElevation(Src.Aim.ElRest);
        }

        Legacy->weapons.append(NewWeapon);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Weapon built Row='%s' Weapon=%p Name='%s' Type='%s' Group='%s' Muzzles=%d SourceIndex=%d"),
            *RowName.ToString(),
            NewWeapon,
            *Src.Name,
            *Src.WeaponType,
            *Src.GroupName,
            MuzzleCount,
            Src.SourceIndex);
    }

    //-------------------------------------------------------------
    // Hardpoints
    //-------------------------------------------------------------
    for (const FShipHardPoint& Src : Row.Hardpoint)
    {
        if (Src.Name.IsEmpty() &&
            Src.DesignName.IsEmpty() &&
            Src.AllowedWeaponTypes.Num() <= 0)
        {
            continue;
        }

        HardPoint* NewHardpoint = new HardPoint(
            Src.Muzzle,
            Src.AzimuthRadians,
            Src.ElevationRadians
        );

        if (!Src.Name.IsEmpty())
        {
            NewHardpoint->SetName(TCHAR_TO_ANSI(*Src.Name));
        }

        if (!Src.Abbrev.IsEmpty())
        {
            NewHardpoint->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        }

        if (!Src.DesignName.IsEmpty())
        {
            NewHardpoint->SetDesign(TCHAR_TO_ANSI(*Src.DesignName));
        }

        NewHardpoint->Mount(
            Src.Location,
            Src.Size,
            Src.HullFactor
        );

        for (const FString& WeaponTypeName : Src.AllowedWeaponTypes)
        {
            if (WeaponTypeName.IsEmpty())
            {
                continue;
            }

            WeaponDesign* AllowedDesign =
                WeaponDesignRegistry::FindLegacy(WeaponTypeName);

            if (!AllowedDesign)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("[ShipDesignRegistry] Hardpoint WeaponDesign NOT FOUND Row='%s' Hardpoint='%s' WeaponType='%s'"),
                    *RowName.ToString(),
                    *Src.Name,
                    *WeaponTypeName);
                continue;
            }

            NewHardpoint->AddDesign(AllowedDesign);

            UE_LOG(LogTemp, Warning,
                TEXT("[ShipDesignRegistry]   Hardpoint allowed weapon Row='%s' Hardpoint='%s' WeaponType='%s' Design=%p"),
                *RowName.ToString(),
                *Src.Name,
                *WeaponTypeName,
                AllowedDesign);
        }

        Legacy->hardpoints.append(NewHardpoint);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Hardpoint built Row='%s' Hardpoint=%p Name='%s' Design='%s' AllowedTypes=%d"),
            *RowName.ToString(),
            NewHardpoint,
            *Src.Name,
            *Src.DesignName,
            Src.AllowedWeaponTypes.Num());
    }

    //-------------------------------------------------------------
    // Landing gear
    //-------------------------------------------------------------

    for (const FShipLandingGear& Src : Row.LandingGear)
    {
        if (Src.GearItems.Num() <= 0)
        {
            continue;
        }

        LandingGear* NewGear = new LandingGear();

        if (!Src.Name.IsEmpty())
        {
            NewGear->SetName(TCHAR_TO_ANSI(*Src.Name));
        }
        else if (!Src.DesignName.IsEmpty())
        {
            NewGear->SetName(TCHAR_TO_ANSI(*Src.DesignName));
        }
        else
        {
            NewGear->SetName("Landing Gear");
        }

        if (!Src.Abbrev.IsEmpty())
        {
            NewGear->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        }

        for (const FLandingGearItem& Item : Src.GearItems)
        {
            NewGear->AddGear(
                nullptr,
                Item.Start,
                Item.End);

            UE_LOG(LogTemp, Warning,
                TEXT("[ShipDesignRegistry]   LandingGear Item Row='%s' Model='%s' Start=%s End=%s"),
                *RowName.ToString(),
                *Item.ModelName,
                *Item.Start.ToString(),
                *Item.End.ToString());
        }

        Legacy->landing_gear.append(NewGear);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] LandingGear built Row='%s' Gear=%p Name='%hs' Items=%d"),
            *RowName.ToString(),
            NewGear,
            NewGear->Name(),
            Src.GearItems.Num());
    }

    //-------------------------------------------------------------
    // Flight decks
    //-------------------------------------------------------------

    for (const FShipFlightDeck& Src : Row.FlightDeck)
    {
        FlightDeck* NewDeck = new FlightDeck();

        if (!NewDeck)
        {
            continue;
        }

        if (!Src.Name.IsEmpty())
        {
            NewDeck->SetName(TCHAR_TO_ANSI(*Src.Name));
        }

        if (!Src.Abbrev.IsEmpty())
        {
            NewDeck->SetAbbreviation(TCHAR_TO_ANSI(*Src.Abbrev));
        }

        if (Src.bLaunchDeck)
        {
            NewDeck->SetLaunchDeck();
        }

        if (Src.bRecoveryDeck)
        {
            NewDeck->SetRecoveryDeck();
        }

        NewDeck->Mount(
            Src.Location,
            Src.Size,
            Src.HullFactor);

        NewDeck->SetCamLoc(Src.Location);
        NewDeck->SetStartPoint(Src.StartPoint);
        NewDeck->SetEndPoint(Src.EndPoint);

        NewDeck->SetCycleTime(Src.CycleTime);

        for (int32 i = 0;
            i < Src.ApproachPoints.Num() &&
            i < FlightDeck::NUM_APPROACH_PTS;
            ++i)
        {
            NewDeck->SetApproachPoint(
                i,
                Src.ApproachPoints[i]);
        }

        for (const FFlightDeckSlot& Spot : Src.Slots)
        {
            NewDeck->AddSlot(
                Spot.Location,
                Spot.FilterMask);
        }

        Legacy->flight_decks.append(NewDeck);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] FlightDeck built Row='%s' Deck=%p Name='%s' Launch=%d Recovery=%d Spots=%d Approaches=%d"),
            *RowName.ToString(),
            NewDeck,
            *Src.Name,
            Src.bLaunchDeck ? 1 : 0,
            Src.bRecoveryDeck ? 1 : 0,
            Src.Slots.Num(),
            Src.ApproachPoints.Num());
    }

    //-------------------------------------------------------------
// Squadrons / Hangar inventory
//-------------------------------------------------------------

    for (const FShipSquadron& Src : Row.Squadron)
    {
        if (Src.DesignName.IsEmpty() || Src.Count <= 0)
        {
            continue;
        }

        ShipDesign* SquadronDesign =
            ShipDesignRegistry::FindLegacy(FName(*Src.DesignName));

        if (!SquadronDesign)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ShipDesignRegistry] Squadron design NOT FOUND Row='%s' Squadron='%s' Design='%s'"),
                *RowName.ToString(),
                *Src.Name,
                *Src.DesignName);
            continue;
        }

        ShipSquadron* NewSquadron =
            new ShipSquadron();

        if (!NewSquadron)
        {
            continue;
        }

        NewSquadron->SetName(
            TCHAR_TO_ANSI(*Src.Name));

        NewSquadron->SetName(TCHAR_TO_ANSI(*Src.Name));
        NewSquadron->SetDesign(SquadronDesign);
        NewSquadron->SetCount(Src.Count);
        NewSquadron->SetAvail(Src.Avail);

        Legacy->squadrons.append(NewSquadron);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipDesignRegistry] Squadron built Row='%s' Squadron=%p Name='%s' Design='%s' Count=%d Avail=%d"),
            *RowName.ToString(),
            NewSquadron,
            ANSI_TO_TCHAR(NewSquadron->GetName()),
            *Src.DesignName,
            Src.Count,
            Src.Avail);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipDesignRegistry] ConvertToLegacyDesign COMPLETE Row='%s' Reactors=%d Drives=%d Thrusters=%d NavSys=%d Sensors=%d Shields=%d Computers=%d Quantum=%d Farcasters=%d FlightDecks=%d LandingGear=%d Squadrons=%d Weapons=%d Hardpoints=%d"),
        *RowName.ToString(),
        Legacy->reactors.size(),
        Legacy->drives.size(),
        Legacy->thrusters.size(),
        Legacy->navsystems.size(),
        Legacy->sensors.size(),
        Legacy->shields.size(),
        Legacy->computers.size(),
        Legacy->quantum_drives.size(),
        Legacy->farcasters.size(),
        Legacy->flight_decks.size(),
        Legacy->landing_gear.size(),
        Legacy->squadrons.size(),
        Legacy->weapons.size(),
        Legacy->hardpoints.size());

    return Legacy;
}


    