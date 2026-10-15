#include "TurretCatalog.h"
#include "FighterTurretActor.h"
#include "LCATurretActor.h"
#include "RocketsTurretActor.h"
#include "ReconPodTurretActor.h"
#include "BlockadeCannonTurretActor.h"
#include "PhalanxCannonTurretActor.h"
#include "VikingCannonTurretActor.h"
#include "UrsaCannonTurretActor.h"
#include "OrcaCannonTurretActor.h"
#include "OrcaInvertedTurretActor.h"
#include "Orca2CannonTurretActor.h"
#include "Orca2InvertedTurretActor.h"
#include "VanguardTurretActor.h"
#include "HeliconTurretActor.h"
#include "LightPDBTurretActor.h"
#include "Rail5TurretActor.h"
#include "Rail12TurretActor.h"
#include "Rail12InvertedTurretActor.h"
#include "Rail15TurretActor.h"
#include "Mag18TurretActor.h"
#include "RostovTurretActor.h"
#include "RostovStarboardTurretActor.h"
#include "GraserTurretActor.h"
#include "AAACannonTurretActor.h"
#include "QuadAAATurretActor.h"
#include "SAM1TurretActor.h"
TSubclassOf<ATurretActor> TurretCatalog::FindClass(FName Type)
{
    if(Type==FName(TEXT("Fighter Turret"))) return AFighterTurretActor::StaticClass();
    if(Type==FName(TEXT("LCA Turret"))) return ALCATurretActor::StaticClass();
    if(Type==FName(TEXT("Rockets"))) return ARocketsTurretActor::StaticClass();
    if(Type==FName(TEXT("Recon Pod"))) return AReconPodTurretActor::StaticClass();
    if(Type==FName(TEXT("Blockade Cannon"))) return ABlockadeCannonTurretActor::StaticClass();
    if(Type==FName(TEXT("Phalanx Cannon"))) return APhalanxCannonTurretActor::StaticClass();
    if(Type==FName(TEXT("Viking Cannon"))) return AVikingCannonTurretActor::StaticClass();
    if(Type==FName(TEXT("Ursa Cannon"))) return AUrsaCannonTurretActor::StaticClass();
    if(Type==FName(TEXT("Orca Cannon"))) return AOrcaCannonTurretActor::StaticClass();
    if(Type==FName(TEXT("Orca Inverted"))) return AOrcaInvertedTurretActor::StaticClass();
    if(Type==FName(TEXT("Orca 2 Cannon"))) return AOrca2CannonTurretActor::StaticClass();
    if(Type==FName(TEXT("Orca 2 Inverted"))) return AOrca2InvertedTurretActor::StaticClass();
    if(Type==FName(TEXT("Vanguard"))) return AVanguardTurretActor::StaticClass();
    if(Type==FName(TEXT("Helicon"))) return AHeliconTurretActor::StaticClass();
    if(Type==FName(TEXT("Light-PDB"))) return ALightPDBTurretActor::StaticClass();
    if(Type==FName(TEXT("Rail-5"))) return ARail5TurretActor::StaticClass();
    if(Type==FName(TEXT("Rail-12"))) return ARail12TurretActor::StaticClass();
    if(Type==FName(TEXT("Rail-12 Inverted"))) return ARail12InvertedTurretActor::StaticClass();
    if(Type==FName(TEXT("Rail-15"))) return ARail15TurretActor::StaticClass();
    if(Type==FName(TEXT("Mag-18"))) return AMag18TurretActor::StaticClass();
    if(Type==FName(TEXT("Rostov"))) return ARostovTurretActor::StaticClass();
    if(Type==FName(TEXT("Rostov-Starboard"))) return ARostovStarboardTurretActor::StaticClass();
    if(Type==FName(TEXT("Graser Turret"))) return AGraserTurretActor::StaticClass();
    if(Type==FName(TEXT("AAA Cannon"))) return AAAACannonTurretActor::StaticClass();
    if(Type==FName(TEXT("Quad AAA"))) return AQuadAAATurretActor::StaticClass();
    if(Type==FName(TEXT("SAM-1"))) return ASAM1TurretActor::StaticClass();
    return nullptr;
}
void TurretCatalog::ShipMounts(FName ShipDef,TArray<FShipTurretMountDef>& Out)
{
    Out.Reset();
    if(ShipDef==FName(TEXT("Adventure"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(240.0,52.0,130.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(-240.0,52.0,130.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-3"); M.LegacyLocation=FVector(130.0,-60.0,130.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-4"); M.LegacyLocation=FVector(-130.0,-60.0,130.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-5"); M.LegacyLocation=FVector(90.0,80.0,-580.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-6"); M.LegacyLocation=FVector(-90.0,80.0,-580.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Asher"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port Phalanx"); M.LegacyLocation=FVector(-110.0,15.0,-128.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Starboard Phalanx"); M.LegacyLocation=FVector(110.0,15.0,-128.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Aft Phalanx"); M.LegacyLocation=FVector(0.0,56.0,-190.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Baikal"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Ursa Cannon"); M.MountName=TEXT("Starboard Cannon"); M.LegacyLocation=FVector(85.0,0.0,-30.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Ursa Cannon"); M.MountName=TEXT("Port Cannon"); M.LegacyLocation=FVector(-85.0,0.0,-30.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Berents"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Fwd Cannon"); M.LegacyLocation=FVector(88.0,36.0,-22.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Fwd Cannon"); M.LegacyLocation=FVector(-88.0,36.0,-22.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Aft Cannon"); M.LegacyLocation=FVector(0.0,-26.0,-190.0); M.AzimuthRadians=3.1415927; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Broadsword"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-5"); M.MountName=TEXT("Rail-1"); M.LegacyLocation=FVector(0.0,73.0,-27.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-5"); M.MountName=TEXT("Rail-2"); M.LegacyLocation=FVector(0.0,71.0,-267.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(-31.0,81.0,299.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(31.0,81.0,299.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-3"); M.LegacyLocation=FVector(0.0,-110.0,96.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=-3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-4"); M.LegacyLocation=FVector(0.0,64.0,-486.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-5"); M.LegacyLocation=FVector(0.0,14.0,-486.0); M.AzimuthRadians=0.0; M.ElevationRadians=3.141592653589793; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Courageous"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Gun-1"); M.LegacyLocation=FVector(0.0,87.0,400.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Gun-2"); M.LegacyLocation=FVector(0.0,130.0,-15.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port Phalanx 1"); M.LegacyLocation=FVector(-176.0,78.0,-250.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Stbd Phalanx 1"); M.LegacyLocation=FVector(176.0,78.0,-250.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port Phalanx 2"); M.LegacyLocation=FVector(-176.0,18.0,-220.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Stbd Phalanx 2"); M.LegacyLocation=FVector(176.0,18.0,-220.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Devastator"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Gun-1"); M.LegacyLocation=FVector(0.0,80.0,575.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Gun-2"); M.LegacyLocation=FVector(0.0,110.0,-550.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Gun-3"); M.LegacyLocation=FVector(190.0,72.0,-275.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Gun-4"); M.LegacyLocation=FVector(-190.0,72.0,-275.0); M.AzimuthRadians=4.71238898038469; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Dragon"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(200.0,118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(-200.0,118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-3"); M.LegacyLocation=FVector(200.0,118.0,200.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-4"); M.LegacyLocation=FVector(-200.0,118.0,200.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-5"); M.LegacyLocation=FVector(90.0,155.0,-1250.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-6"); M.LegacyLocation=FVector(-90.0,155.0,-1250.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-7"); M.LegacyLocation=FVector(200.0,-118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-8"); M.LegacyLocation=FVector(-200.0,118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-9"); M.LegacyLocation=FVector(200.0,-118.0,200.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-10"); M.LegacyLocation=FVector(-200.0,-118.0,200.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Fortress"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 1A"); M.LegacyLocation=FVector(0.0,388.0,320.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 1B"); M.LegacyLocation=FVector(0.0,268.0,320.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 2A"); M.LegacyLocation=FVector(280.0,388.0,-160.0); M.AzimuthRadians=2.0943951023931953; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 2B"); M.LegacyLocation=FVector(280.0,268.0,-160.0); M.AzimuthRadians=2.0943951023931953; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 3A"); M.LegacyLocation=FVector(-280.0,388.0,-160.0); M.AzimuthRadians=-2.0943951023931953; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 3B"); M.LegacyLocation=FVector(-280.0,268.0,-160.0); M.AzimuthRadians=-2.0943951023931953; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Gnd_aaa1"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("AAA Cannon"); M.MountName=TEXT("AAA Cannon"); M.LegacyLocation=FVector(0.0,160.0,0.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Gnd_aaa2"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Quad AAA"); M.MountName=TEXT("Quad AAA"); M.LegacyLocation=FVector(0.0,160.0,0.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Gnd_sam1"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("SAM-1"); M.MountName=TEXT("SAM-1"); M.LegacyLocation=FVector(0.0,90.0,0.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Goliath"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("PDB 1"); M.LegacyLocation=FVector(104.0,-17.0,64.0); M.AzimuthRadians=1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("PDB 2"); M.LegacyLocation=FVector(-104.0,-17.0,64.0); M.AzimuthRadians=-1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("PDB 3"); M.LegacyLocation=FVector(104.0,-17.0,-64.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("PDB 4"); M.LegacyLocation=FVector(-104.0,-17.0,-64.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Broadsword"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-5"); M.MountName=TEXT("Rail-1"); M.LegacyLocation=FVector(0.0,73.0,-27.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-5"); M.MountName=TEXT("Rail-2"); M.LegacyLocation=FVector(0.0,71.0,-267.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(-31.0,81.0,299.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(31.0,81.0,299.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-3"); M.LegacyLocation=FVector(0.0,-110.0,96.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=-3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-4"); M.LegacyLocation=FVector(0.0,64.0,-486.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-5"); M.LegacyLocation=FVector(0.0,14.0,-486.0); M.AzimuthRadians=0.0; M.ElevationRadians=3.141592653589793; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Dragon"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(200.0,118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(-200.0,118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-3"); M.LegacyLocation=FVector(200.0,118.0,200.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-4"); M.LegacyLocation=FVector(-200.0,118.0,200.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-5"); M.LegacyLocation=FVector(90.0,155.0,-1250.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB-6"); M.LegacyLocation=FVector(-90.0,155.0,-1250.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-7"); M.LegacyLocation=FVector(200.0,-118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-8"); M.LegacyLocation=FVector(-200.0,118.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-9"); M.LegacyLocation=FVector(200.0,-118.0,200.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("PDB-10"); M.LegacyLocation=FVector(-200.0,-118.0,200.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Imperial"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov"); M.MountName=TEXT("Missile 1"); M.LegacyLocation=FVector(-183.0,29.0,-159.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov-Starboard"); M.MountName=TEXT("Missile 2"); M.LegacyLocation=FVector(183.0,29.0,-159.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,68.0,153.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(-91.0,-47.0,313.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=-3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(91.0,-47.0,313.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,92.0,-228.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(-139.0,72.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(139.0,72.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(139.0,-12.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(-139.0,-12.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Tiger"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(0.0,75.0,220.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(0.0,135.0,-420.0); M.AzimuthRadians=3.14159; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Vendetta"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov"); M.MountName=TEXT("Missile 1"); M.LegacyLocation=FVector(-282.0,40.0,-92.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov-Starboard"); M.MountName=TEXT("Missile 2"); M.LegacyLocation=FVector(282.0,40.0,-92.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,90.0,147.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,105.0,-548.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 1"); M.LegacyLocation=FVector(0.0,73.0,422.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 2"); M.LegacyLocation=FVector(0.0,-143.0,506.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=-3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 3"); M.LegacyLocation=FVector(0.0,-104.0,-360.0); M.AzimuthRadians=0.0; M.ElevationRadians=3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 4"); M.LegacyLocation=FVector(209.0,88.0,-96.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 5"); M.LegacyLocation=FVector(-209.0,88.0,-96.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Volnaris"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 1"); M.LegacyLocation=FVector(0.0,45.0,340.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 2"); M.LegacyLocation=FVector(0.0,55.0,225.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 3"); M.LegacyLocation=FVector(0.0,55.0,-255.0); M.AzimuthRadians=3.14159; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Imperial"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov"); M.MountName=TEXT("Missile 1"); M.LegacyLocation=FVector(-183.0,29.0,-159.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov-Starboard"); M.MountName=TEXT("Missile 2"); M.LegacyLocation=FVector(183.0,29.0,-159.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,68.0,153.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(-91.0,-47.0,313.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=-3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(91.0,-47.0,313.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,92.0,-228.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(-139.0,72.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(139.0,72.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(139.0,-12.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("RAIL12"); M.LegacyLocation=FVector(-139.0,-12.0,-172.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Marauder"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(-110.0,26.0,20.0); M.AzimuthRadians=1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(110.0,26.0,20.0); M.AzimuthRadians=-1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Mine1"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Blockade Cannon"); M.MountName=TEXT("Fwd Cannon"); M.LegacyLocation=FVector(0.0,0.0,0.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Orion"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Vanguard 1"); M.LegacyLocation=FVector(128.0,95.0,704.0); M.AzimuthRadians=1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Vanguard 2"); M.LegacyLocation=FVector(-128.0,95.0,704.0); M.AzimuthRadians=-1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Vanguard 3"); M.LegacyLocation=FVector(128.0,95.0,228.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Vanguard 4"); M.LegacyLocation=FVector(-128.0,95.0,228.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Vanguard 5"); M.LegacyLocation=FVector(64.0,103.0,-830.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Vanguard"); M.MountName=TEXT("Vanguard 6"); M.LegacyLocation=FVector(-64.0,103.0,-830.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port Phalanx"); M.LegacyLocation=FVector(0.0,70.0,960.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port Phalanx"); M.LegacyLocation=FVector(0.0,-75.0,960.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port Phalanx"); M.LegacyLocation=FVector(64.0,-132.0,-830.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port Phalanx"); M.LegacyLocation=FVector(-64.0,-132.0,-830.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("QShip"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(-58.0,64.0,0.0); M.AzimuthRadians=1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(58.0,64.0,0.0); M.AzimuthRadians=-1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-3"); M.LegacyLocation=FVector(-58.0,-64.0,0.0); M.AzimuthRadians=1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Light-PDB"); M.MountName=TEXT("PDB-4"); M.LegacyLocation=FVector(58.0,-64.0,0.0); M.AzimuthRadians=-1.0471975511965976; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("S5"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 1A"); M.LegacyLocation=FVector(1150.0,488.0,0.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 1B"); M.LegacyLocation=FVector(1150.0,320.0,0.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 2A"); M.LegacyLocation=FVector(0.0,488.0,-1150.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 2B"); M.LegacyLocation=FVector(0.0,320.0,-1150.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 3A"); M.LegacyLocation=FVector(-1150.0,488.0,0.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 3B"); M.LegacyLocation=FVector(-1150.0,320.0,0.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 4A"); M.LegacyLocation=FVector(0.0,488.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 4B"); M.LegacyLocation=FVector(0.0,320.0,1150.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("S6"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 1"); M.LegacyLocation=FVector(0.0,118.0,920.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 2"); M.LegacyLocation=FVector(0.0,-118.0,920.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 3"); M.LegacyLocation=FVector(0.0,118.0,-920.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Viking Cannon"); M.MountName=TEXT("Turret 4"); M.LegacyLocation=FVector(0.0,-118.0,-920.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("S7"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Graser Turret"); M.MountName=TEXT("Graser 1"); M.LegacyLocation=FVector(240.0,50.0,100.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Graser Turret"); M.MountName=TEXT("Graser 2"); M.LegacyLocation=FVector(-240.0,50.0,100.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Graser Turret"); M.MountName=TEXT("Graser 3"); M.LegacyLocation=FVector(240.0,50.0,-100.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Graser Turret"); M.MountName=TEXT("Graser 4"); M.LegacyLocation=FVector(-240.0,50.0,-100.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 1A"); M.LegacyLocation=FVector(105.0,36.0,704.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 2A"); M.LegacyLocation=FVector(105.0,36.0,-640.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 3A"); M.LegacyLocation=FVector(-105.0,36.0,704.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("Turret 4A"); M.LegacyLocation=FVector(-105.0,36.0,-640.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 1B"); M.LegacyLocation=FVector(105.0,-24.0,704.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 2B"); M.LegacyLocation=FVector(105.0,-24.0,-640.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 3B"); M.LegacyLocation=FVector(-105.0,-24.0,704.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12 Inverted"); M.MountName=TEXT("Turret 4B"); M.LegacyLocation=FVector(-105.0,-24.0,-640.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Spectre"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port/Fwd Phalanx"); M.LegacyLocation=FVector(-211.0,-50.0,-106.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Stbd/Fwd Phalanx"); M.LegacyLocation=FVector(211.0,-50.0,-106.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Port/Aft Phalanx"); M.LegacyLocation=FVector(-211.0,-50.0,-255.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Phalanx Cannon"); M.MountName=TEXT("Stbd/Aft Phalanx"); M.LegacyLocation=FVector(211.0,-50.0,-255.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Thunderbolt"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Fighter Turret"); M.MountName=TEXT("Fighter Turret"); M.LegacyLocation=FVector(0.0,0.0,-195.0); M.AzimuthRadians=3.1416; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Tiger"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-1"); M.LegacyLocation=FVector(0.0,75.0,220.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Helicon"); M.MountName=TEXT("PDB-2"); M.LegacyLocation=FVector(0.0,135.0,-420.0); M.AzimuthRadians=3.14159; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Vendetta"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov"); M.MountName=TEXT("Missile 1"); M.LegacyLocation=FVector(-282.0,40.0,-92.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rostov-Starboard"); M.MountName=TEXT("Missile 2"); M.LegacyLocation=FVector(282.0,40.0,-92.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,90.0,147.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Mag-18"); M.MountName=TEXT("18INMAG"); M.LegacyLocation=FVector(0.0,105.0,-548.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 1"); M.LegacyLocation=FVector(0.0,73.0,422.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 2"); M.LegacyLocation=FVector(0.0,-143.0,506.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=-3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 3"); M.LegacyLocation=FVector(0.0,-104.0,-360.0); M.AzimuthRadians=0.0; M.ElevationRadians=3.141592653589793; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 4"); M.LegacyLocation=FVector(209.0,88.0,-96.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 5"); M.LegacyLocation=FVector(-209.0,88.0,-96.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Volnaris"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 1"); M.LegacyLocation=FVector(0.0,45.0,340.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 2"); M.LegacyLocation=FVector(0.0,55.0,225.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Rail-12"); M.MountName=TEXT("PDB 3"); M.LegacyLocation=FVector(0.0,55.0,-255.0); M.AzimuthRadians=3.14159; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Wolf"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("Fwd Cannon"); M.LegacyLocation=FVector(0.0,82.0,500.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Inverted"); M.MountName=TEXT("Chin Cannon"); M.LegacyLocation=FVector(0.0,-102.0,500.0); M.AzimuthRadians=0.0; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("Stbd Cannon"); M.LegacyLocation=FVector(60.0,50.0,38.0); M.AzimuthRadians=1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("Port Cannon"); M.LegacyLocation=FVector(-60.0,50.0,38.0); M.AzimuthRadians=-1.5707963267948966; M.ElevationRadians=0.0; Out.Add(M); }
        { FShipTurretMountDef M; M.WeaponType=TEXT("Orca Cannon"); M.MountName=TEXT("Aft Cannon"); M.LegacyLocation=FVector(0.0,70.0,-256.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Zodiac"))) {
        { FShipTurretMountDef M; M.WeaponType=TEXT("LCA Turret"); M.MountName=TEXT("Turret"); M.LegacyLocation=FVector(0.0,74.0,-189.0); M.AzimuthRadians=3.141592653589793; M.ElevationRadians=0.0; Out.Add(M); }
        return;
    }
}
