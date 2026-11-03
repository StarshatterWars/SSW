#pragma once
#include "TurretBaseActor.h"
namespace TurretBaseCatalog
{
    TSubclassOf<ATurretBaseActor> NativeClass(const FString& Model);
    TSubclassOf<ATurretBaseActor> VisualClass(const FString& Model);
    void Definition(FName WeaponType,FString& Model,float& Scale);
}
