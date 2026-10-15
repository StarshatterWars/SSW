#include "ShipWeaponMountComponent.h"
void ShipWeaponMountCatalog::Get(FName ShipDef,TArray<FNativeWeaponMountDef>& Out)
{
    Out.Reset();
    if(ShipDef==FName(TEXT("Asher"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-22.0,-9.0,400.0); M.Muzzle=FVector(-22.0,-9.0,425.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(0.0,-22.0,420.0); M.Muzzle=FVector(0.0,-22.0,425.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(22.0,-9.0,420.0); M.Muzzle=FVector(22.0,-9.0,425.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Starboard Torpedo"); M.bLoadout=false; M.Location=FVector(100.0,-12.0,0.0); M.Muzzle=FVector(106.0,-12.0,60.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Fusion Torpedo")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Port Torpedo"); M.bLoadout=false; M.Location=FVector(-100.0,-12.0,0.0); M.Muzzle=FVector(-106.0,-12.0,60.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Fusion Torpedo")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Avenger"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,160.0); M.Muzzle=FVector(-33.0,-21.0,205.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-111.0,-35.0,-100.0); M.Muzzle=FVector(-111.0,-35.0,-135.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-93.0,-34.0,-100.0); M.Muzzle=FVector(-93.0,-34.0,-130.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-76.0,-33.0,-100.0); M.Muzzle=FVector(-76.0,-33.0,-125.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-58.0,-32.0,-100.0); M.Muzzle=FVector(-58.0,-32.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(58.0,-32.0,-100.0); M.Muzzle=FVector(58.0,-32.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(76.0,-33.0,-100.0); M.Muzzle=FVector(76.0,-33.0,-125.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(93.0,-34.0,-100.0); M.Muzzle=FVector(93.0,-34.0,-130.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(111.0,-35.0,-100.0); M.Muzzle=FVector(111.0,-35.0,-135.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Baikal"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Fwd Cannon"); M.bLoadout=false; M.Location=FVector(0.0,25.0,500.0); M.Muzzle=FVector(0.0,25.0,540.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Plasma Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-50.0,0.0,-40.0); M.Muzzle=FVector(-20.0,0.0,450.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Mk7 Interceptor")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Berents"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Plasma Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,300.0); M.Muzzle=FVector(0.0,4.0,333.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Plasma Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-33.0,0.0,220.0); M.Muzzle=FVector(-33.0,33.0,233.0); M.Azimuth=-0.15; M.Elevation=0.0;
          M.Types.Add(TEXT("Mk7 Interceptor")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Interceptor 2"); M.bLoadout=false; M.Location=FVector(33.0,0.0,220.0); M.Muzzle=FVector(33.0,33.0,233.0); M.Azimuth=0.15; M.Elevation=0.0;
          M.Types.Add(TEXT("Mk7 Interceptor")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Broadsword"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-45.0,-33.0,340.0); M.Muzzle=FVector(-55.0,-33.0,470.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(44.0,-33.0,340.0); M.Muzzle=FVector(54.0,-33.0,470.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-45.0,-55.0,340.0); M.Muzzle=FVector(-55.0,-55.0,448.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(44.0,-55.0,340.0); M.Muzzle=FVector(54.0,-55.0,448.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Missile Array"); M.bLoadout=false; M.Location=FVector(0.0,-1.0,220.0); M.Muzzle=FVector(-25.0,-1.0,422.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Nike")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Cobra"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,-32.0); M.Muzzle=FVector(-7.0,8.0,175.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(129.0,-21.0,-30.0); M.Muzzle=FVector(129.0,-21.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(105.0,-18.0,-30.0); M.Muzzle=FVector(105.0,-18.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(65.0,-16.0,-30.0); M.Muzzle=FVector(65.0,-16.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(40.0,-14.0,-30.0); M.Muzzle=FVector(40.0,-14.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-40.0,-14.0,-30.0); M.Muzzle=FVector(-40.0,-14.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-65.0,-16.0,-30.0); M.Muzzle=FVector(-65.0,-16.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(-105.0,-18.0,-30.0); M.Muzzle=FVector(-105.0,-18.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(-128.0,-21.0,-30.0); M.Muzzle=FVector(-128.0,-21.0,-70.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Courageous"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-40.0,-12.0,600.0); M.Muzzle=FVector(-51.0,-12.0,641.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(40.0,-12.0,600.0); M.Muzzle=FVector(51.0,-12.0,641.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-30.0,-45.0,580.0); M.Muzzle=FVector(-37.0,-45.0,623.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(30.0,-45.0,580.0); M.Muzzle=FVector(37.0,-45.0,623.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Starboard Missile"); M.bLoadout=false; M.Location=FVector(100.0,22.0,250.0); M.Muzzle=FVector(70.0,55.0,355.0); M.Azimuth=0.2617993877991494; M.Elevation=0.0;
          M.Types.Add(TEXT("Nike")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Port Missile"); M.bLoadout=false; M.Location=FVector(-100.0,22.0,250.0); M.Muzzle=FVector(-70.0,55.0,355.0); M.Azimuth=-0.2617993877991494; M.Elevation=0.0;
          M.Types.Add(TEXT("Nike")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Interceptor"); M.bLoadout=false; M.Location=FVector(0.0,30.0,250.0); M.Muzzle=FVector(35.0,75.0,355.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Mk7 Interceptor")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Cutlass"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,32.0); M.Muzzle=FVector(-150.0,5.0,-10.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-46.0,-10.0,-25.0); M.Muzzle=FVector(-64.0,-10.0,-100.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(64.0,-10.0,-25.0); M.Muzzle=FVector(64.0,-10.0,-100.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Devastator"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-65.0,-22.0,730.0); M.Muzzle=FVector(-65.0,-22.0,760.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(65.0,-22.0,730.0); M.Muzzle=FVector(65.0,-22.0,760.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 33"); M.bLoadout=false; M.Location=FVector(-52.0,-54.0,710.0); M.Muzzle=FVector(-52.0,-54.0,740.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 4"); M.bLoadout=false; M.Location=FVector(52.0,-54.0,710.0); M.Muzzle=FVector(52.0,-54.0,740.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Starboard Missile"); M.bLoadout=false; M.Location=FVector(60.0,0.0,400.0); M.Muzzle=FVector(75.0,13.0,600.0); M.Azimuth=0.2617993877991494; M.Elevation=0.0;
          M.Types.Add(TEXT("Athena1")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Port Missile"); M.bLoadout=false; M.Location=FVector(-60.0,0.0,400.0); M.Muzzle=FVector(-75.0,13.0,600.0); M.Azimuth=-0.2617993877991494; M.Elevation=0.0;
          M.Types.Add(TEXT("Athena2")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Dragon"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-30.0,-15.0,330.0); M.Muzzle=FVector(-283.0,6.0,1650.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(30.0,-15.0,330.0); M.Muzzle=FVector(283.0,6.0,1650.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Eagle"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,32.0); M.Muzzle=FVector(-19.0,-3.0,177.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Delta Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-69.0,-22.0,-60.0); M.Muzzle=FVector(-69.0,-22.0,-75.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-48.0,-17.0,-60.0); M.Muzzle=FVector(-48.0,-17.0,-75.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-20.0,-13.0,-15.0); M.Muzzle=FVector(-20.0,-13.0,-25.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-14.0,-14.0,60.0); M.Muzzle=FVector(-14.0,-14.0,52.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(15.0,-14.0,60.0); M.Muzzle=FVector(15.0,-14.0,52.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(20.0,-13.0,-15.0); M.Muzzle=FVector(20.0,-13.0,-25.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(48.0,-17.0,-60.0); M.Muzzle=FVector(48.0,-17.0,-75.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(70.0,-22.0,-60.0); M.Muzzle=FVector(70.0,-22.0,-75.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Falcon"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,-32.0); M.Muzzle=FVector(-21.0,0.0,130.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Delta Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-110.0,-7.0,-145.0); M.Muzzle=FVector(-110.0,-7.0,-145.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-93.0,-5.0,-145.0); M.Muzzle=FVector(-93.0,-5.0,-145.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(93.0,-5.0,-145.0); M.Muzzle=FVector(93.0,-5.0,-145.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(110.0,-7.0,-145.0); M.Muzzle=FVector(110.0,-7.0,-145.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Broadsword"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-45.0,-33.0,340.0); M.Muzzle=FVector(-55.0,-33.0,470.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(44.0,-33.0,340.0); M.Muzzle=FVector(54.0,-33.0,470.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-45.0,-55.0,340.0); M.Muzzle=FVector(-55.0,-55.0,448.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(44.0,-55.0,340.0); M.Muzzle=FVector(54.0,-55.0,448.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Missile Array"); M.bLoadout=false; M.Location=FVector(0.0,-1.0,220.0); M.Muzzle=FVector(-25.0,-1.0,422.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Nike")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Dragon"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-30.0,-15.0,330.0); M.Muzzle=FVector(-283.0,6.0,1650.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(30.0,-15.0,330.0); M.Muzzle=FVector(283.0,6.0,1650.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Imperial"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-42.0,-9.0,620.0); M.Muzzle=FVector(-42.0,-9.0,640.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(42.0,-9.0,620.0); M.Muzzle=FVector(42.0,-9.0,640.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Tiger"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(74.0,-12.0,110.0); M.Muzzle=FVector(74.0,-12.0,240.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(-74.0,-12.0,110.0); M.Muzzle=FVector(-74.0,-12.0,240.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-20.0,0.0,300.0); M.Muzzle=FVector(-40.0,0.0,390.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Mk7 Interceptor")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Vendetta"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-8.0,33.0,600.0); M.Muzzle=FVector(-8.0,33.0,779.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(8.0,33.0,600.0); M.Muzzle=FVector(8.0,33.0,779.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Hulk-Volnaris"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-34.0,-19.0,460.0); M.Muzzle=FVector(-34.0,-19.0,570.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(34.0,-19.0,460.0); M.Muzzle=FVector(34.0,-19.0,570.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Torpedo Banks"); M.bLoadout=false; M.Location=FVector(0.0,-32.0,400.0); M.Muzzle=FVector(-25.0,17.0,470.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Fusion Torpedo")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Imperial"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-42.0,-9.0,620.0); M.Muzzle=FVector(-42.0,-9.0,640.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(42.0,-9.0,620.0); M.Muzzle=FVector(42.0,-9.0,640.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Intruder"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,120.0); M.Muzzle=FVector(-24.0,-6.0,145.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-93.0,-27.0,-100.0); M.Muzzle=FVector(-93.0,-27.0,-125.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-62.0,-25.0,-100.0); M.Muzzle=FVector(-62.0,-25.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(62.0,-25.0,-100.0); M.Muzzle=FVector(62.0,-25.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(93.0,-27.0,-100.0); M.Muzzle=FVector(93.0,-27.0,-125.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Jackal"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-8.0,-26.0,160.0); M.Muzzle=FVector(-8.0,-26.0,182.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Vulcan Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Starboard Cannon"); M.bLoadout=false; M.Location=FVector(8.0,-26.0,160.0); M.Muzzle=FVector(8.0,-26.0,182.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Vulcan Cannon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Marauder"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-260.0,-32.0,60.0); M.Muzzle=FVector(-260.0,-32.0,194.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Plasma Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Starboard Cannon"); M.bLoadout=false; M.Location=FVector(260.0,-32.0,60.0); M.Muzzle=FVector(260.0,-32.0,194.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Plasma Cannon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("QShip"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Port Cannon"); M.bLoadout=false; M.Location=FVector(-30.0,0.0,360.0); M.Muzzle=FVector(-30.0,0.0,400.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Plasma Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Starboard Cannon"); M.bLoadout=false; M.Location=FVector(30.0,0.0,360.0); M.Muzzle=FVector(30.0,0.0,400.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Plasma Cannon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Raptor"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,120.0); M.Muzzle=FVector(-17.0,-19.0,170.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-120.0,-49.0,-100.0); M.Muzzle=FVector(-120.0,-49.0,-140.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-99.0,-47.0,-100.0); M.Muzzle=FVector(-99.0,-47.0,-130.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-79.0,-44.0,-100.0); M.Muzzle=FVector(-79.0,-44.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-60.0,-41.0,-100.0); M.Muzzle=FVector(-60.0,-41.0,-110.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(60.0,-41.0,-100.0); M.Muzzle=FVector(60.0,-41.0,-110.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(79.0,-44.0,-100.0); M.Muzzle=FVector(79.0,-44.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(99.0,-45.0,-100.0); M.Muzzle=FVector(99.0,-45.0,-130.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(120.0,-49.0,-100.0); M.Muzzle=FVector(120.0,-49.0,-140.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Razor"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,32.0); M.Muzzle=FVector(-7.0,9.0,190.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(126.0,-29.0,0.0); M.Muzzle=FVector(126.0,-29.0,-50.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(102.0,-26.0,0.0); M.Muzzle=FVector(102.0,-26.0,-50.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(62.0,-21.0,0.0); M.Muzzle=FVector(62.0,-21.0,-8.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(36.0,-19.0,0.0); M.Muzzle=FVector(36.0,-19.0,-8.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-36.0,-19.0,0.0); M.Muzzle=FVector(-36.0,-19.0,-8.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-62.0,-21.0,0.0); M.Muzzle=FVector(-62.0,-21.0,-8.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(-102.0,-26.0,0.0); M.Muzzle=FVector(-102.0,-26.0,-50.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(-126.0,-29.0,0.0); M.Muzzle=FVector(-126.0,-29.0,-50.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Saber"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,32.0); M.Muzzle=FVector(-190.0,15.0,55.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-101.0,11.0,-10.0); M.Muzzle=FVector(-101.0,11.0,-85.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-74.0,1.0,-10.0); M.Muzzle=FVector(-74.0,1.0,-75.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(74.0,1.0,-10.0); M.Muzzle=FVector(74.0,1.0,-75.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(101.0,11.0,-10.0); M.Muzzle=FVector(101.0,11.0,-85.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Spectre"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-36.0,-9.0,400.0); M.Muzzle=FVector(-36.0,-9.0,467.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(36.0,-9.0,400.0); M.Muzzle=FVector(36.0,-9.0,467.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 3"); M.bLoadout=false; M.Location=FVector(-14.0,-23.0,400.0); M.Muzzle=FVector(-14.0,-23.0,467.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 4"); M.bLoadout=false; M.Location=FVector(14.0,-23.0,400.0); M.Muzzle=FVector(14.0,-23.0,467.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Starboard Torpedo"); M.bLoadout=false; M.Location=FVector(0,0,0); M.Muzzle=FVector(192.0,-32.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Fusion Torpedo")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Port Torpedo"); M.bLoadout=false; M.Location=FVector(0,0,0); M.Muzzle=FVector(-192.0,-32.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Fusion Torpedo")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Stormhawk"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,32.0); M.Muzzle=FVector(-28.0,2.0,77.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Delta Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-78.0,-10.0,-120.0); M.Muzzle=FVector(-78.0,-10.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-50.0,-5.0,-110.0); M.Muzzle=FVector(-50.0,-5.0,-110.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-16.0,-18.0,-110.0); M.Muzzle=FVector(-16.0,-18.0,-110.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(16.0,-18.0,-110.0); M.Muzzle=FVector(16.0,-18.0,-110.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); M.Types.Add(TEXT("Recon Pod")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(50.0,-5.0,-110.0); M.Muzzle=FVector(50.0,-5.0,-110.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(78.0,-10.0,-120.0); M.Muzzle=FVector(78.0,-10.0,-120.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Thunderbolt"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,160.0); M.Muzzle=FVector(-22.0,-1.0,200.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Heavy Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-116.0,-25.0,-70.0); M.Muzzle=FVector(-116.0,-25.0,-82.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-60.0,-16.0,-60.0); M.Muzzle=FVector(-60.0,-16.0,-72.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-40.0,-16.0,-60.0); M.Muzzle=FVector(-40.0,-16.0,-72.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(40.0,-16.0,-60.0); M.Muzzle=FVector(40.0,-16.0,-72.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); M.Types.Add(TEXT("Rockets")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(60.0,-16.0,-60.0); M.Muzzle=FVector(60.0,-16.0,-72.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(116.0,-25.0,-70.0); M.Muzzle=FVector(116.0,-25.0,-82.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Javelin")); M.Types.Add(TEXT("Vector")); M.Types.Add(TEXT("Harpoon")); M.Types.Add(TEXT("Hammer")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Tiger"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(74.0,-12.0,110.0); M.Muzzle=FVector(74.0,-12.0,240.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(-74.0,-12.0,110.0); M.Muzzle=FVector(-74.0,-12.0,240.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Interceptor 1"); M.bLoadout=false; M.Location=FVector(-20.0,0.0,300.0); M.Muzzle=FVector(-40.0,0.0,390.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Mk7 Interceptor")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Vendetta"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 1"); M.bLoadout=false; M.Location=FVector(-8.0,33.0,600.0); M.Muzzle=FVector(-8.0,33.0,779.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Gamma Laser 2"); M.bLoadout=false; M.Location=FVector(8.0,33.0,600.0); M.Muzzle=FVector(8.0,33.0,779.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Gamma Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Viper"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,120.0); M.Muzzle=FVector(-7.0,7.0,190.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Alpha Cannon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(103.0,-39.0,-100.0); M.Muzzle=FVector(103.0,-39.0,-118.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(83.0,-35.0,-100.0); M.Muzzle=FVector(83.0,-35.0,-118.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(51.0,-25.0,-60.0); M.Muzzle=FVector(51.0,-25.0,-80.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-51.0,-25.0,-60.0); M.Muzzle=FVector(-51.0,-25.0,-80.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-84.0,-33.0,-100.0); M.Muzzle=FVector(-84.0,-33.0,-118.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-103.0,-39.0,-100.0); M.Muzzle=FVector(-103.0,-39.0,-118.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Archer")); M.Types.Add(TEXT("Harpoon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Volnaris"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-34.0,-19.0,460.0); M.Muzzle=FVector(-34.0,-19.0,570.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(34.0,-19.0,460.0); M.Muzzle=FVector(34.0,-19.0,570.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Torpedo Banks"); M.bLoadout=false; M.Location=FVector(0.0,-32.0,400.0); M.Muzzle=FVector(-25.0,17.0,470.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Fusion Torpedo")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Wolf"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 1"); M.bLoadout=false; M.Location=FVector(-25.0,23.0,800.0); M.Muzzle=FVector(-25.0,23.0,907.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("XRay Laser 2"); M.bLoadout=false; M.Location=FVector(25.0,23.0,800.0); M.Muzzle=FVector(25.0,23.0,907.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("XRay Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("Zodiac"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Main Cannon"); M.bLoadout=false; M.Location=FVector(0.0,-10.0,110.0); M.Muzzle=FVector(-38.0,-21.0,191.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Vulcan Cannon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("ZZA"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,20.0); M.Muzzle=FVector(0.0,0.0,80.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Cannon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("ZZCA"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-60.0,0.0,128.0); M.Muzzle=FVector(-64.0,0.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(60.0,0.0,128.0); M.Muzzle=FVector(64.0,0.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 3"); M.bLoadout=false; M.Location=FVector(0.0,-60.0,128.0); M.Muzzle=FVector(0.0,-64.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 4"); M.bLoadout=false; M.Location=FVector(0.0,60.0,128.0); M.Muzzle=FVector(0.0,64.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("ZZCS"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-60.0,0.0,128.0); M.Muzzle=FVector(-64.0,0.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(60.0,0.0,128.0); M.Muzzle=FVector(64.0,0.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 3"); M.bLoadout=false; M.Location=FVector(0.0,-60.0,128.0); M.Muzzle=FVector(0.0,-64.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 4"); M.bLoadout=false; M.Location=FVector(0.0,60.0,128.0); M.Muzzle=FVector(0.0,64.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("ZZDD"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(0.0,200.0,-228.0); M.Muzzle=FVector(0.0,200.0,-190.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(0.0,-200.0,-228.0); M.Muzzle=FVector(0.0,-200.0,-190.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("ZZF"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.0,0.0,10.0); M.Muzzle=FVector(0.0,0.0,90.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Cannon")); Out.Add(M); }
        return;
    }
    if(ShipDef==FName(TEXT("ZZFF"))) {
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 1"); M.bLoadout=false; M.Location=FVector(-60.0,0.0,128.0); M.Muzzle=FVector(-64.0,0.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 2"); M.bLoadout=false; M.Location=FVector(60.0,0.0,128.0); M.Muzzle=FVector(64.0,0.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 3"); M.bLoadout=false; M.Location=FVector(0.0,-60.0,128.0); M.Muzzle=FVector(0.0,-64.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        { FNativeWeaponMountDef M; M.Name=TEXT("Laser 4"); M.bLoadout=false; M.Location=FVector(0.0,60.0,128.0); M.Muzzle=FVector(0.0,64.0,192.0); M.Azimuth=0.0; M.Elevation=0.0;
          M.Types.Add(TEXT("Zolon Laser")); Out.Add(M); }
        return;
    }
}
