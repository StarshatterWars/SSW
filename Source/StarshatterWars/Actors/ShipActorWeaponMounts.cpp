#include "ShipActor.h"

#include "Components/SceneComponent.h"
#include "ShipDesignRegistry.h"
#include "HardPoint.h"
#include "UObject/UObjectThreadContext.h"

namespace
{
    bool InWeaponMountConstructor()
    {
        return FUObjectThreadContext::Get().IsInConstructor > 0;
    }
}

void AShipActor::ConfigureWeaponHardpointsFromShipDef(const FString& ShipName)
{
    auto MakeScene = [this](FName Name, USceneComponent* Parent) -> USceneComponent*
    {
        USceneComponent* Comp = InWeaponMountConstructor()
            ? CreateDefaultSubobject<USceneComponent>(Name)
            : NewObject<USceneComponent>(this, Name);
        if (!Comp) return nullptr;
        Comp->SetupAttachment(Parent ? Parent : GetRootComponent());
        if (!InWeaponMountConstructor()) Comp->RegisterComponent();
        return Comp;
    };

    if (!WeaponHardpointRoot)
    {
        WeaponHardpointRoot = MakeScene(TEXT("WeaponHardpointRoot"), RootComponent);
    }

    if (!InWeaponMountConstructor())
    {
        for (TObjectPtr<USceneComponent>& Mount : NativeWeaponHardpoints)
        {
            if (Mount) Mount->DestroyComponent();
        }
    }
    NativeWeaponHardpoints.Reset();

    const ShipDesign* Design = ShipDesignRegistry::FindLegacy(TCHAR_TO_ANSI(*ShipName));
    if (Design)
    {
        for (int i = 0; i < Design->hardpoints.size(); ++i)
        {
            HardPoint* Hp = Design->hardpoints[i];
            if (!Hp) continue;

            FShipPointDef Point;
            Point.PointName = FName(*FString::Printf(TEXT("HP_%d"), i));
            Point.LocalOffset = Hp->MountLocation();
            WeaponMountPointDefs.Add(Point);
        }
    }

    for (int32 Index = 0; Index < WeaponMountPointDefs.Num(); ++Index)
    {
        const FShipPointDef& Def = WeaponMountPointDefs[Index];
        const FName Name = Def.PointName.IsNone()
            ? FName(*FString::Printf(TEXT("Hardpoint_%d"), Index))
            : Def.PointName;

        USceneComponent* Mount = MakeScene(Name, WeaponHardpointRoot);
        if (!Mount) continue;

        Mount->SetRelativeLocation(Def.LocalOffset);
        Mount->SetRelativeRotation(Def.LocalRotation);
        NativeWeaponHardpoints.Add(Mount);
    }
}
