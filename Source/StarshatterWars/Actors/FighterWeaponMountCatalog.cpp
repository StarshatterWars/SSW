#include "ShipWeaponMountComponent.h"

void FighterWeaponMountCatalog::Get(FName ShipDef, TArray<FNativeWeaponMountDef>& Out)
{
    Out.Reset();
    if (ShipDef == FName(TEXT("Avenger")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,160.000000);
          M.Muzzle=FVector(-33.000000,-21.000000,205.000000);
          M.Muzzles.Add(FVector(-33.000000,-21.000000,205.000000));
          M.Muzzles.Add(FVector(-17.000000,-24.000000,180.000000));
          M.Muzzles.Add(FVector(17.000000,-24.000000,180.000000));
          M.Muzzles.Add(FVector(31.000000,-21.000000,205.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-111.000000,-35.000000,-100.000000);
          M.Muzzle=FVector(-111.000000,-35.000000,-135.000000);
          M.Muzzles.Add(FVector(-111.000000,-35.000000,-135.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-93.000000,-34.000000,-100.000000);
          M.Muzzle=FVector(-93.000000,-34.000000,-130.000000);
          M.Muzzles.Add(FVector(-93.000000,-34.000000,-130.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-76.000000,-33.000000,-100.000000);
          M.Muzzle=FVector(-76.000000,-33.000000,-125.000000);
          M.Muzzles.Add(FVector(-76.000000,-33.000000,-125.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-58.000000,-32.000000,-100.000000);
          M.Muzzle=FVector(-58.000000,-32.000000,-120.000000);
          M.Muzzles.Add(FVector(-58.000000,-32.000000,-120.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(58.000000,-32.000000,-100.000000);
          M.Muzzle=FVector(58.000000,-32.000000,-120.000000);
          M.Muzzles.Add(FVector(58.000000,-32.000000,-120.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(76.000000,-33.000000,-100.000000);
          M.Muzzle=FVector(76.000000,-33.000000,-125.000000);
          M.Muzzles.Add(FVector(76.000000,-33.000000,-125.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(93.000000,-34.000000,-100.000000);
          M.Muzzle=FVector(93.000000,-34.000000,-130.000000);
          M.Muzzles.Add(FVector(93.000000,-34.000000,-130.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(111.000000,-35.000000,-100.000000);
          M.Muzzle=FVector(111.000000,-35.000000,-135.000000);
          M.Muzzles.Add(FVector(111.000000,-35.000000,-135.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Cobra")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,-32.000000);
          M.Muzzle=FVector(-7.000000,8.000000,175.000000);
          M.Muzzles.Add(FVector(-7.000000,8.000000,175.000000));
          M.Muzzles.Add(FVector(7.000000,8.000000,175.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(129.000000,-21.000000,-30.000000);
          M.Muzzle=FVector(129.000000,-21.000000,-70.000000);
          M.Muzzles.Add(FVector(129.000000,-21.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(105.000000,-18.000000,-30.000000);
          M.Muzzle=FVector(105.000000,-18.000000,-70.000000);
          M.Muzzles.Add(FVector(105.000000,-18.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(65.000000,-16.000000,-30.000000);
          M.Muzzle=FVector(65.000000,-16.000000,-70.000000);
          M.Muzzles.Add(FVector(65.000000,-16.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(40.000000,-14.000000,-30.000000);
          M.Muzzle=FVector(40.000000,-14.000000,-70.000000);
          M.Muzzles.Add(FVector(40.000000,-14.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-40.000000,-14.000000,-30.000000);
          M.Muzzle=FVector(-40.000000,-14.000000,-70.000000);
          M.Muzzles.Add(FVector(-40.000000,-14.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-65.000000,-16.000000,-30.000000);
          M.Muzzle=FVector(-65.000000,-16.000000,-70.000000);
          M.Muzzles.Add(FVector(-65.000000,-16.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(-105.000000,-18.000000,-30.000000);
          M.Muzzle=FVector(-105.000000,-18.000000,-70.000000);
          M.Muzzles.Add(FVector(-105.000000,-18.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(-128.000000,-21.000000,-30.000000);
          M.Muzzle=FVector(-128.000000,-21.000000,-70.000000);
          M.Muzzles.Add(FVector(-128.000000,-21.000000,-70.000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Cutlass")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,32.000000);
          M.Muzzle=FVector(-150.000000,5.000000,-10.000000);
          M.Muzzles.Add(FVector(-150.000000,5.000000,-10.000000));
          M.Muzzles.Add(FVector(-150.000000,-6.000000,-10.000000));
          M.Muzzles.Add(FVector(150.000000,5.000000,-10.000000));
          M.Muzzles.Add(FVector(150.000000,-6.000000,-10.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-46.000000,-10.000000,-25.000000);
          M.Muzzle=FVector(-64.000000,-10.000000,-100.000000);
          M.Muzzles.Add(FVector(-64.000000,-10.000000,-100.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(64.000000,-10.000000,-25.000000);
          M.Muzzle=FVector(64.000000,-10.000000,-100.000000);
          M.Muzzles.Add(FVector(64.000000,-10.000000,-100.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Eagle")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,32.000000);
          M.Muzzle=FVector(-19.000000,-3.000000,177.000000);
          M.Muzzles.Add(FVector(-19.000000,-3.000000,177.000000));
          M.Muzzles.Add(FVector(19.000000,-3.000000,177.000000));
          M.Types.Add(TEXT("Delta Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-69.000000,-22.000000,-60.000000);
          M.Muzzle=FVector(-69.000000,-22.000000,-75.000000);
          M.Muzzles.Add(FVector(-69.000000,-22.000000,-75.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-48.000000,-17.000000,-60.000000);
          M.Muzzle=FVector(-48.000000,-17.000000,-75.000000);
          M.Muzzles.Add(FVector(-48.000000,-17.000000,-75.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-20.000000,-13.000000,-15.000000);
          M.Muzzle=FVector(-20.000000,-13.000000,-25.000000);
          M.Muzzles.Add(FVector(-20.000000,-13.000000,-25.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-14.000000,-14.000000,60.000000);
          M.Muzzle=FVector(-14.000000,-14.000000,52.000000);
          M.Muzzles.Add(FVector(-14.000000,-14.000000,52.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(15.000000,-14.000000,60.000000);
          M.Muzzle=FVector(15.000000,-14.000000,52.000000);
          M.Muzzles.Add(FVector(15.000000,-14.000000,52.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(20.000000,-13.000000,-15.000000);
          M.Muzzle=FVector(20.000000,-13.000000,-25.000000);
          M.Muzzles.Add(FVector(20.000000,-13.000000,-25.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(48.000000,-17.000000,-60.000000);
          M.Muzzle=FVector(48.000000,-17.000000,-75.000000);
          M.Muzzles.Add(FVector(48.000000,-17.000000,-75.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(70.000000,-22.000000,-60.000000);
          M.Muzzle=FVector(70.000000,-22.000000,-75.000000);
          M.Muzzles.Add(FVector(70.000000,-22.000000,-75.000000));
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
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,-32.000000);
          M.Muzzle=FVector(-21.000000,0.000000,130.000000);
          M.Muzzles.Add(FVector(-21.000000,0.000000,130.000000));
          M.Muzzles.Add(FVector(21.000000,0.000000,130.000000));
          M.Types.Add(TEXT("Delta Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-110.000000,-7.000000,-145.000000);
          M.Muzzle=FVector(-110.000000,-7.000000,-145.000000);
          M.Muzzles.Add(FVector(-110.000000,-7.000000,-145.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-93.000000,-5.000000,-145.000000);
          M.Muzzle=FVector(-93.000000,-5.000000,-145.000000);
          M.Muzzles.Add(FVector(-93.000000,-5.000000,-145.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(93.000000,-5.000000,-145.000000);
          M.Muzzle=FVector(93.000000,-5.000000,-145.000000);
          M.Muzzles.Add(FVector(93.000000,-5.000000,-145.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(110.000000,-7.000000,-145.000000);
          M.Muzzle=FVector(110.000000,-7.000000,-145.000000);
          M.Muzzles.Add(FVector(110.000000,-7.000000,-145.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Intruder")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,120.000000);
          M.Muzzle=FVector(-24.000000,-6.000000,145.000000);
          M.Muzzles.Add(FVector(-24.000000,-6.000000,145.000000));
          M.Muzzles.Add(FVector(24.000000,-6.000000,145.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-93.000000,-27.000000,-100.000000);
          M.Muzzle=FVector(-93.000000,-27.000000,-125.000000);
          M.Muzzles.Add(FVector(-93.000000,-27.000000,-125.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-62.000000,-25.000000,-100.000000);
          M.Muzzle=FVector(-62.000000,-25.000000,-120.000000);
          M.Muzzles.Add(FVector(-62.000000,-25.000000,-120.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(62.000000,-25.000000,-100.000000);
          M.Muzzle=FVector(62.000000,-25.000000,-120.000000);
          M.Muzzles.Add(FVector(62.000000,-25.000000,-120.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(93.000000,-27.000000,-100.000000);
          M.Muzzle=FVector(93.000000,-27.000000,-125.000000);
          M.Muzzles.Add(FVector(93.000000,-27.000000,-125.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Jackal")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-1"); M.bLoadout=false; M.Location=FVector(-8.000000,-26.000000,160.000000);
          M.Muzzle=FVector(-8.000000,-26.000000,182.000000);
          M.Muzzles.Add(FVector(-8.000000,-26.000000,182.000000));
          M.Types.Add(TEXT("Vulcan Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Gun-2"); M.bLoadout=false; M.Location=FVector(8.000000,-26.000000,160.000000);
          M.Muzzle=FVector(8.000000,-26.000000,182.000000);
          M.Muzzles.Add(FVector(8.000000,-26.000000,182.000000));
          M.Types.Add(TEXT("Vulcan Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Raptor")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,120.000000);
          M.Muzzle=FVector(-17.000000,-19.000000,170.000000);
          M.Muzzles.Add(FVector(-17.000000,-19.000000,170.000000));
          M.Muzzles.Add(FVector(17.000000,-19.000000,170.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-120.000000,-49.000000,-100.000000);
          M.Muzzle=FVector(-120.000000,-49.000000,-140.000000);
          M.Muzzles.Add(FVector(-120.000000,-49.000000,-140.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-99.000000,-47.000000,-100.000000);
          M.Muzzle=FVector(-99.000000,-47.000000,-130.000000);
          M.Muzzles.Add(FVector(-99.000000,-47.000000,-130.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-79.000000,-44.000000,-100.000000);
          M.Muzzle=FVector(-79.000000,-44.000000,-120.000000);
          M.Muzzles.Add(FVector(-79.000000,-44.000000,-120.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-60.000000,-41.000000,-100.000000);
          M.Muzzle=FVector(-60.000000,-41.000000,-110.000000);
          M.Muzzles.Add(FVector(-60.000000,-41.000000,-110.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(60.000000,-41.000000,-100.000000);
          M.Muzzle=FVector(60.000000,-41.000000,-110.000000);
          M.Muzzles.Add(FVector(60.000000,-41.000000,-110.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(79.000000,-44.000000,-100.000000);
          M.Muzzle=FVector(79.000000,-44.000000,-120.000000);
          M.Muzzles.Add(FVector(79.000000,-44.000000,-120.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(99.000000,-45.000000,-100.000000);
          M.Muzzle=FVector(99.000000,-45.000000,-130.000000);
          M.Muzzles.Add(FVector(99.000000,-45.000000,-130.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(120.000000,-49.000000,-100.000000);
          M.Muzzle=FVector(120.000000,-49.000000,-140.000000);
          M.Muzzles.Add(FVector(120.000000,-49.000000,-140.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Razor")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,32.000000);
          M.Muzzle=FVector(-7.000000,9.000000,190.000000);
          M.Muzzles.Add(FVector(-7.000000,9.000000,190.000000));
          M.Muzzles.Add(FVector(7.000000,9.000000,190.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(126.000000,-29.000000,0.000000);
          M.Muzzle=FVector(126.000000,-29.000000,-50.000000);
          M.Muzzles.Add(FVector(126.000000,-29.000000,-50.000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(102.000000,-26.000000,0.000000);
          M.Muzzle=FVector(102.000000,-26.000000,-50.000000);
          M.Muzzles.Add(FVector(102.000000,-26.000000,-50.000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(62.000000,-21.000000,0.000000);
          M.Muzzle=FVector(62.000000,-21.000000,-8.000000);
          M.Muzzles.Add(FVector(62.000000,-21.000000,-8.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(36.000000,-19.000000,0.000000);
          M.Muzzle=FVector(36.000000,-19.000000,-8.000000);
          M.Muzzles.Add(FVector(36.000000,-19.000000,-8.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-36.000000,-19.000000,0.000000);
          M.Muzzle=FVector(-36.000000,-19.000000,-8.000000);
          M.Muzzles.Add(FVector(-36.000000,-19.000000,-8.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-62.000000,-21.000000,0.000000);
          M.Muzzle=FVector(-62.000000,-21.000000,-8.000000);
          M.Muzzles.Add(FVector(-62.000000,-21.000000,-8.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-7"); M.bLoadout=true; M.Location=FVector(-102.000000,-26.000000,0.000000);
          M.Muzzle=FVector(-102.000000,-26.000000,-50.000000);
          M.Muzzles.Add(FVector(-102.000000,-26.000000,-50.000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-8"); M.bLoadout=true; M.Location=FVector(-126.000000,-29.000000,0.000000);
          M.Muzzle=FVector(-126.000000,-29.000000,-50.000000);
          M.Muzzles.Add(FVector(-126.000000,-29.000000,-50.000000));
          M.Types.Add(TEXT("Archer"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Saber")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,32.000000);
          M.Muzzle=FVector(-190.000000,15.000000,55.000000);
          M.Muzzles.Add(FVector(-190.000000,15.000000,55.000000));
          M.Muzzles.Add(FVector(190.000000,15.000000,55.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-101.000000,11.000000,-10.000000);
          M.Muzzle=FVector(-101.000000,11.000000,-85.000000);
          M.Muzzles.Add(FVector(-101.000000,11.000000,-85.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-74.000000,1.000000,-10.000000);
          M.Muzzle=FVector(-74.000000,1.000000,-75.000000);
          M.Muzzles.Add(FVector(-74.000000,1.000000,-75.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(74.000000,1.000000,-10.000000);
          M.Muzzle=FVector(74.000000,1.000000,-75.000000);
          M.Muzzles.Add(FVector(74.000000,1.000000,-75.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(101.000000,11.000000,-10.000000);
          M.Muzzle=FVector(101.000000,11.000000,-85.000000);
          M.Muzzles.Add(FVector(101.000000,11.000000,-85.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Stormhawk")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,32.000000);
          M.Muzzle=FVector(-28.000000,2.000000,77.000000);
          M.Muzzles.Add(FVector(-28.000000,2.000000,77.000000));
          M.Muzzles.Add(FVector(28.000000,2.000000,77.000000));
          M.Types.Add(TEXT("Delta Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-78.000000,-10.000000,-120.000000);
          M.Muzzle=FVector(-78.000000,-10.000000,-120.000000);
          M.Muzzles.Add(FVector(-78.000000,-10.000000,-120.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-50.000000,-5.000000,-110.000000);
          M.Muzzle=FVector(-50.000000,-5.000000,-110.000000);
          M.Muzzles.Add(FVector(-50.000000,-5.000000,-110.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-16.000000,-18.000000,-110.000000);
          M.Muzzle=FVector(-16.000000,-18.000000,-110.000000);
          M.Muzzles.Add(FVector(-16.000000,-18.000000,-110.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(16.000000,-18.000000,-110.000000);
          M.Muzzle=FVector(16.000000,-18.000000,-110.000000);
          M.Muzzles.Add(FVector(16.000000,-18.000000,-110.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          M.Types.Add(TEXT("Recon Pod"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(50.000000,-5.000000,-110.000000);
          M.Muzzle=FVector(50.000000,-5.000000,-110.000000);
          M.Muzzles.Add(FVector(50.000000,-5.000000,-110.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(78.000000,-10.000000,-120.000000);
          M.Muzzle=FVector(78.000000,-10.000000,-120.000000);
          M.Muzzles.Add(FVector(78.000000,-10.000000,-120.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Thunderbolt")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,160.000000);
          M.Muzzle=FVector(-22.000000,-1.000000,200.000000);
          M.Muzzles.Add(FVector(-22.000000,-1.000000,200.000000));
          M.Muzzles.Add(FVector(21.000000,-1.000000,200.000000));
          M.Types.Add(TEXT("Heavy Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,-195.000000);
          M.Muzzle=FVector(0.000000,0.000000,24.000000);
          M.Muzzles.Add(FVector(0.000000,0.000000,24.000000));
          M.Types.Add(TEXT("Fighter Turret"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(-116.000000,-25.000000,-70.000000);
          M.Muzzle=FVector(-116.000000,-25.000000,-82.000000);
          M.Muzzles.Add(FVector(-116.000000,-25.000000,-82.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(-60.000000,-16.000000,-60.000000);
          M.Muzzle=FVector(-60.000000,-16.000000,-72.000000);
          M.Muzzles.Add(FVector(-60.000000,-16.000000,-72.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(-40.000000,-16.000000,-60.000000);
          M.Muzzle=FVector(-40.000000,-16.000000,-72.000000);
          M.Muzzles.Add(FVector(-40.000000,-16.000000,-72.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(40.000000,-16.000000,-60.000000);
          M.Muzzle=FVector(40.000000,-16.000000,-72.000000);
          M.Muzzles.Add(FVector(40.000000,-16.000000,-72.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          M.Types.Add(TEXT("Rockets"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(60.000000,-16.000000,-60.000000);
          M.Muzzle=FVector(60.000000,-16.000000,-72.000000);
          M.Muzzles.Add(FVector(60.000000,-16.000000,-72.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(116.000000,-25.000000,-70.000000);
          M.Muzzle=FVector(116.000000,-25.000000,-82.000000);
          M.Muzzles.Add(FVector(116.000000,-25.000000,-82.000000));
          M.Types.Add(TEXT("Javelin"));
          M.Types.Add(TEXT("Vector"));
          M.Types.Add(TEXT("Harpoon"));
          M.Types.Add(TEXT("Hammer"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Viper")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,120.000000);
          M.Muzzle=FVector(-7.000000,7.000000,190.000000);
          M.Muzzles.Add(FVector(-7.000000,7.000000,190.000000));
          M.Muzzles.Add(FVector(7.000000,7.000000,190.000000));
          M.Types.Add(TEXT("Alpha Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-1"); M.bLoadout=true; M.Location=FVector(103.000000,-39.000000,-100.000000);
          M.Muzzle=FVector(103.000000,-39.000000,-118.000000);
          M.Muzzles.Add(FVector(103.000000,-39.000000,-118.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-2"); M.bLoadout=true; M.Location=FVector(83.000000,-35.000000,-100.000000);
          M.Muzzle=FVector(83.000000,-35.000000,-118.000000);
          M.Muzzles.Add(FVector(83.000000,-35.000000,-118.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-3"); M.bLoadout=true; M.Location=FVector(51.000000,-25.000000,-60.000000);
          M.Muzzle=FVector(51.000000,-25.000000,-80.000000);
          M.Muzzles.Add(FVector(51.000000,-25.000000,-80.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-4"); M.bLoadout=true; M.Location=FVector(-51.000000,-25.000000,-60.000000);
          M.Muzzle=FVector(-51.000000,-25.000000,-80.000000);
          M.Muzzles.Add(FVector(-51.000000,-25.000000,-80.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-5"); M.bLoadout=true; M.Location=FVector(-84.000000,-33.000000,-100.000000);
          M.Muzzle=FVector(-84.000000,-33.000000,-118.000000);
          M.Muzzles.Add(FVector(-84.000000,-33.000000,-118.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("STA-6"); M.bLoadout=true; M.Location=FVector(-103.000000,-39.000000,-100.000000);
          M.Muzzle=FVector(-103.000000,-39.000000,-118.000000);
          M.Muzzles.Add(FVector(-103.000000,-39.000000,-118.000000));
          M.Types.Add(TEXT("Archer"));
          M.Types.Add(TEXT("Harpoon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("Zodiac")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Guns"); M.bLoadout=false; M.Location=FVector(0.000000,-10.000000,110.000000);
          M.Muzzle=FVector(-38.000000,-21.000000,191.000000);
          M.Muzzles.Add(FVector(-38.000000,-21.000000,191.000000));
          M.Muzzles.Add(FVector(-30.000000,-22.000000,203.000000));
          M.Muzzles.Add(FVector(30.000000,-22.000000,203.000000));
          M.Muzzles.Add(FVector(38.000000,-21.000000,191.000000));
          M.Types.Add(TEXT("Vulcan Cannon"));
          Out.Add(M); }
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Turret"); M.bLoadout=false; M.Location=FVector(0.000000,74.000000,-189.000000);
          M.Muzzle=FVector(-16.000000,0.000000,148.000000);
          M.Muzzles.Add(FVector(-16.000000,0.000000,148.000000));
          M.Muzzles.Add(FVector(16.000000,0.000000,148.000000));
          M.Types.Add(TEXT("LCA Turret"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZA")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,20.000000);
          M.Muzzle=FVector(0.000000,0.000000,80.000000);
          M.Muzzles.Add(FVector(0.000000,0.000000,80.000000));
          M.Types.Add(TEXT("Zolon Cannon"));
          Out.Add(M); }
        return;
    }
    if (ShipDef == FName(TEXT("ZZF")))
    {
        { FNativeWeaponMountDef M;
          M.Name=TEXT("Cannon"); M.bLoadout=false; M.Location=FVector(0.000000,0.000000,10.000000);
          M.Muzzle=FVector(0.000000,0.000000,90.000000);
          M.Muzzles.Add(FVector(0.000000,0.000000,90.000000));
          M.Types.Add(TEXT("Zolon Cannon"));
          Out.Add(M); }
        return;
    }
}
