#include "ShipWeaponMountComponent.h"
void ShipWeaponMountCatalog::Get(FName ShipDef,TArray<FNativeWeaponMountDef>& Out)
{
    Out.Reset();
    if (ShipDef == FName(TEXT("Adventure")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(240.000000000,52.000000000,130.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(-240.000000000,52.000000000,130.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-3"); M.bLoadout=false; M.Location=FVector(130.000000000,-60.000000000,130.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-4"); M.bLoadout=false; M.Location=FVector(-130.000000000,-60.000000000,130.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-5"); M.bLoadout=false; M.Location=FVector(90.000000000,80.000000000,-580.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-6"); M.bLoadout=false; M.Location=FVector(-90.000000000,80.000000000,-580.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Asher")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-22.000000000,-9.000000000,400.000000000);
          M.Muzzle=FVector(-22.000000000,-9.000000000,425.000000000);
          M.Muzzles.Add(FVector(-22.000000000,-9.000000000,425.000000000));
          M.BarrelOffsets.Add(FVector(25.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(0.000000000,-22.000000000,420.000000000);
          M.Muzzle=FVector(0.000000000,-22.000000000,425.000000000);
          M.Muzzles.Add(FVector(0.000000000,-22.000000000,425.000000000));
          M.BarrelOffsets.Add(FVector(5.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(22.000000000,-9.000000000,420.000000000);
          M.Muzzle=FVector(22.000000000,-9.000000000,425.000000000);
          M.Muzzles.Add(FVector(22.000000000,-9.000000000,425.000000000));
          M.BarrelOffsets.Add(FVector(5.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Torpedo"); M.bLoadout=false; M.Location=FVector(100.000000000,-12.000000000,0.000000000);
          M.Muzzle=FVector(106.000000000,-12.000000000,60.000000000);
          M.Muzzles.Add(FVector(106.000000000,-12.000000000,60.000000000));
          M.BarrelOffsets.Add(FVector(60.000000000,6.000000000,0.000000000));
          M.Types.Add(TEXT("Fusion Torpedo"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Torpedo"); M.bLoadout=false; M.Location=FVector(-100.000000000,-12.000000000,0.000000000);
          M.Muzzle=FVector(-106.000000000,-12.000000000,60.000000000);
          M.Muzzles.Add(FVector(-106.000000000,-12.000000000,60.000000000));
          M.BarrelOffsets.Add(FVector(60.000000000,-6.000000000,0.000000000));
          M.Types.Add(TEXT("Fusion Torpedo"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Phalanx"); M.bLoadout=false; M.Location=FVector(-110.000000000,15.000000000,-128.000000000);
          M.Muzzle=FVector(-110.000000000,15.000000000,-128.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Phalanx"); M.bLoadout=false; M.Location=FVector(110.000000000,15.000000000,-128.000000000);
          M.Muzzle=FVector(110.000000000,15.000000000,-128.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Aft Phalanx"); M.bLoadout=false; M.Location=FVector(0.000000000,56.000000000,-190.000000000);
          M.Muzzle=FVector(0.000000000,56.000000000,-190.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Avenger")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,160.000000000);
          M.Muzzle=FVector(-33.000000000,-21.000000000,205.000000000);
          M.Muzzles.Add(FVector(-33.000000000,-21.000000000,205.000000000));
          M.Muzzles.Add(FVector(-17.000000000,-24.000000000,180.000000000));
          M.Muzzles.Add(FVector(17.000000000,-24.000000000,180.000000000));
          M.Muzzles.Add(FVector(31.000000000,-21.000000000,205.000000000));
          M.BarrelOffsets.Add(FVector(45.000000000,-33.000000000,-21.000000000));
          M.BarrelOffsets.Add(FVector(20.000000000,-17.000000000,-24.000000000));
          M.BarrelOffsets.Add(FVector(20.000000000,17.000000000,-24.000000000));
          M.BarrelOffsets.Add(FVector(45.000000000,31.000000000,-21.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-111.000000000,-35.000000000,-100.000000000);
          M.Muzzle=FVector(-111.000000000,-35.000000000,-135.000000000);
          M.Muzzles.Add(FVector(-111.000000000,-35.000000000,-135.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-93.000000000,-34.000000000,-100.000000000);
          M.Muzzle=FVector(-93.000000000,-34.000000000,-130.000000000);
          M.Muzzles.Add(FVector(-93.000000000,-34.000000000,-130.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-76.000000000,-33.000000000,-100.000000000);
          M.Muzzle=FVector(-76.000000000,-33.000000000,-125.000000000);
          M.Muzzles.Add(FVector(-76.000000000,-33.000000000,-125.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-58.000000000,-32.000000000,-100.000000000);
          M.Muzzle=FVector(-58.000000000,-32.000000000,-120.000000000);
          M.Muzzles.Add(FVector(-58.000000000,-32.000000000,-120.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(58.000000000,-32.000000000,-100.000000000);
          M.Muzzle=FVector(58.000000000,-32.000000000,-120.000000000);
          M.Muzzles.Add(FVector(58.000000000,-32.000000000,-120.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(76.000000000,-33.000000000,-100.000000000);
          M.Muzzle=FVector(76.000000000,-33.000000000,-125.000000000);
          M.Muzzles.Add(FVector(76.000000000,-33.000000000,-125.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(93.000000000,-34.000000000,-100.000000000);
          M.Muzzle=FVector(93.000000000,-34.000000000,-130.000000000);
          M.Muzzles.Add(FVector(93.000000000,-34.000000000,-130.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(111.000000000,-35.000000000,-100.000000000);
          M.Muzzle=FVector(111.000000000,-35.000000000,-135.000000000);
          M.Muzzles.Add(FVector(111.000000000,-35.000000000,-135.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Baikal")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Fwd Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,25.000000000,500.000000000);
          M.Muzzle=FVector(0.000000000,25.000000000,540.000000000);
          M.Muzzles.Add(FVector(0.000000000,25.000000000,540.000000000));
          M.BarrelOffsets.Add(FVector(40.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Plasma Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-50.000000000,0.000000000,-40.000000000);
          M.Muzzle=FVector(-20.000000000,0.000000000,450.000000000);
          M.Muzzles.Add(FVector(-20.000000000,0.000000000,450.000000000));
          M.Muzzles.Add(FVector(20.000000000,0.000000000,450.000000000));
          M.BarrelOffsets.Add(FVector(490.000000000,30.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(490.000000000,70.000000000,0.000000000));
          M.Types.Add(TEXT("Mk7 Interceptor"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Cannon"); M.bLoadout=false; M.Location=FVector(85.000000000,0.000000000,-30.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,70.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,70.000000000));
          M.BarrelOffsets.Add(FVector(87.500000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Ursa Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-85.000000000,0.000000000,-30.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,70.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,70.000000000));
          M.BarrelOffsets.Add(FVector(87.500000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Ursa Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Berents")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Plasma Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,300.000000000);
          M.Muzzle=FVector(0.000000000,4.000000000,333.000000000);
          M.Muzzles.Add(FVector(0.000000000,4.000000000,333.000000000));
          M.BarrelOffsets.Add(FVector(33.000000000,0.000000000,4.000000000));
          M.Types.Add(TEXT("Plasma Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-33.000000000,0.000000000,220.000000000);
          M.Muzzle=FVector(-33.000000000,33.000000000,233.000000000);
          M.Muzzles.Add(FVector(-33.000000000,33.000000000,233.000000000));
          M.BarrelOffsets.Add(FVector(12.999955450,0.034033882,33.000000000));
          M.Types.Add(TEXT("Mk7 Interceptor"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Interceptor 2"); M.bLoadout=false; M.Location=FVector(33.000000000,0.000000000,220.000000000);
          M.Muzzle=FVector(33.000000000,33.000000000,233.000000000);
          M.Muzzles.Add(FVector(33.000000000,33.000000000,233.000000000));
          M.BarrelOffsets.Add(FVector(12.999955450,-0.034033882,33.000000000));
          M.Types.Add(TEXT("Mk7 Interceptor"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Fwd Cannon"); M.bLoadout=false; M.Location=FVector(88.000000000,36.000000000,-22.000000000);
          M.Muzzle=FVector(88.000000000,36.000000000,-22.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Fwd Cannon"); M.bLoadout=false; M.Location=FVector(-88.000000000,36.000000000,-22.000000000);
          M.Muzzle=FVector(-88.000000000,36.000000000,-22.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Aft Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,-26.000000000,-190.000000000);
          M.Muzzle=FVector(0.000000000,-26.000000000,-190.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Broadsword")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-45.000000000,-33.000000000,340.000000000);
          M.Muzzle=FVector(-55.000000000,-33.000000000,470.000000000);
          M.Muzzles.Add(FVector(-55.000000000,-33.000000000,470.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,-10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(44.000000000,-33.000000000,340.000000000);
          M.Muzzle=FVector(54.000000000,-33.000000000,470.000000000);
          M.Muzzles.Add(FVector(54.000000000,-33.000000000,470.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-45.000000000,-55.000000000,340.000000000);
          M.Muzzle=FVector(-55.000000000,-55.000000000,448.000000000);
          M.Muzzles.Add(FVector(-55.000000000,-55.000000000,448.000000000));
          M.BarrelOffsets.Add(FVector(108.000000000,-10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(44.000000000,-55.000000000,340.000000000);
          M.Muzzle=FVector(54.000000000,-55.000000000,448.000000000);
          M.Muzzles.Add(FVector(54.000000000,-55.000000000,448.000000000));
          M.BarrelOffsets.Add(FVector(108.000000000,10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile Array"); M.bLoadout=false; M.Location=FVector(0.000000000,-1.000000000,220.000000000);
          M.Muzzle=FVector(-25.000000000,-1.000000000,422.000000000);
          M.Muzzles.Add(FVector(-25.000000000,-1.000000000,422.000000000));
          M.Muzzles.Add(FVector(0.000000000,-1.000000000,422.000000000));
          M.Muzzles.Add(FVector(25.000000000,-1.000000000,422.000000000));
          M.BarrelOffsets.Add(FVector(202.000000000,-25.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(202.000000000,0.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(202.000000000,25.000000000,0.000000000));
          M.Types.Add(TEXT("Nike"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Rail-1"); M.bLoadout=false; M.Location=FVector(0.000000000,73.000000000,-27.000000000);
          M.Muzzle=FVector(0.000000000,73.000000000,-27.000000000);
          M.BarrelOffsets.Add(FVector(42.857142857,2.285714286,6.285714286));
          M.BarrelOffsets.Add(FVector(42.857142857,-2.285714286,6.285714286));
          M.Types.Add(TEXT("Rail-5"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Rail-2"); M.bLoadout=false; M.Location=FVector(0.000000000,71.000000000,-267.000000000);
          M.Muzzle=FVector(0.000000000,71.000000000,-267.000000000);
          M.BarrelOffsets.Add(FVector(42.857142857,2.285714286,6.285714286));
          M.BarrelOffsets.Add(FVector(42.857142857,-2.285714286,6.285714286));
          M.Types.Add(TEXT("Rail-5"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(-31.000000000,81.000000000,299.000000000);
          M.Muzzle=FVector(-31.000000000,81.000000000,299.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(31.000000000,81.000000000,299.000000000);
          M.Muzzle=FVector(31.000000000,81.000000000,299.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-3"); M.bLoadout=false; M.Location=FVector(0.000000000,-110.000000000,96.000000000);
          M.Muzzle=FVector(0.000000000,-110.000000000,96.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-4"); M.bLoadout=false; M.Location=FVector(0.000000000,64.000000000,-486.000000000);
          M.Muzzle=FVector(0.000000000,64.000000000,-486.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-5"); M.bLoadout=false; M.Location=FVector(0.000000000,14.000000000,-486.000000000);
          M.Muzzle=FVector(0.000000000,14.000000000,-486.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Cobra")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,-32.000000000);
          M.Muzzle=FVector(-7.000000000,8.000000000,175.000000000);
          M.Muzzles.Add(FVector(-7.000000000,8.000000000,175.000000000));
          M.Muzzles.Add(FVector(7.000000000,8.000000000,175.000000000));
          M.BarrelOffsets.Add(FVector(207.000000000,-7.000000000,8.000000000));
          M.BarrelOffsets.Add(FVector(207.000000000,7.000000000,8.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(129.000000000,-21.000000000,-30.000000000);
          M.Muzzle=FVector(129.000000000,-21.000000000,-70.000000000);
          M.Muzzles.Add(FVector(129.000000000,-21.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(105.000000000,-18.000000000,-30.000000000);
          M.Muzzle=FVector(105.000000000,-18.000000000,-70.000000000);
          M.Muzzles.Add(FVector(105.000000000,-18.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(65.000000000,-16.000000000,-30.000000000);
          M.Muzzle=FVector(65.000000000,-16.000000000,-70.000000000);
          M.Muzzles.Add(FVector(65.000000000,-16.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(40.000000000,-14.000000000,-30.000000000);
          M.Muzzle=FVector(40.000000000,-14.000000000,-70.000000000);
          M.Muzzles.Add(FVector(40.000000000,-14.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-40.000000000,-14.000000000,-30.000000000);
          M.Muzzle=FVector(-40.000000000,-14.000000000,-70.000000000);
          M.Muzzles.Add(FVector(-40.000000000,-14.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-65.000000000,-16.000000000,-30.000000000);
          M.Muzzle=FVector(-65.000000000,-16.000000000,-70.000000000);
          M.Muzzles.Add(FVector(-65.000000000,-16.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(-105.000000000,-18.000000000,-30.000000000);
          M.Muzzle=FVector(-105.000000000,-18.000000000,-70.000000000);
          M.Muzzles.Add(FVector(-105.000000000,-18.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(-128.000000000,-21.000000000,-30.000000000);
          M.Muzzle=FVector(-128.000000000,-21.000000000,-70.000000000);
          M.Muzzles.Add(FVector(-128.000000000,-21.000000000,-70.000000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Courageous")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-40.000000000,-12.000000000,600.000000000);
          M.Muzzle=FVector(-51.000000000,-12.000000000,641.000000000);
          M.Muzzles.Add(FVector(-51.000000000,-12.000000000,641.000000000));
          M.BarrelOffsets.Add(FVector(41.000000000,-11.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(40.000000000,-12.000000000,600.000000000);
          M.Muzzle=FVector(51.000000000,-12.000000000,641.000000000);
          M.Muzzles.Add(FVector(51.000000000,-12.000000000,641.000000000));
          M.BarrelOffsets.Add(FVector(41.000000000,11.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-30.000000000,-45.000000000,580.000000000);
          M.Muzzle=FVector(-37.000000000,-45.000000000,623.000000000);
          M.Muzzles.Add(FVector(-37.000000000,-45.000000000,623.000000000));
          M.BarrelOffsets.Add(FVector(43.000000000,-7.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(30.000000000,-45.000000000,580.000000000);
          M.Muzzle=FVector(37.000000000,-45.000000000,623.000000000);
          M.Muzzles.Add(FVector(37.000000000,-45.000000000,623.000000000));
          M.BarrelOffsets.Add(FVector(43.000000000,7.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Missile"); M.bLoadout=false; M.Location=FVector(100.000000000,22.000000000,250.000000000);
          M.Muzzle=FVector(70.000000000,55.000000000,355.000000000);
          M.Muzzles.Add(FVector(70.000000000,55.000000000,355.000000000));
          M.Muzzles.Add(FVector(80.000000000,45.000000000,355.000000000));
          M.BarrelOffsets.Add(FVector(93.657640407,-56.153774524,33.000000000));
          M.BarrelOffsets.Add(FVector(96.245830858,-46.494516262,23.000000000));
          M.Types.Add(TEXT("Nike"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Missile"); M.bLoadout=false; M.Location=FVector(-100.000000000,22.000000000,250.000000000);
          M.Muzzle=FVector(-70.000000000,55.000000000,355.000000000);
          M.Muzzles.Add(FVector(-70.000000000,55.000000000,355.000000000));
          M.Muzzles.Add(FVector(-80.000000000,45.000000000,355.000000000));
          M.BarrelOffsets.Add(FVector(93.657640407,56.153774524,33.000000000));
          M.BarrelOffsets.Add(FVector(96.245830858,46.494516262,23.000000000));
          M.Types.Add(TEXT("Nike"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Interceptor"); M.bLoadout=false; M.Location=FVector(0.000000000,30.000000000,250.000000000);
          M.Muzzle=FVector(35.000000000,75.000000000,355.000000000);
          M.Muzzles.Add(FVector(35.000000000,75.000000000,355.000000000));
          M.Muzzles.Add(FVector(50.000000000,70.000000000,355.000000000));
          M.Muzzles.Add(FVector(-35.000000000,75.000000000,355.000000000));
          M.Muzzles.Add(FVector(-50.000000000,70.000000000,355.000000000));
          M.BarrelOffsets.Add(FVector(105.000000000,35.000000000,45.000000000));
          M.BarrelOffsets.Add(FVector(105.000000000,50.000000000,40.000000000));
          M.BarrelOffsets.Add(FVector(105.000000000,-35.000000000,45.000000000));
          M.BarrelOffsets.Add(FVector(105.000000000,-50.000000000,40.000000000));
          M.Types.Add(TEXT("Mk7 Interceptor"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-1"); M.bLoadout=false; M.Location=FVector(0.000000000,87.000000000,400.000000000);
          M.Muzzle=FVector(0.000000000,87.000000000,400.000000000);
          M.BarrelOffsets.Add(FVector(32.727272727,3.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(32.727272727,-3.818181818,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-2"); M.bLoadout=false; M.Location=FVector(0.000000000,130.000000000,-15.000000000);
          M.Muzzle=FVector(0.000000000,130.000000000,-15.000000000);
          M.BarrelOffsets.Add(FVector(32.727272727,3.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(32.727272727,-3.818181818,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Phalanx 1"); M.bLoadout=false; M.Location=FVector(-176.000000000,78.000000000,-250.000000000);
          M.Muzzle=FVector(-176.000000000,78.000000000,-250.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,2.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-2.181818182,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Stbd Phalanx 1"); M.bLoadout=false; M.Location=FVector(176.000000000,78.000000000,-250.000000000);
          M.Muzzle=FVector(176.000000000,78.000000000,-250.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,2.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-2.181818182,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Phalanx 2"); M.bLoadout=false; M.Location=FVector(-176.000000000,18.000000000,-220.000000000);
          M.Muzzle=FVector(-176.000000000,18.000000000,-220.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,2.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-2.181818182,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Stbd Phalanx 2"); M.bLoadout=false; M.Location=FVector(176.000000000,18.000000000,-220.000000000);
          M.Muzzle=FVector(176.000000000,18.000000000,-220.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,2.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-2.181818182,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Courier")))
    {
        return;
    }
    if (ShipDef == FName(TEXT("Cutlass")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,32.000000000);
          M.Muzzle=FVector(-150.000000000,5.000000000,-10.000000000);
          M.Muzzles.Add(FVector(-150.000000000,5.000000000,-10.000000000));
          M.Muzzles.Add(FVector(-150.000000000,-6.000000000,-10.000000000));
          M.Muzzles.Add(FVector(150.000000000,5.000000000,-10.000000000));
          M.Muzzles.Add(FVector(150.000000000,-6.000000000,-10.000000000));
          M.BarrelOffsets.Add(FVector(-42.000000000,-150.000000000,5.000000000));
          M.BarrelOffsets.Add(FVector(-42.000000000,-150.000000000,-6.000000000));
          M.BarrelOffsets.Add(FVector(-42.000000000,150.000000000,5.000000000));
          M.BarrelOffsets.Add(FVector(-42.000000000,150.000000000,-6.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-46.000000000,-10.000000000,-25.000000000);
          M.Muzzle=FVector(-64.000000000,-10.000000000,-100.000000000);
          M.Muzzles.Add(FVector(-64.000000000,-10.000000000,-100.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(64.000000000,-10.000000000,-25.000000000);
          M.Muzzle=FVector(64.000000000,-10.000000000,-100.000000000);
          M.Muzzles.Add(FVector(64.000000000,-10.000000000,-100.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Devastator")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-65.000000000,-22.000000000,730.000000000);
          M.Muzzle=FVector(-65.000000000,-22.000000000,760.000000000);
          M.Muzzles.Add(FVector(-65.000000000,-22.000000000,760.000000000));
          M.BarrelOffsets.Add(FVector(30.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(65.000000000,-22.000000000,730.000000000);
          M.Muzzle=FVector(65.000000000,-22.000000000,760.000000000);
          M.Muzzles.Add(FVector(65.000000000,-22.000000000,760.000000000));
          M.BarrelOffsets.Add(FVector(30.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 33"); M.bLoadout=false; M.Location=FVector(-52.000000000,-54.000000000,710.000000000);
          M.Muzzle=FVector(-52.000000000,-54.000000000,740.000000000);
          M.Muzzles.Add(FVector(-52.000000000,-54.000000000,740.000000000));
          M.BarrelOffsets.Add(FVector(30.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 4"); M.bLoadout=false; M.Location=FVector(52.000000000,-54.000000000,710.000000000);
          M.Muzzle=FVector(52.000000000,-54.000000000,740.000000000);
          M.Muzzles.Add(FVector(52.000000000,-54.000000000,740.000000000));
          M.BarrelOffsets.Add(FVector(30.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Missile"); M.bLoadout=false; M.Location=FVector(60.000000000,0.000000000,400.000000000);
          M.Muzzle=FVector(75.000000000,13.000000000,600.000000000);
          M.Muzzles.Add(FVector(75.000000000,13.000000000,600.000000000));
          M.Muzzles.Add(FVector(65.000000000,13.000000000,600.000000000));
          M.BarrelOffsets.Add(FVector(197.067450934,-37.274921626,13.000000000));
          M.BarrelOffsets.Add(FVector(194.479260483,-46.934179889,13.000000000));
          M.Types.Add(TEXT("Athena1"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Missile"); M.bLoadout=false; M.Location=FVector(-60.000000000,0.000000000,400.000000000);
          M.Muzzle=FVector(-75.000000000,13.000000000,600.000000000);
          M.Muzzles.Add(FVector(-75.000000000,13.000000000,600.000000000));
          M.Muzzles.Add(FVector(-65.000000000,13.000000000,600.000000000));
          M.BarrelOffsets.Add(FVector(197.067450934,37.274921626,13.000000000));
          M.BarrelOffsets.Add(FVector(194.479260483,46.934179889,13.000000000));
          M.Types.Add(TEXT("Athena2"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-1"); M.bLoadout=false; M.Location=FVector(0.000000000,80.000000000,575.000000000);
          M.Muzzle=FVector(0.000000000,80.000000000,575.000000000);
          M.BarrelOffsets.Add(FVector(28.800000000,3.360000000,0.000000000));
          M.BarrelOffsets.Add(FVector(28.800000000,-3.360000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-2"); M.bLoadout=false; M.Location=FVector(0.000000000,110.000000000,-550.000000000);
          M.Muzzle=FVector(0.000000000,110.000000000,-550.000000000);
          M.BarrelOffsets.Add(FVector(28.800000000,3.360000000,0.000000000));
          M.BarrelOffsets.Add(FVector(28.800000000,-3.360000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-3"); M.bLoadout=false; M.Location=FVector(190.000000000,72.000000000,-275.000000000);
          M.Muzzle=FVector(190.000000000,72.000000000,-275.000000000);
          M.BarrelOffsets.Add(FVector(28.800000000,3.360000000,0.000000000));
          M.BarrelOffsets.Add(FVector(28.800000000,-3.360000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-4"); M.bLoadout=false; M.Location=FVector(-190.000000000,72.000000000,-275.000000000);
          M.Muzzle=FVector(-190.000000000,72.000000000,-275.000000000);
          M.BarrelOffsets.Add(FVector(28.800000000,3.360000000,0.000000000));
          M.BarrelOffsets.Add(FVector(28.800000000,-3.360000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Dragon")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-30.000000000,-15.000000000,330.000000000);
          M.Muzzle=FVector(-283.000000000,6.000000000,1650.000000000);
          M.Muzzles.Add(FVector(-283.000000000,6.000000000,1650.000000000));
          M.BarrelOffsets.Add(FVector(1320.000000000,-253.000000000,21.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(30.000000000,-15.000000000,330.000000000);
          M.Muzzle=FVector(283.000000000,6.000000000,1650.000000000);
          M.Muzzles.Add(FVector(283.000000000,6.000000000,1650.000000000));
          M.BarrelOffsets.Add(FVector(1320.000000000,253.000000000,21.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(200.000000000,118.000000000,1150.000000000);
          M.Muzzle=FVector(200.000000000,118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(-200.000000000,118.000000000,1150.000000000);
          M.Muzzle=FVector(-200.000000000,118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-3"); M.bLoadout=false; M.Location=FVector(200.000000000,118.000000000,200.000000000);
          M.Muzzle=FVector(200.000000000,118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-4"); M.bLoadout=false; M.Location=FVector(-200.000000000,118.000000000,200.000000000);
          M.Muzzle=FVector(-200.000000000,118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-5"); M.bLoadout=false; M.Location=FVector(90.000000000,155.000000000,-1250.000000000);
          M.Muzzle=FVector(90.000000000,155.000000000,-1250.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-6"); M.bLoadout=false; M.Location=FVector(-90.000000000,155.000000000,-1250.000000000);
          M.Muzzle=FVector(-90.000000000,155.000000000,-1250.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-7"); M.bLoadout=false; M.Location=FVector(200.000000000,-118.000000000,1150.000000000);
          M.Muzzle=FVector(200.000000000,-118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-8"); M.bLoadout=false; M.Location=FVector(-200.000000000,118.000000000,1150.000000000);
          M.Muzzle=FVector(-200.000000000,118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-9"); M.bLoadout=false; M.Location=FVector(200.000000000,-118.000000000,200.000000000);
          M.Muzzle=FVector(200.000000000,-118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-10"); M.bLoadout=false; M.Location=FVector(-200.000000000,-118.000000000,200.000000000);
          M.Muzzle=FVector(-200.000000000,-118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Eagle")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,32.000000000);
          M.Muzzle=FVector(-19.000000000,-3.000000000,177.000000000);
          M.Muzzles.Add(FVector(-19.000000000,-3.000000000,177.000000000));
          M.Muzzles.Add(FVector(19.000000000,-3.000000000,177.000000000));
          M.BarrelOffsets.Add(FVector(145.000000000,-19.000000000,-3.000000000));
          M.BarrelOffsets.Add(FVector(145.000000000,19.000000000,-3.000000000));
          M.Types.Add(TEXT("Delta Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-69.000000000,-22.000000000,-60.000000000);
          M.Muzzle=FVector(-69.000000000,-22.000000000,-75.000000000);
          M.Muzzles.Add(FVector(-69.000000000,-22.000000000,-75.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-48.000000000,-17.000000000,-60.000000000);
          M.Muzzle=FVector(-48.000000000,-17.000000000,-75.000000000);
          M.Muzzles.Add(FVector(-48.000000000,-17.000000000,-75.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-20.000000000,-13.000000000,-15.000000000);
          M.Muzzle=FVector(-20.000000000,-13.000000000,-25.000000000);
          M.Muzzles.Add(FVector(-20.000000000,-13.000000000,-25.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-14.000000000,-14.000000000,60.000000000);
          M.Muzzle=FVector(-14.000000000,-14.000000000,52.000000000);
          M.Muzzles.Add(FVector(-14.000000000,-14.000000000,52.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(15.000000000,-14.000000000,60.000000000);
          M.Muzzle=FVector(15.000000000,-14.000000000,52.000000000);
          M.Muzzles.Add(FVector(15.000000000,-14.000000000,52.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(20.000000000,-13.000000000,-15.000000000);
          M.Muzzle=FVector(20.000000000,-13.000000000,-25.000000000);
          M.Muzzles.Add(FVector(20.000000000,-13.000000000,-25.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(48.000000000,-17.000000000,-60.000000000);
          M.Muzzle=FVector(48.000000000,-17.000000000,-75.000000000);
          M.Muzzles.Add(FVector(48.000000000,-17.000000000,-75.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(70.000000000,-22.000000000,-60.000000000);
          M.Muzzle=FVector(70.000000000,-22.000000000,-75.000000000);
          M.Muzzles.Add(FVector(70.000000000,-22.000000000,-75.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Falcon")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,-32.000000000);
          M.Muzzle=FVector(-21.000000000,0.000000000,130.000000000);
          M.Muzzles.Add(FVector(-21.000000000,0.000000000,130.000000000));
          M.Muzzles.Add(FVector(21.000000000,0.000000000,130.000000000));
          M.BarrelOffsets.Add(FVector(162.000000000,-21.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(162.000000000,21.000000000,0.000000000));
          M.Types.Add(TEXT("Delta Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-110.000000000,-7.000000000,-145.000000000);
          M.Muzzle=FVector(-110.000000000,-7.000000000,-145.000000000);
          M.Muzzles.Add(FVector(-110.000000000,-7.000000000,-145.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-93.000000000,-5.000000000,-145.000000000);
          M.Muzzle=FVector(-93.000000000,-5.000000000,-145.000000000);
          M.Muzzles.Add(FVector(-93.000000000,-5.000000000,-145.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(93.000000000,-5.000000000,-145.000000000);
          M.Muzzle=FVector(93.000000000,-5.000000000,-145.000000000);
          M.Muzzles.Add(FVector(93.000000000,-5.000000000,-145.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(110.000000000,-7.000000000,-145.000000000);
          M.Muzzle=FVector(110.000000000,-7.000000000,-145.000000000);
          M.Muzzles.Add(FVector(110.000000000,-7.000000000,-145.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Farcaster")))
    {
        return;
    }
    if (ShipDef == FName(TEXT("Fortress")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 1A"); M.bLoadout=false; M.Location=FVector(0.000000000,388.000000000,320.000000000);
          M.Muzzle=FVector(0.000000000,388.000000000,320.000000000);
          M.BarrelOffsets.Add(FVector(29.545454545,1.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(29.545454545,-1.818181818,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 1B"); M.bLoadout=false; M.Location=FVector(0.000000000,268.000000000,320.000000000);
          M.Muzzle=FVector(0.000000000,268.000000000,320.000000000);
          M.BarrelOffsets.Add(FVector(29.545454545,1.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(29.545454545,-1.818181818,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 2A"); M.bLoadout=false; M.Location=FVector(280.000000000,388.000000000,-160.000000000);
          M.Muzzle=FVector(280.000000000,388.000000000,-160.000000000);
          M.BarrelOffsets.Add(FVector(29.545454545,1.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(29.545454545,-1.818181818,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 2B"); M.bLoadout=false; M.Location=FVector(280.000000000,268.000000000,-160.000000000);
          M.Muzzle=FVector(280.000000000,268.000000000,-160.000000000);
          M.BarrelOffsets.Add(FVector(29.545454545,1.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(29.545454545,-1.818181818,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 3A"); M.bLoadout=false; M.Location=FVector(-280.000000000,388.000000000,-160.000000000);
          M.Muzzle=FVector(-280.000000000,388.000000000,-160.000000000);
          M.BarrelOffsets.Add(FVector(29.545454545,1.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(29.545454545,-1.818181818,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 3B"); M.bLoadout=false; M.Location=FVector(-280.000000000,268.000000000,-160.000000000);
          M.Muzzle=FVector(-280.000000000,268.000000000,-160.000000000);
          M.BarrelOffsets.Add(FVector(29.545454545,1.818181818,0.000000000));
          M.BarrelOffsets.Add(FVector(29.545454545,-1.818181818,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Gnd_aaa1")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,160.000000000,0.000000000);
          M.Muzzle=FVector(-20.000000000,0.000000000,70.000000000);
          M.Muzzles.Add(FVector(-20.000000000,0.000000000,70.000000000));
          M.Muzzles.Add(FVector(20.000000000,0.000000000,70.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,-20.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,20.000000000,0.000000000));
          M.Types.Add(TEXT("AAA Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Gnd_aaa2")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,160.000000000,0.000000000);
          M.Muzzle=FVector(0.000000000,160.000000000,0.000000000);
          M.BarrelOffsets.Add(FVector(68.000000000,29.000000000,9.000000000));
          M.BarrelOffsets.Add(FVector(68.000000000,-29.000000000,9.000000000));
          M.BarrelOffsets.Add(FVector(68.000000000,29.000000000,-8.000000000));
          M.BarrelOffsets.Add(FVector(68.000000000,-29.000000000,-8.000000000));
          M.Types.Add(TEXT("Quad AAA"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Gnd_sam1")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("SAM"); M.bLoadout=false; M.Location=FVector(0.000000000,90.000000000,0.000000000);
          M.Muzzle=FVector(-50.000000000,10.000000000,-36.000000000);
          M.Muzzles.Add(FVector(-50.000000000,10.000000000,-36.000000000));
          M.Muzzles.Add(FVector(-50.000000000,-9.000000000,-36.000000000));
          M.Muzzles.Add(FVector(-12.000000000,10.000000000,-36.000000000));
          M.Muzzles.Add(FVector(-12.000000000,-9.000000000,-36.000000000));
          M.Muzzles.Add(FVector(12.000000000,10.000000000,-36.000000000));
          M.Muzzles.Add(FVector(12.000000000,-9.000000000,-36.000000000));
          M.Muzzles.Add(FVector(50.000000000,10.000000000,-36.000000000));
          M.Muzzles.Add(FVector(50.000000000,-9.000000000,-36.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,-50.000000000,-80.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,-50.000000000,-99.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,-12.000000000,-80.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,-12.000000000,-99.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,12.000000000,-80.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,12.000000000,-99.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,50.000000000,-80.000000000));
          M.BarrelOffsets.Add(FVector(-36.000000000,50.000000000,-99.000000000));
          M.Types.Add(TEXT("SAM-1"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Goliath")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 1"); M.bLoadout=false; M.Location=FVector(104.000000000,-17.000000000,64.000000000);
          M.Muzzle=FVector(30.000000000,0.000000000,190.000000000);
          M.Muzzles.Add(FVector(30.000000000,0.000000000,190.000000000));
          M.Muzzles.Add(FVector(-30.000000000,0.000000000,190.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,18.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,-18.181818182,0.000000000));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 2"); M.bLoadout=false; M.Location=FVector(-104.000000000,-17.000000000,64.000000000);
          M.Muzzle=FVector(30.000000000,0.000000000,190.000000000);
          M.Muzzles.Add(FVector(30.000000000,0.000000000,190.000000000));
          M.Muzzles.Add(FVector(-30.000000000,0.000000000,190.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,18.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,-18.181818182,0.000000000));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 3"); M.bLoadout=false; M.Location=FVector(104.000000000,-17.000000000,-64.000000000);
          M.Muzzle=FVector(30.000000000,0.000000000,190.000000000);
          M.Muzzles.Add(FVector(30.000000000,0.000000000,190.000000000));
          M.Muzzles.Add(FVector(-30.000000000,0.000000000,190.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,18.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,-18.181818182,0.000000000));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 4"); M.bLoadout=false; M.Location=FVector(-104.000000000,-17.000000000,-64.000000000);
          M.Muzzle=FVector(30.000000000,0.000000000,190.000000000);
          M.Muzzles.Add(FVector(30.000000000,0.000000000,190.000000000));
          M.Muzzles.Add(FVector(-30.000000000,0.000000000,190.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,18.181818182,0.000000000));
          M.BarrelOffsets.Add(FVector(115.151515152,-18.181818182,0.000000000));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Hulk-Broadsword")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-45.000000000,-33.000000000,340.000000000);
          M.Muzzle=FVector(-55.000000000,-33.000000000,470.000000000);
          M.Muzzles.Add(FVector(-55.000000000,-33.000000000,470.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,-10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(44.000000000,-33.000000000,340.000000000);
          M.Muzzle=FVector(54.000000000,-33.000000000,470.000000000);
          M.Muzzles.Add(FVector(54.000000000,-33.000000000,470.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-45.000000000,-55.000000000,340.000000000);
          M.Muzzle=FVector(-55.000000000,-55.000000000,448.000000000);
          M.Muzzles.Add(FVector(-55.000000000,-55.000000000,448.000000000));
          M.BarrelOffsets.Add(FVector(108.000000000,-10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(44.000000000,-55.000000000,340.000000000);
          M.Muzzle=FVector(54.000000000,-55.000000000,448.000000000);
          M.Muzzles.Add(FVector(54.000000000,-55.000000000,448.000000000));
          M.BarrelOffsets.Add(FVector(108.000000000,10.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile Array"); M.bLoadout=false; M.Location=FVector(0.000000000,-1.000000000,220.000000000);
          M.Muzzle=FVector(-25.000000000,-1.000000000,422.000000000);
          M.Muzzles.Add(FVector(-25.000000000,-1.000000000,422.000000000));
          M.Muzzles.Add(FVector(0.000000000,-1.000000000,422.000000000));
          M.Muzzles.Add(FVector(25.000000000,-1.000000000,422.000000000));
          M.BarrelOffsets.Add(FVector(202.000000000,-25.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(202.000000000,0.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(202.000000000,25.000000000,0.000000000));
          M.Types.Add(TEXT("Nike"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Rail-1"); M.bLoadout=false; M.Location=FVector(0.000000000,73.000000000,-27.000000000);
          M.Muzzle=FVector(0.000000000,73.000000000,-27.000000000);
          M.BarrelOffsets.Add(FVector(42.857142857,2.285714286,6.285714286));
          M.BarrelOffsets.Add(FVector(42.857142857,-2.285714286,6.285714286));
          M.Types.Add(TEXT("Rail-5"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Rail-2"); M.bLoadout=false; M.Location=FVector(0.000000000,71.000000000,-267.000000000);
          M.Muzzle=FVector(0.000000000,71.000000000,-267.000000000);
          M.BarrelOffsets.Add(FVector(42.857142857,2.285714286,6.285714286));
          M.BarrelOffsets.Add(FVector(42.857142857,-2.285714286,6.285714286));
          M.Types.Add(TEXT("Rail-5"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(-31.000000000,81.000000000,299.000000000);
          M.Muzzle=FVector(-31.000000000,81.000000000,299.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(31.000000000,81.000000000,299.000000000);
          M.Muzzle=FVector(31.000000000,81.000000000,299.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-3"); M.bLoadout=false; M.Location=FVector(0.000000000,-110.000000000,96.000000000);
          M.Muzzle=FVector(0.000000000,-110.000000000,96.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-4"); M.bLoadout=false; M.Location=FVector(0.000000000,64.000000000,-486.000000000);
          M.Muzzle=FVector(0.000000000,64.000000000,-486.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-5"); M.bLoadout=false; M.Location=FVector(0.000000000,14.000000000,-486.000000000);
          M.Muzzle=FVector(0.000000000,14.000000000,-486.000000000);
          M.BarrelOffsets.Add(FVector(37.428571429,0.000000000,2.857142857));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Hulk-Dragon")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-30.000000000,-15.000000000,330.000000000);
          M.Muzzle=FVector(-283.000000000,6.000000000,1650.000000000);
          M.Muzzles.Add(FVector(-283.000000000,6.000000000,1650.000000000));
          M.BarrelOffsets.Add(FVector(1320.000000000,-253.000000000,21.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(30.000000000,-15.000000000,330.000000000);
          M.Muzzle=FVector(283.000000000,6.000000000,1650.000000000);
          M.Muzzles.Add(FVector(283.000000000,6.000000000,1650.000000000));
          M.BarrelOffsets.Add(FVector(1320.000000000,253.000000000,21.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(200.000000000,118.000000000,1150.000000000);
          M.Muzzle=FVector(200.000000000,118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(-200.000000000,118.000000000,1150.000000000);
          M.Muzzle=FVector(-200.000000000,118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-3"); M.bLoadout=false; M.Location=FVector(200.000000000,118.000000000,200.000000000);
          M.Muzzle=FVector(200.000000000,118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-4"); M.bLoadout=false; M.Location=FVector(-200.000000000,118.000000000,200.000000000);
          M.Muzzle=FVector(-200.000000000,118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-5"); M.bLoadout=false; M.Location=FVector(90.000000000,155.000000000,-1250.000000000);
          M.Muzzle=FVector(90.000000000,155.000000000,-1250.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-6"); M.bLoadout=false; M.Location=FVector(-90.000000000,155.000000000,-1250.000000000);
          M.Muzzle=FVector(-90.000000000,155.000000000,-1250.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-7"); M.bLoadout=false; M.Location=FVector(200.000000000,-118.000000000,1150.000000000);
          M.Muzzle=FVector(200.000000000,-118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-8"); M.bLoadout=false; M.Location=FVector(-200.000000000,118.000000000,1150.000000000);
          M.Muzzle=FVector(-200.000000000,118.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-9"); M.bLoadout=false; M.Location=FVector(200.000000000,-118.000000000,200.000000000);
          M.Muzzle=FVector(200.000000000,-118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-10"); M.bLoadout=false; M.Location=FVector(-200.000000000,-118.000000000,200.000000000);
          M.Muzzle=FVector(-200.000000000,-118.000000000,200.000000000);
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,-11.000000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Hulk-Imperial")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-42.000000000,-9.000000000,620.000000000);
          M.Muzzle=FVector(-42.000000000,-9.000000000,640.000000000);
          M.Muzzles.Add(FVector(-42.000000000,-9.000000000,640.000000000));
          M.BarrelOffsets.Add(FVector(20.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(42.000000000,-9.000000000,620.000000000);
          M.Muzzle=FVector(42.000000000,-9.000000000,640.000000000);
          M.Muzzles.Add(FVector(42.000000000,-9.000000000,640.000000000));
          M.BarrelOffsets.Add(FVector(20.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 1"); M.bLoadout=false; M.Location=FVector(-183.000000000,29.000000000,-159.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(249.000000000,183.000000000,-29.000000000));
          M.Types.Add(TEXT("Rostov"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 2"); M.bLoadout=false; M.Location=FVector(183.000000000,29.000000000,-159.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(249.000000000,-183.000000000,-29.000000000));
          M.Types.Add(TEXT("Rostov-Starboard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,68.000000000,153.000000000);
          M.Muzzle=FVector(0.000000000,68.000000000,153.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(-91.000000000,-47.000000000,313.000000000);
          M.Muzzle=FVector(-91.000000000,-47.000000000,313.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(91.000000000,-47.000000000,313.000000000);
          M.Muzzle=FVector(91.000000000,-47.000000000,313.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,92.000000000,-228.000000000);
          M.Muzzle=FVector(0.000000000,92.000000000,-228.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(-139.000000000,72.000000000,-172.000000000);
          M.Muzzle=FVector(-139.000000000,72.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(139.000000000,72.000000000,-172.000000000);
          M.Muzzle=FVector(139.000000000,72.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(139.000000000,-12.000000000,-172.000000000);
          M.Muzzle=FVector(139.000000000,-12.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(-139.000000000,-12.000000000,-172.000000000);
          M.Muzzle=FVector(-139.000000000,-12.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Hulk-Tiger")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(74.000000000,-12.000000000,110.000000000);
          M.Muzzle=FVector(74.000000000,-12.000000000,240.000000000);
          M.Muzzles.Add(FVector(74.000000000,-12.000000000,240.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(-74.000000000,-12.000000000,110.000000000);
          M.Muzzle=FVector(-74.000000000,-12.000000000,240.000000000);
          M.Muzzles.Add(FVector(-74.000000000,-12.000000000,240.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-20.000000000,0.000000000,300.000000000);
          M.Muzzle=FVector(-40.000000000,0.000000000,390.000000000);
          M.Muzzles.Add(FVector(-40.000000000,0.000000000,390.000000000));
          M.Muzzles.Add(FVector(40.000000000,0.000000000,390.000000000));
          M.BarrelOffsets.Add(FVector(90.000000000,-20.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(90.000000000,60.000000000,0.000000000));
          M.Types.Add(TEXT("Mk7 Interceptor"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(0.000000000,75.000000000,220.000000000);
          M.Muzzle=FVector(0.000000000,10.000000000,131.000000000);
          M.Muzzles.Add(FVector(0.000000000,10.000000000,131.000000000));
          M.BarrelOffsets.Add(FVector(72.777777778,0.000000000,5.555555556));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(0.000000000,135.000000000,-420.000000000);
          M.Muzzle=FVector(0.000000000,10.000000000,131.000000000);
          M.Muzzles.Add(FVector(0.000000000,10.000000000,131.000000000));
          M.BarrelOffsets.Add(FVector(72.777777778,0.000000000,5.555555556));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Hulk-Vendetta")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-8.000000000,33.000000000,600.000000000);
          M.Muzzle=FVector(-8.000000000,33.000000000,779.000000000);
          M.Muzzles.Add(FVector(-8.000000000,33.000000000,779.000000000));
          M.BarrelOffsets.Add(FVector(179.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(8.000000000,33.000000000,600.000000000);
          M.Muzzle=FVector(8.000000000,33.000000000,779.000000000);
          M.Muzzles.Add(FVector(8.000000000,33.000000000,779.000000000));
          M.BarrelOffsets.Add(FVector(179.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 1"); M.bLoadout=false; M.Location=FVector(-282.000000000,40.000000000,-92.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(182.000000000,282.000000000,-40.000000000));
          M.Types.Add(TEXT("Rostov"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 2"); M.bLoadout=false; M.Location=FVector(282.000000000,40.000000000,-92.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(182.000000000,-282.000000000,-40.000000000));
          M.Types.Add(TEXT("Rostov-Starboard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,90.000000000,147.000000000);
          M.Muzzle=FVector(0.000000000,12.000000000,81.000000000);
          M.Muzzles.Add(FVector(0.000000000,12.000000000,81.000000000));
          M.BarrelOffsets.Add(FVector(77.142857143,0.000000000,11.428571429));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,105.000000000,-548.000000000);
          M.Muzzle=FVector(0.000000000,12.000000000,81.000000000);
          M.Muzzles.Add(FVector(0.000000000,12.000000000,81.000000000));
          M.BarrelOffsets.Add(FVector(77.142857143,0.000000000,11.428571429));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 1"); M.bLoadout=false; M.Location=FVector(0.000000000,73.000000000,422.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 2"); M.bLoadout=false; M.Location=FVector(0.000000000,-143.000000000,506.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 3"); M.bLoadout=false; M.Location=FVector(0.000000000,-104.000000000,-360.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 4"); M.bLoadout=false; M.Location=FVector(209.000000000,88.000000000,-96.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 5"); M.bLoadout=false; M.Location=FVector(-209.000000000,88.000000000,-96.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Hulk-Volnaris")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-34.000000000,-19.000000000,460.000000000);
          M.Muzzle=FVector(-34.000000000,-19.000000000,570.000000000);
          M.Muzzles.Add(FVector(-34.000000000,-19.000000000,570.000000000));
          M.BarrelOffsets.Add(FVector(110.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(34.000000000,-19.000000000,460.000000000);
          M.Muzzle=FVector(34.000000000,-19.000000000,570.000000000);
          M.Muzzles.Add(FVector(34.000000000,-19.000000000,570.000000000));
          M.BarrelOffsets.Add(FVector(110.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Torpedo Banks"); M.bLoadout=false; M.Location=FVector(0.000000000,-32.000000000,400.000000000);
          M.Muzzle=FVector(-25.000000000,17.000000000,470.000000000);
          M.Muzzles.Add(FVector(-25.000000000,17.000000000,470.000000000));
          M.Muzzles.Add(FVector(25.000000000,17.000000000,470.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,-25.000000000,49.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,25.000000000,49.000000000));
          M.Types.Add(TEXT("Fusion Torpedo"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 1"); M.bLoadout=false; M.Location=FVector(0.000000000,45.000000000,340.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 2"); M.bLoadout=false; M.Location=FVector(0.000000000,55.000000000,225.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 3"); M.bLoadout=false; M.Location=FVector(0.000000000,55.000000000,-255.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("CargoC")))
    {
        return;
    }
    if (ShipDef == FName(TEXT("CargoA")))
    {
        return;
    }
    if (ShipDef == FName(TEXT("CargoB")))
    {
        return;
    }
    if (ShipDef == FName(TEXT("Imperial")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-42.000000000,-9.000000000,620.000000000);
          M.Muzzle=FVector(-42.000000000,-9.000000000,640.000000000);
          M.Muzzles.Add(FVector(-42.000000000,-9.000000000,640.000000000));
          M.BarrelOffsets.Add(FVector(20.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(42.000000000,-9.000000000,620.000000000);
          M.Muzzle=FVector(42.000000000,-9.000000000,640.000000000);
          M.Muzzles.Add(FVector(42.000000000,-9.000000000,640.000000000));
          M.BarrelOffsets.Add(FVector(20.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 1"); M.bLoadout=false; M.Location=FVector(-183.000000000,29.000000000,-159.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(249.000000000,183.000000000,-29.000000000));
          M.Types.Add(TEXT("Rostov"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 2"); M.bLoadout=false; M.Location=FVector(183.000000000,29.000000000,-159.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(249.000000000,-183.000000000,-29.000000000));
          M.Types.Add(TEXT("Rostov-Starboard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,68.000000000,153.000000000);
          M.Muzzle=FVector(0.000000000,68.000000000,153.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(-91.000000000,-47.000000000,313.000000000);
          M.Muzzle=FVector(-91.000000000,-47.000000000,313.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(91.000000000,-47.000000000,313.000000000);
          M.Muzzle=FVector(91.000000000,-47.000000000,313.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,92.000000000,-228.000000000);
          M.Muzzle=FVector(0.000000000,92.000000000,-228.000000000);
          M.BarrelOffsets.Add(FVector(81.000000000,0.000000000,12.000000000));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(-139.000000000,72.000000000,-172.000000000);
          M.Muzzle=FVector(-139.000000000,72.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(139.000000000,72.000000000,-172.000000000);
          M.Muzzle=FVector(139.000000000,72.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(139.000000000,-12.000000000,-172.000000000);
          M.Muzzle=FVector(139.000000000,-12.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("RAIL12"); M.bLoadout=false; M.Location=FVector(-139.000000000,-12.000000000,-172.000000000);
          M.Muzzle=FVector(-139.000000000,-12.000000000,-172.000000000);
          M.BarrelOffsets.Add(FVector(56.250000000,3.000000000,8.250000000));
          M.BarrelOffsets.Add(FVector(56.250000000,-3.000000000,8.250000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Intruder")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,120.000000000);
          M.Muzzle=FVector(-24.000000000,-6.000000000,145.000000000);
          M.Muzzles.Add(FVector(-24.000000000,-6.000000000,145.000000000));
          M.Muzzles.Add(FVector(24.000000000,-6.000000000,145.000000000));
          M.BarrelOffsets.Add(FVector(25.000000000,-24.000000000,-6.000000000));
          M.BarrelOffsets.Add(FVector(25.000000000,24.000000000,-6.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-93.000000000,-27.000000000,-100.000000000);
          M.Muzzle=FVector(-93.000000000,-27.000000000,-125.000000000);
          M.Muzzles.Add(FVector(-93.000000000,-27.000000000,-125.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-62.000000000,-25.000000000,-100.000000000);
          M.Muzzle=FVector(-62.000000000,-25.000000000,-120.000000000);
          M.Muzzles.Add(FVector(-62.000000000,-25.000000000,-120.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(62.000000000,-25.000000000,-100.000000000);
          M.Muzzle=FVector(62.000000000,-25.000000000,-120.000000000);
          M.Muzzles.Add(FVector(62.000000000,-25.000000000,-120.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(93.000000000,-27.000000000,-100.000000000);
          M.Muzzle=FVector(93.000000000,-27.000000000,-125.000000000);
          M.Muzzles.Add(FVector(93.000000000,-27.000000000,-125.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Jackal")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-8.000000000,-26.000000000,160.000000000);
          M.Muzzle=FVector(-8.000000000,-26.000000000,182.000000000);
          M.Muzzles.Add(FVector(-8.000000000,-26.000000000,182.000000000));
          M.BarrelOffsets.Add(FVector(22.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Vulcan Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Cannon"); M.bLoadout=false; M.Location=FVector(8.000000000,-26.000000000,160.000000000);
          M.Muzzle=FVector(8.000000000,-26.000000000,182.000000000);
          M.Muzzles.Add(FVector(8.000000000,-26.000000000,182.000000000));
          M.BarrelOffsets.Add(FVector(22.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Vulcan Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Marauder")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-260.000000000,-32.000000000,60.000000000);
          M.Muzzle=FVector(-260.000000000,-32.000000000,194.000000000);
          M.Muzzles.Add(FVector(-260.000000000,-32.000000000,194.000000000));
          M.BarrelOffsets.Add(FVector(134.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Plasma Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Cannon"); M.bLoadout=false; M.Location=FVector(260.000000000,-32.000000000,60.000000000);
          M.Muzzle=FVector(260.000000000,-32.000000000,194.000000000);
          M.Muzzles.Add(FVector(260.000000000,-32.000000000,194.000000000));
          M.BarrelOffsets.Add(FVector(134.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Plasma Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(-110.000000000,26.000000000,20.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(110.000000000,26.000000000,20.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Mine1")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Fwd Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,0.000000000);
          M.Muzzle=FVector(105.000000000,8.000000000,75.000000000);
          M.Muzzles.Add(FVector(105.000000000,8.000000000,75.000000000));
          M.Muzzles.Add(FVector(-105.000000000,8.000000000,75.000000000));
          M.Muzzles.Add(FVector(105.000000000,-8.000000000,75.000000000));
          M.Muzzles.Add(FVector(-105.000000000,-8.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,105.000000000,8.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-105.000000000,8.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,105.000000000,-8.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-105.000000000,-8.000000000));
          M.Types.Add(TEXT("Blockade Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Observatory")))
    {
        return;
    }
    if (ShipDef == FName(TEXT("Orion")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Vanguard 1"); M.bLoadout=false; M.Location=FVector(128.000000000,95.000000000,704.000000000);
          M.Muzzle=FVector(128.000000000,95.000000000,704.000000000);
          M.BarrelOffsets.Add(FVector(18.000000000,2.100000000,0.000000000));
          M.BarrelOffsets.Add(FVector(18.000000000,-2.100000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Vanguard 2"); M.bLoadout=false; M.Location=FVector(-128.000000000,95.000000000,704.000000000);
          M.Muzzle=FVector(-128.000000000,95.000000000,704.000000000);
          M.BarrelOffsets.Add(FVector(18.000000000,2.100000000,0.000000000));
          M.BarrelOffsets.Add(FVector(18.000000000,-2.100000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Vanguard 3"); M.bLoadout=false; M.Location=FVector(128.000000000,95.000000000,228.000000000);
          M.Muzzle=FVector(128.000000000,95.000000000,228.000000000);
          M.BarrelOffsets.Add(FVector(18.000000000,2.100000000,0.000000000));
          M.BarrelOffsets.Add(FVector(18.000000000,-2.100000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Vanguard 4"); M.bLoadout=false; M.Location=FVector(-128.000000000,95.000000000,228.000000000);
          M.Muzzle=FVector(-128.000000000,95.000000000,228.000000000);
          M.BarrelOffsets.Add(FVector(18.000000000,2.100000000,0.000000000));
          M.BarrelOffsets.Add(FVector(18.000000000,-2.100000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Vanguard 5"); M.bLoadout=false; M.Location=FVector(64.000000000,103.000000000,-830.000000000);
          M.Muzzle=FVector(64.000000000,103.000000000,-830.000000000);
          M.BarrelOffsets.Add(FVector(18.000000000,2.100000000,0.000000000));
          M.BarrelOffsets.Add(FVector(18.000000000,-2.100000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Vanguard 6"); M.bLoadout=false; M.Location=FVector(-64.000000000,103.000000000,-830.000000000);
          M.Muzzle=FVector(-64.000000000,103.000000000,-830.000000000);
          M.BarrelOffsets.Add(FVector(18.000000000,2.100000000,0.000000000));
          M.BarrelOffsets.Add(FVector(18.000000000,-2.100000000,0.000000000));
          M.Types.Add(TEXT("Vanguard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Phalanx"); M.bLoadout=false; M.Location=FVector(0.000000000,70.000000000,960.000000000);
          M.Muzzle=FVector(0.000000000,70.000000000,960.000000000);
          M.BarrelOffsets.Add(FVector(8.250000000,1.200000000,0.000000000));
          M.BarrelOffsets.Add(FVector(8.250000000,-1.200000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Phalanx"); M.bLoadout=false; M.Location=FVector(0.000000000,-75.000000000,960.000000000);
          M.Muzzle=FVector(0.000000000,-75.000000000,960.000000000);
          M.BarrelOffsets.Add(FVector(8.250000000,1.200000000,0.000000000));
          M.BarrelOffsets.Add(FVector(8.250000000,-1.200000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Phalanx"); M.bLoadout=false; M.Location=FVector(64.000000000,-132.000000000,-830.000000000);
          M.Muzzle=FVector(64.000000000,-132.000000000,-830.000000000);
          M.BarrelOffsets.Add(FVector(8.250000000,1.200000000,0.000000000));
          M.BarrelOffsets.Add(FVector(8.250000000,-1.200000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Phalanx"); M.bLoadout=false; M.Location=FVector(-64.000000000,-132.000000000,-830.000000000);
          M.Muzzle=FVector(-64.000000000,-132.000000000,-830.000000000);
          M.BarrelOffsets.Add(FVector(8.250000000,1.200000000,0.000000000));
          M.BarrelOffsets.Add(FVector(8.250000000,-1.200000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("QShip")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-30.000000000,0.000000000,360.000000000);
          M.Muzzle=FVector(-30.000000000,0.000000000,400.000000000);
          M.Muzzles.Add(FVector(-30.000000000,0.000000000,400.000000000));
          M.BarrelOffsets.Add(FVector(40.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Plasma Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Cannon"); M.bLoadout=false; M.Location=FVector(30.000000000,0.000000000,360.000000000);
          M.Muzzle=FVector(30.000000000,0.000000000,400.000000000);
          M.Muzzles.Add(FVector(30.000000000,0.000000000,400.000000000));
          M.BarrelOffsets.Add(FVector(40.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Plasma Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(-58.000000000,64.000000000,0.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(58.000000000,64.000000000,0.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-3"); M.bLoadout=false; M.Location=FVector(-58.000000000,-64.000000000,0.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-4"); M.bLoadout=false; M.Location=FVector(58.000000000,-64.000000000,0.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,71.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,71.000000000));
          M.BarrelOffsets.Add(FVector(42.600000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Light-PDB"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Raptor")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,120.000000000);
          M.Muzzle=FVector(-17.000000000,-19.000000000,170.000000000);
          M.Muzzles.Add(FVector(-17.000000000,-19.000000000,170.000000000));
          M.Muzzles.Add(FVector(17.000000000,-19.000000000,170.000000000));
          M.BarrelOffsets.Add(FVector(50.000000000,-17.000000000,-19.000000000));
          M.BarrelOffsets.Add(FVector(50.000000000,17.000000000,-19.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-120.000000000,-49.000000000,-100.000000000);
          M.Muzzle=FVector(-120.000000000,-49.000000000,-140.000000000);
          M.Muzzles.Add(FVector(-120.000000000,-49.000000000,-140.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-99.000000000,-47.000000000,-100.000000000);
          M.Muzzle=FVector(-99.000000000,-47.000000000,-130.000000000);
          M.Muzzles.Add(FVector(-99.000000000,-47.000000000,-130.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-79.000000000,-44.000000000,-100.000000000);
          M.Muzzle=FVector(-79.000000000,-44.000000000,-120.000000000);
          M.Muzzles.Add(FVector(-79.000000000,-44.000000000,-120.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-60.000000000,-41.000000000,-100.000000000);
          M.Muzzle=FVector(-60.000000000,-41.000000000,-110.000000000);
          M.Muzzles.Add(FVector(-60.000000000,-41.000000000,-110.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(60.000000000,-41.000000000,-100.000000000);
          M.Muzzle=FVector(60.000000000,-41.000000000,-110.000000000);
          M.Muzzles.Add(FVector(60.000000000,-41.000000000,-110.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(79.000000000,-44.000000000,-100.000000000);
          M.Muzzle=FVector(79.000000000,-44.000000000,-120.000000000);
          M.Muzzles.Add(FVector(79.000000000,-44.000000000,-120.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(99.000000000,-45.000000000,-100.000000000);
          M.Muzzle=FVector(99.000000000,-45.000000000,-130.000000000);
          M.Muzzles.Add(FVector(99.000000000,-45.000000000,-130.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(120.000000000,-49.000000000,-100.000000000);
          M.Muzzle=FVector(120.000000000,-49.000000000,-140.000000000);
          M.Muzzles.Add(FVector(120.000000000,-49.000000000,-140.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Razor")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,32.000000000);
          M.Muzzle=FVector(-7.000000000,9.000000000,190.000000000);
          M.Muzzles.Add(FVector(-7.000000000,9.000000000,190.000000000));
          M.Muzzles.Add(FVector(7.000000000,9.000000000,190.000000000));
          M.BarrelOffsets.Add(FVector(158.000000000,-7.000000000,9.000000000));
          M.BarrelOffsets.Add(FVector(158.000000000,7.000000000,9.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(126.000000000,-29.000000000,0.000000000);
          M.Muzzle=FVector(126.000000000,-29.000000000,-50.000000000);
          M.Muzzles.Add(FVector(126.000000000,-29.000000000,-50.000000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(102.000000000,-26.000000000,0.000000000);
          M.Muzzle=FVector(102.000000000,-26.000000000,-50.000000000);
          M.Muzzles.Add(FVector(102.000000000,-26.000000000,-50.000000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(62.000000000,-21.000000000,0.000000000);
          M.Muzzle=FVector(62.000000000,-21.000000000,-8.000000000);
          M.Muzzles.Add(FVector(62.000000000,-21.000000000,-8.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(36.000000000,-19.000000000,0.000000000);
          M.Muzzle=FVector(36.000000000,-19.000000000,-8.000000000);
          M.Muzzles.Add(FVector(36.000000000,-19.000000000,-8.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-36.000000000,-19.000000000,0.000000000);
          M.Muzzle=FVector(-36.000000000,-19.000000000,-8.000000000);
          M.Muzzles.Add(FVector(-36.000000000,-19.000000000,-8.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-62.000000000,-21.000000000,0.000000000);
          M.Muzzle=FVector(-62.000000000,-21.000000000,-8.000000000);
          M.Muzzles.Add(FVector(-62.000000000,-21.000000000,-8.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(-102.000000000,-26.000000000,0.000000000);
          M.Muzzle=FVector(-102.000000000,-26.000000000,-50.000000000);
          M.Muzzles.Add(FVector(-102.000000000,-26.000000000,-50.000000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(-126.000000000,-29.000000000,0.000000000);
          M.Muzzle=FVector(-126.000000000,-29.000000000,-50.000000000);
          M.Muzzles.Add(FVector(-126.000000000,-29.000000000,-50.000000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("S5")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 1A"); M.bLoadout=false; M.Location=FVector(1150.000000000,488.000000000,0.000000000);
          M.Muzzle=FVector(1150.000000000,488.000000000,0.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,4.125000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 1B"); M.bLoadout=false; M.Location=FVector(1150.000000000,320.000000000,0.000000000);
          M.Muzzle=FVector(1150.000000000,320.000000000,0.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,-4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,-4.125000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 2A"); M.bLoadout=false; M.Location=FVector(0.000000000,488.000000000,-1150.000000000);
          M.Muzzle=FVector(0.000000000,488.000000000,-1150.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,4.125000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 2B"); M.bLoadout=false; M.Location=FVector(0.000000000,320.000000000,-1150.000000000);
          M.Muzzle=FVector(0.000000000,320.000000000,-1150.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,-4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,-4.125000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 3A"); M.bLoadout=false; M.Location=FVector(-1150.000000000,488.000000000,0.000000000);
          M.Muzzle=FVector(-1150.000000000,488.000000000,0.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,4.125000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 3B"); M.bLoadout=false; M.Location=FVector(-1150.000000000,320.000000000,0.000000000);
          M.Muzzle=FVector(-1150.000000000,320.000000000,0.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,-4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,-4.125000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 4A"); M.bLoadout=false; M.Location=FVector(0.000000000,488.000000000,1150.000000000);
          M.Muzzle=FVector(0.000000000,488.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,4.125000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 4B"); M.bLoadout=false; M.Location=FVector(0.000000000,320.000000000,1150.000000000);
          M.Muzzle=FVector(0.000000000,320.000000000,1150.000000000);
          M.BarrelOffsets.Add(FVector(28.125000000,1.500000000,-4.125000000));
          M.BarrelOffsets.Add(FVector(28.125000000,-1.500000000,-4.125000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("S6")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 1"); M.bLoadout=false; M.Location=FVector(0.000000000,118.000000000,920.000000000);
          M.Muzzle=FVector(0.000000000,118.000000000,920.000000000);
          M.BarrelOffsets.Add(FVector(36.111111111,2.222222222,0.000000000));
          M.BarrelOffsets.Add(FVector(36.111111111,-2.222222222,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 2"); M.bLoadout=false; M.Location=FVector(0.000000000,-118.000000000,920.000000000);
          M.Muzzle=FVector(0.000000000,-118.000000000,920.000000000);
          M.BarrelOffsets.Add(FVector(36.111111111,2.222222222,0.000000000));
          M.BarrelOffsets.Add(FVector(36.111111111,-2.222222222,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 3"); M.bLoadout=false; M.Location=FVector(0.000000000,118.000000000,-920.000000000);
          M.Muzzle=FVector(0.000000000,118.000000000,-920.000000000);
          M.BarrelOffsets.Add(FVector(36.111111111,2.222222222,0.000000000));
          M.BarrelOffsets.Add(FVector(36.111111111,-2.222222222,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 4"); M.bLoadout=false; M.Location=FVector(0.000000000,-118.000000000,-920.000000000);
          M.Muzzle=FVector(0.000000000,-118.000000000,-920.000000000);
          M.BarrelOffsets.Add(FVector(36.111111111,2.222222222,0.000000000));
          M.BarrelOffsets.Add(FVector(36.111111111,-2.222222222,0.000000000));
          M.Types.Add(TEXT("Viking Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("S7")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Graser 1"); M.bLoadout=false; M.Location=FVector(240.000000000,50.000000000,100.000000000);
          M.Muzzle=FVector(240.000000000,50.000000000,100.000000000);
          M.BarrelOffsets.Add(FVector(12.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Graser Turret"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Graser 2"); M.bLoadout=false; M.Location=FVector(-240.000000000,50.000000000,100.000000000);
          M.Muzzle=FVector(-240.000000000,50.000000000,100.000000000);
          M.BarrelOffsets.Add(FVector(12.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Graser Turret"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Graser 3"); M.bLoadout=false; M.Location=FVector(240.000000000,50.000000000,-100.000000000);
          M.Muzzle=FVector(240.000000000,50.000000000,-100.000000000);
          M.BarrelOffsets.Add(FVector(12.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Graser Turret"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Graser 4"); M.bLoadout=false; M.Location=FVector(-240.000000000,50.000000000,-100.000000000);
          M.Muzzle=FVector(-240.000000000,50.000000000,-100.000000000);
          M.BarrelOffsets.Add(FVector(12.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Graser Turret"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 1A"); M.bLoadout=false; M.Location=FVector(105.000000000,36.000000000,704.000000000);
          M.Muzzle=FVector(105.000000000,36.000000000,704.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,2.200000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 2A"); M.bLoadout=false; M.Location=FVector(105.000000000,36.000000000,-640.000000000);
          M.Muzzle=FVector(105.000000000,36.000000000,-640.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,2.200000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 3A"); M.bLoadout=false; M.Location=FVector(-105.000000000,36.000000000,704.000000000);
          M.Muzzle=FVector(-105.000000000,36.000000000,704.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,2.200000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 4A"); M.bLoadout=false; M.Location=FVector(-105.000000000,36.000000000,-640.000000000);
          M.Muzzle=FVector(-105.000000000,36.000000000,-640.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,2.200000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 1B"); M.bLoadout=false; M.Location=FVector(105.000000000,-24.000000000,704.000000000);
          M.Muzzle=FVector(105.000000000,-24.000000000,704.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,-2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,-2.200000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 2B"); M.bLoadout=false; M.Location=FVector(105.000000000,-24.000000000,-640.000000000);
          M.Muzzle=FVector(105.000000000,-24.000000000,-640.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,-2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,-2.200000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 3B"); M.bLoadout=false; M.Location=FVector(-105.000000000,-24.000000000,704.000000000);
          M.Muzzle=FVector(-105.000000000,-24.000000000,704.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,-2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,-2.200000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret 4B"); M.bLoadout=false; M.Location=FVector(-105.000000000,-24.000000000,-640.000000000);
          M.Muzzle=FVector(-105.000000000,-24.000000000,-640.000000000);
          M.BarrelOffsets.Add(FVector(15.000000000,0.800000000,-2.200000000));
          M.BarrelOffsets.Add(FVector(15.000000000,-0.800000000,-2.200000000));
          M.Types.Add(TEXT("Rail-12 Inverted"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Saber")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,32.000000000);
          M.Muzzle=FVector(-190.000000000,15.000000000,55.000000000);
          M.Muzzles.Add(FVector(-190.000000000,15.000000000,55.000000000));
          M.Muzzles.Add(FVector(190.000000000,15.000000000,55.000000000));
          M.BarrelOffsets.Add(FVector(23.000000000,-190.000000000,15.000000000));
          M.BarrelOffsets.Add(FVector(23.000000000,190.000000000,15.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-101.000000000,11.000000000,-10.000000000);
          M.Muzzle=FVector(-101.000000000,11.000000000,-85.000000000);
          M.Muzzles.Add(FVector(-101.000000000,11.000000000,-85.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-74.000000000,1.000000000,-10.000000000);
          M.Muzzle=FVector(-74.000000000,1.000000000,-75.000000000);
          M.Muzzles.Add(FVector(-74.000000000,1.000000000,-75.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(74.000000000,1.000000000,-10.000000000);
          M.Muzzle=FVector(74.000000000,1.000000000,-75.000000000);
          M.Muzzles.Add(FVector(74.000000000,1.000000000,-75.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(101.000000000,11.000000000,-10.000000000);
          M.Muzzle=FVector(101.000000000,11.000000000,-85.000000000);
          M.Muzzles.Add(FVector(101.000000000,11.000000000,-85.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Spectre")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-36.000000000,-9.000000000,400.000000000);
          M.Muzzle=FVector(-36.000000000,-9.000000000,467.000000000);
          M.Muzzles.Add(FVector(-36.000000000,-9.000000000,467.000000000));
          M.BarrelOffsets.Add(FVector(67.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(36.000000000,-9.000000000,400.000000000);
          M.Muzzle=FVector(36.000000000,-9.000000000,467.000000000);
          M.Muzzles.Add(FVector(36.000000000,-9.000000000,467.000000000));
          M.BarrelOffsets.Add(FVector(67.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-14.000000000,-23.000000000,400.000000000);
          M.Muzzle=FVector(-14.000000000,-23.000000000,467.000000000);
          M.Muzzles.Add(FVector(-14.000000000,-23.000000000,467.000000000));
          M.BarrelOffsets.Add(FVector(67.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(14.000000000,-23.000000000,400.000000000);
          M.Muzzle=FVector(14.000000000,-23.000000000,467.000000000);
          M.Muzzles.Add(FVector(14.000000000,-23.000000000,467.000000000));
          M.BarrelOffsets.Add(FVector(67.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Starboard Torpedo"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,0.000000000);
          M.Muzzle=FVector(192.000000000,-32.000000000,-120.000000000);
          M.Muzzles.Add(FVector(192.000000000,-32.000000000,-120.000000000));
          M.BarrelOffsets.Add(FVector(-120.000000000,192.000000000,-32.000000000));
          M.Types.Add(TEXT("Fusion Torpedo"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Torpedo"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,0.000000000);
          M.Muzzle=FVector(-192.000000000,-32.000000000,-120.000000000);
          M.Muzzles.Add(FVector(-192.000000000,-32.000000000,-120.000000000));
          M.BarrelOffsets.Add(FVector(-120.000000000,-192.000000000,-32.000000000));
          M.Types.Add(TEXT("Fusion Torpedo"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port/Fwd Phalanx"); M.bLoadout=false; M.Location=FVector(-211.000000000,-50.000000000,-106.000000000);
          M.Muzzle=FVector(-211.000000000,-50.000000000,-106.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Stbd/Fwd Phalanx"); M.bLoadout=false; M.Location=FVector(211.000000000,-50.000000000,-106.000000000);
          M.Muzzle=FVector(211.000000000,-50.000000000,-106.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port/Aft Phalanx"); M.bLoadout=false; M.Location=FVector(-211.000000000,-50.000000000,-255.000000000);
          M.Muzzle=FVector(-211.000000000,-50.000000000,-255.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Stbd/Aft Phalanx"); M.bLoadout=false; M.Location=FVector(211.000000000,-50.000000000,-255.000000000);
          M.Muzzle=FVector(211.000000000,-50.000000000,-255.000000000);
          M.BarrelOffsets.Add(FVector(13.200000000,1.920000000,0.000000000));
          M.BarrelOffsets.Add(FVector(13.200000000,-1.920000000,0.000000000));
          M.Types.Add(TEXT("Phalanx Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Stormhawk")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,32.000000000);
          M.Muzzle=FVector(-28.000000000,2.000000000,77.000000000);
          M.Muzzles.Add(FVector(-28.000000000,2.000000000,77.000000000));
          M.Muzzles.Add(FVector(28.000000000,2.000000000,77.000000000));
          M.BarrelOffsets.Add(FVector(45.000000000,-28.000000000,2.000000000));
          M.BarrelOffsets.Add(FVector(45.000000000,28.000000000,2.000000000));
          M.Types.Add(TEXT("Delta Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-78.000000000,-10.000000000,-120.000000000);
          M.Muzzle=FVector(-78.000000000,-10.000000000,-120.000000000);
          M.Muzzles.Add(FVector(-78.000000000,-10.000000000,-120.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-50.000000000,-5.000000000,-110.000000000);
          M.Muzzle=FVector(-50.000000000,-5.000000000,-110.000000000);
          M.Muzzles.Add(FVector(-50.000000000,-5.000000000,-110.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-16.000000000,-18.000000000,-110.000000000);
          M.Muzzle=FVector(-16.000000000,-18.000000000,-110.000000000);
          M.Muzzles.Add(FVector(-16.000000000,-18.000000000,-110.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(16.000000000,-18.000000000,-110.000000000);
          M.Muzzle=FVector(16.000000000,-18.000000000,-110.000000000);
          M.Muzzles.Add(FVector(16.000000000,-18.000000000,-110.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          M.Types.Add(TEXT("Recon Pod"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(50.000000000,-5.000000000,-110.000000000);
          M.Muzzle=FVector(50.000000000,-5.000000000,-110.000000000);
          M.Muzzles.Add(FVector(50.000000000,-5.000000000,-110.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(78.000000000,-10.000000000,-120.000000000);
          M.Muzzle=FVector(78.000000000,-10.000000000,-120.000000000);
          M.Muzzles.Add(FVector(78.000000000,-10.000000000,-120.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Thunderbolt")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,160.000000000);
          M.Muzzle=FVector(-22.000000000,-1.000000000,200.000000000);
          M.Muzzles.Add(FVector(-22.000000000,-1.000000000,200.000000000));
          M.Muzzles.Add(FVector(21.000000000,-1.000000000,200.000000000));
          M.BarrelOffsets.Add(FVector(40.000000000,-22.000000000,-1.000000000));
          M.BarrelOffsets.Add(FVector(40.000000000,21.000000000,-1.000000000));
          M.Types.Add(TEXT("Heavy Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,-195.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,24.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,24.000000000));
          M.BarrelOffsets.Add(FVector(19.555555556,0.000000000,0.000000000));
          M.Types.Add(TEXT("Fighter Turret"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-116.000000000,-25.000000000,-70.000000000);
          M.Muzzle=FVector(-116.000000000,-25.000000000,-82.000000000);
          M.Muzzles.Add(FVector(-116.000000000,-25.000000000,-82.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-60.000000000,-16.000000000,-60.000000000);
          M.Muzzle=FVector(-60.000000000,-16.000000000,-72.000000000);
          M.Muzzles.Add(FVector(-60.000000000,-16.000000000,-72.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-40.000000000,-16.000000000,-60.000000000);
          M.Muzzle=FVector(-40.000000000,-16.000000000,-72.000000000);
          M.Muzzles.Add(FVector(-40.000000000,-16.000000000,-72.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(40.000000000,-16.000000000,-60.000000000);
          M.Muzzle=FVector(40.000000000,-16.000000000,-72.000000000);
          M.Muzzles.Add(FVector(40.000000000,-16.000000000,-72.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(60.000000000,-16.000000000,-60.000000000);
          M.Muzzle=FVector(60.000000000,-16.000000000,-72.000000000);
          M.Muzzles.Add(FVector(60.000000000,-16.000000000,-72.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(116.000000000,-25.000000000,-70.000000000);
          M.Muzzle=FVector(116.000000000,-25.000000000,-82.000000000);
          M.Muzzles.Add(FVector(116.000000000,-25.000000000,-82.000000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Tiger")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(74.000000000,-12.000000000,110.000000000);
          M.Muzzle=FVector(74.000000000,-12.000000000,240.000000000);
          M.Muzzles.Add(FVector(74.000000000,-12.000000000,240.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(-74.000000000,-12.000000000,110.000000000);
          M.Muzzle=FVector(-74.000000000,-12.000000000,240.000000000);
          M.Muzzles.Add(FVector(-74.000000000,-12.000000000,240.000000000));
          M.BarrelOffsets.Add(FVector(130.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-20.000000000,0.000000000,300.000000000);
          M.Muzzle=FVector(-40.000000000,0.000000000,390.000000000);
          M.Muzzles.Add(FVector(-40.000000000,0.000000000,390.000000000));
          M.Muzzles.Add(FVector(40.000000000,0.000000000,390.000000000));
          M.BarrelOffsets.Add(FVector(90.000000000,-20.000000000,0.000000000));
          M.BarrelOffsets.Add(FVector(90.000000000,60.000000000,0.000000000));
          M.Types.Add(TEXT("Mk7 Interceptor"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-1"); M.bLoadout=false; M.Location=FVector(0.000000000,75.000000000,220.000000000);
          M.Muzzle=FVector(0.000000000,10.000000000,131.000000000);
          M.Muzzles.Add(FVector(0.000000000,10.000000000,131.000000000));
          M.BarrelOffsets.Add(FVector(72.777777778,0.000000000,5.555555556));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB-2"); M.bLoadout=false; M.Location=FVector(0.000000000,135.000000000,-420.000000000);
          M.Muzzle=FVector(0.000000000,10.000000000,131.000000000);
          M.Muzzles.Add(FVector(0.000000000,10.000000000,131.000000000));
          M.BarrelOffsets.Add(FVector(72.777777778,0.000000000,5.555555556));
          M.Types.Add(TEXT("Helicon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Vendetta")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-8.000000000,33.000000000,600.000000000);
          M.Muzzle=FVector(-8.000000000,33.000000000,779.000000000);
          M.Muzzles.Add(FVector(-8.000000000,33.000000000,779.000000000));
          M.BarrelOffsets.Add(FVector(179.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(8.000000000,33.000000000,600.000000000);
          M.Muzzle=FVector(8.000000000,33.000000000,779.000000000);
          M.Muzzles.Add(FVector(8.000000000,33.000000000,779.000000000));
          M.BarrelOffsets.Add(FVector(179.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Gamma Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 1"); M.bLoadout=false; M.Location=FVector(-282.000000000,40.000000000,-92.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(182.000000000,282.000000000,-40.000000000));
          M.Types.Add(TEXT("Rostov"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Missile 2"); M.bLoadout=false; M.Location=FVector(282.000000000,40.000000000,-92.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(182.000000000,-282.000000000,-40.000000000));
          M.Types.Add(TEXT("Rostov-Starboard"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,90.000000000,147.000000000);
          M.Muzzle=FVector(0.000000000,12.000000000,81.000000000);
          M.Muzzles.Add(FVector(0.000000000,12.000000000,81.000000000));
          M.BarrelOffsets.Add(FVector(77.142857143,0.000000000,11.428571429));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("18INMAG"); M.bLoadout=false; M.Location=FVector(0.000000000,105.000000000,-548.000000000);
          M.Muzzle=FVector(0.000000000,12.000000000,81.000000000);
          M.Muzzles.Add(FVector(0.000000000,12.000000000,81.000000000));
          M.BarrelOffsets.Add(FVector(77.142857143,0.000000000,11.428571429));
          M.Types.Add(TEXT("Mag-18"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 1"); M.bLoadout=false; M.Location=FVector(0.000000000,73.000000000,422.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 2"); M.bLoadout=false; M.Location=FVector(0.000000000,-143.000000000,506.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 3"); M.bLoadout=false; M.Location=FVector(0.000000000,-104.000000000,-360.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 4"); M.bLoadout=false; M.Location=FVector(209.000000000,88.000000000,-96.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 5"); M.bLoadout=false; M.Location=FVector(-209.000000000,88.000000000,-96.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(53.571428571,2.857142857,7.857142857));
          M.BarrelOffsets.Add(FVector(53.571428571,-2.857142857,7.857142857));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Viper")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,120.000000000);
          M.Muzzle=FVector(-7.000000000,7.000000000,190.000000000);
          M.Muzzles.Add(FVector(-7.000000000,7.000000000,190.000000000));
          M.Muzzles.Add(FVector(7.000000000,7.000000000,190.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,-7.000000000,7.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,7.000000000,7.000000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(103.000000000,-39.000000000,-100.000000000);
          M.Muzzle=FVector(103.000000000,-39.000000000,-118.000000000);
          M.Muzzles.Add(FVector(103.000000000,-39.000000000,-118.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(83.000000000,-35.000000000,-100.000000000);
          M.Muzzle=FVector(83.000000000,-35.000000000,-118.000000000);
          M.Muzzles.Add(FVector(83.000000000,-35.000000000,-118.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(51.000000000,-25.000000000,-60.000000000);
          M.Muzzle=FVector(51.000000000,-25.000000000,-80.000000000);
          M.Muzzles.Add(FVector(51.000000000,-25.000000000,-80.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-51.000000000,-25.000000000,-60.000000000);
          M.Muzzle=FVector(-51.000000000,-25.000000000,-80.000000000);
          M.Muzzles.Add(FVector(-51.000000000,-25.000000000,-80.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-84.000000000,-33.000000000,-100.000000000);
          M.Muzzle=FVector(-84.000000000,-33.000000000,-118.000000000);
          M.Muzzles.Add(FVector(-84.000000000,-33.000000000,-118.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-103.000000000,-39.000000000,-100.000000000);
          M.Muzzle=FVector(-103.000000000,-39.000000000,-118.000000000);
          M.Muzzles.Add(FVector(-103.000000000,-39.000000000,-118.000000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Volnaris")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-34.000000000,-19.000000000,460.000000000);
          M.Muzzle=FVector(-34.000000000,-19.000000000,570.000000000);
          M.Muzzles.Add(FVector(-34.000000000,-19.000000000,570.000000000));
          M.BarrelOffsets.Add(FVector(110.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(34.000000000,-19.000000000,460.000000000);
          M.Muzzle=FVector(34.000000000,-19.000000000,570.000000000);
          M.Muzzles.Add(FVector(34.000000000,-19.000000000,570.000000000));
          M.BarrelOffsets.Add(FVector(110.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Torpedo Banks"); M.bLoadout=false; M.Location=FVector(0.000000000,-32.000000000,400.000000000);
          M.Muzzle=FVector(-25.000000000,17.000000000,470.000000000);
          M.Muzzles.Add(FVector(-25.000000000,17.000000000,470.000000000));
          M.Muzzles.Add(FVector(25.000000000,17.000000000,470.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,-25.000000000,49.000000000));
          M.BarrelOffsets.Add(FVector(70.000000000,25.000000000,49.000000000));
          M.Types.Add(TEXT("Fusion Torpedo"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 1"); M.bLoadout=false; M.Location=FVector(0.000000000,45.000000000,340.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 2"); M.bLoadout=false; M.Location=FVector(0.000000000,55.000000000,225.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("PDB 3"); M.bLoadout=false; M.Location=FVector(0.000000000,55.000000000,-255.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,4.000000000,11.000000000));
          M.BarrelOffsets.Add(FVector(75.000000000,-4.000000000,11.000000000));
          M.Types.Add(TEXT("Rail-12"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Wolf")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-25.000000000,23.000000000,800.000000000);
          M.Muzzle=FVector(-25.000000000,23.000000000,907.000000000);
          M.Muzzles.Add(FVector(-25.000000000,23.000000000,907.000000000));
          M.BarrelOffsets.Add(FVector(107.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(25.000000000,23.000000000,800.000000000);
          M.Muzzle=FVector(25.000000000,23.000000000,907.000000000);
          M.Muzzles.Add(FVector(25.000000000,23.000000000,907.000000000));
          M.BarrelOffsets.Add(FVector(107.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("XRay Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Fwd Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,82.000000000,500.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(88.235294118,4.705882353,12.941176471));
          M.BarrelOffsets.Add(FVector(88.235294118,-4.705882353,12.941176471));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Chin Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,-102.000000000,500.000000000);
          M.Muzzle=FVector(4.000000000,-11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,-11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,-11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(88.235294118,4.705882353,-12.941176471));
          M.BarrelOffsets.Add(FVector(88.235294118,-4.705882353,-12.941176471));
          M.Types.Add(TEXT("Orca Inverted"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Stbd Cannon"); M.bLoadout=false; M.Location=FVector(60.000000000,50.000000000,38.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(88.235294118,4.705882353,12.941176471));
          M.BarrelOffsets.Add(FVector(88.235294118,-4.705882353,12.941176471));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-60.000000000,50.000000000,38.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(88.235294118,4.705882353,12.941176471));
          M.BarrelOffsets.Add(FVector(88.235294118,-4.705882353,12.941176471));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Aft Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,70.000000000,-256.000000000);
          M.Muzzle=FVector(4.000000000,11.000000000,75.000000000);
          M.Muzzles.Add(FVector(4.000000000,11.000000000,75.000000000));
          M.Muzzles.Add(FVector(-4.000000000,11.000000000,75.000000000));
          M.BarrelOffsets.Add(FVector(88.235294118,4.705882353,12.941176471));
          M.BarrelOffsets.Add(FVector(88.235294118,-4.705882353,12.941176471));
          M.Types.Add(TEXT("Orca Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Zodiac")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Main Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,-10.000000000,110.000000000);
          M.Muzzle=FVector(-38.000000000,-21.000000000,191.000000000);
          M.Muzzles.Add(FVector(-38.000000000,-21.000000000,191.000000000));
          M.Muzzles.Add(FVector(-30.000000000,-22.000000000,203.000000000));
          M.Muzzles.Add(FVector(30.000000000,-22.000000000,203.000000000));
          M.Muzzles.Add(FVector(38.000000000,-21.000000000,191.000000000));
          M.BarrelOffsets.Add(FVector(81.000000000,-38.000000000,-11.000000000));
          M.BarrelOffsets.Add(FVector(93.000000000,-30.000000000,-12.000000000));
          M.BarrelOffsets.Add(FVector(93.000000000,30.000000000,-12.000000000));
          M.BarrelOffsets.Add(FVector(81.000000000,38.000000000,-11.000000000));
          M.Types.Add(TEXT("Vulcan Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret"); M.bLoadout=false; M.Location=FVector(0.000000000,74.000000000,-189.000000000);
          M.Muzzle=FVector(-16.000000000,0.000000000,148.000000000);
          M.Muzzles.Add(FVector(-16.000000000,0.000000000,148.000000000));
          M.Muzzles.Add(FVector(16.000000000,0.000000000,148.000000000));
          M.BarrelOffsets.Add(FVector(39.466666667,-4.266666667,0.000000000));
          M.BarrelOffsets.Add(FVector(39.466666667,4.266666667,0.000000000));
          M.Types.Add(TEXT("LCA Turret"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZA")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,20.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,80.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,80.000000000));
          M.BarrelOffsets.Add(FVector(60.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZCA")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-60.000000000,0.000000000,128.000000000);
          M.Muzzle=FVector(-64.000000000,0.000000000,192.000000000);
          M.Muzzles.Add(FVector(-64.000000000,0.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,-4.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(60.000000000,0.000000000,128.000000000);
          M.Muzzle=FVector(64.000000000,0.000000000,192.000000000);
          M.Muzzles.Add(FVector(64.000000000,0.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,4.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 3"); M.bLoadout=false; M.Location=FVector(0.000000000,-60.000000000,128.000000000);
          M.Muzzle=FVector(0.000000000,-64.000000000,192.000000000);
          M.Muzzles.Add(FVector(0.000000000,-64.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,0.000000000,-4.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 4"); M.bLoadout=false; M.Location=FVector(0.000000000,60.000000000,128.000000000);
          M.Muzzle=FVector(0.000000000,64.000000000,192.000000000);
          M.Muzzles.Add(FVector(0.000000000,64.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,0.000000000,4.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZCS")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-60.000000000,0.000000000,128.000000000);
          M.Muzzle=FVector(-64.000000000,0.000000000,192.000000000);
          M.Muzzles.Add(FVector(-64.000000000,0.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,-4.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(60.000000000,0.000000000,128.000000000);
          M.Muzzle=FVector(64.000000000,0.000000000,192.000000000);
          M.Muzzles.Add(FVector(64.000000000,0.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,4.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 3"); M.bLoadout=false; M.Location=FVector(0.000000000,-60.000000000,128.000000000);
          M.Muzzle=FVector(0.000000000,-64.000000000,192.000000000);
          M.Muzzles.Add(FVector(0.000000000,-64.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,0.000000000,-4.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 4"); M.bLoadout=false; M.Location=FVector(0.000000000,60.000000000,128.000000000);
          M.Muzzle=FVector(0.000000000,64.000000000,192.000000000);
          M.Muzzles.Add(FVector(0.000000000,64.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,0.000000000,4.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZDD")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(0.000000000,200.000000000,-228.000000000);
          M.Muzzle=FVector(0.000000000,200.000000000,-190.000000000);
          M.Muzzles.Add(FVector(0.000000000,200.000000000,-190.000000000));
          M.BarrelOffsets.Add(FVector(38.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(0.000000000,-200.000000000,-228.000000000);
          M.Muzzle=FVector(0.000000000,-200.000000000,-190.000000000);
          M.Muzzles.Add(FVector(0.000000000,-200.000000000,-190.000000000));
          M.BarrelOffsets.Add(FVector(38.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZFF")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-60.000000000,0.000000000,128.000000000);
          M.Muzzle=FVector(-64.000000000,0.000000000,192.000000000);
          M.Muzzles.Add(FVector(-64.000000000,0.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,-4.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(60.000000000,0.000000000,128.000000000);
          M.Muzzle=FVector(64.000000000,0.000000000,192.000000000);
          M.Muzzles.Add(FVector(64.000000000,0.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,4.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 3"); M.bLoadout=false; M.Location=FVector(0.000000000,-60.000000000,128.000000000);
          M.Muzzle=FVector(0.000000000,-64.000000000,192.000000000);
          M.Muzzles.Add(FVector(0.000000000,-64.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,0.000000000,-4.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Laser 4"); M.bLoadout=false; M.Location=FVector(0.000000000,60.000000000,128.000000000);
          M.Muzzle=FVector(0.000000000,64.000000000,192.000000000);
          M.Muzzles.Add(FVector(0.000000000,64.000000000,192.000000000));
          M.BarrelOffsets.Add(FVector(64.000000000,0.000000000,4.000000000));
          M.Types.Add(TEXT("Zolon Laser"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZF")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000000,0.000000000,10.000000000);
          M.Muzzle=FVector(0.000000000,0.000000000,90.000000000);
          M.Muzzles.Add(FVector(0.000000000,0.000000000,90.000000000));
          M.BarrelOffsets.Add(FVector(80.000000000,0.000000000,0.000000000));
          M.Types.Add(TEXT("Zolon Cannon"));
          Out.Add(M); }
        return;
    }
}
