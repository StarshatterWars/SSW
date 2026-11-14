#include "ProductionActor.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AProductionActor::AProductionActor()
{
    static const ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Game/Models/Xfer/Xfer.Xfer"));
    if (Mesh.Succeeded())
        SetHullMesh(Mesh.Object);
}
