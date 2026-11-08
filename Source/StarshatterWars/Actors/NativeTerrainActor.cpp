#include "NativeTerrainActor.h"
#include "ProceduralMeshComponent.h"
#include "Terrain.h"
#include "TerrainRegion.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
ANativeTerrainActor::ANativeTerrainActor()
{
    Mesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainSurface"));SetRootComponent(Mesh);
    Mesh->bUseAsyncCooking=true;Mesh->bUseComplexAsSimpleCollision=true;
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial")));
}
void ANativeTerrainActor::SetSurfaceSky(AActor* OrbitalPlanet)
{
    if (HiddenPlanet.Get() == OrbitalPlanet)
        return;

    if (AActor* Previous = HiddenPlanet.Get())
        Previous->SetActorHiddenInGame(bPlanetWasHidden);
    HiddenPlanet = OrbitalPlanet;
    if (OrbitalPlanet)
    {
        bPlanetWasHidden = OrbitalPlanet->IsHidden();
        OrbitalPlanet->SetActorHiddenInGame(true);
        if (!SurfaceAtmosphere)
        {
            SurfaceAtmosphere = NewObject<USkyAtmosphereComponent>(this);
            SurfaceAtmosphere->RegisterComponent();

            SurfaceSun = NewObject<UDirectionalLightComponent>(this);
            SurfaceSun->SetAtmosphereSunLight(true);
            SurfaceSun->SetIntensity(4.0f);
            SurfaceSun->SetForwardShadingPriority(1);
            SurfaceSun->SetWorldRotation(FRotator(-45.0, -35.0, 0.0));
            SurfaceSun->RegisterComponent();
        }
    }
    if (SurfaceAtmosphere)
        SurfaceAtmosphere->SetVisibility(OrbitalPlanet != nullptr);
    if (SurfaceSun)
        SurfaceSun->SetVisibility(OrbitalPlanet != nullptr);
}

void ANativeTerrainActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetSurfaceSky(nullptr);
    Super::EndPlay(EndPlayReason);
}

void ANativeTerrainActor::UpdateTerrain(Terrain* Model,const FVector& Observer,const FVector& RegionOrigin)
{
    if(!Model || !Model->HasHeightfield()){SetActorHiddenInGame(true);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);LastModel=nullptr;return;}
    SetActorHiddenInGame(false);Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    const double Step=FMath::Max(1.,Model->NativeSampleSpacing());
    const FIntPoint Cell(FMath::FloorToInt(Observer.X/(Step*32)),FMath::FloorToInt(Observer.Z/(Step*32)));
    const FVector Center(Cell.X*Step*32,0,Cell.Y*Step*32);
    SetActorLocation(RegionOrigin+FVector(Center.Z,Center.X,0));
    if(Model==LastModel&&Cell==LastCell)return;
    if (Model != LastModel)
    {
        const TerrainRegion* Region = Model->GetRegion();
        const TCHAR* MaterialPath = Region && FCStringAnsi::Stricmp(Region->GetName(), "Janek Station") == 0
            ? TEXT("/Game/Maps/Ground/M_JanekGround.M_JanekGround")
            : TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial");
        if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath))
            Mesh->SetMaterial(0, Material);
    }
    LastModel=Model;LastCell=Cell;
    constexpr int N=257;TArray<FVector> V,Normals;TArray<int32> Tri;TArray<FVector2D> UV;TArray<FProcMeshTangent> Tangents;
    V.Reserve(N*N);UV.Reserve(N*N);Tri.Reserve((N-1)*(N-1)*6);
    for(int Z=0;Z<N;++Z)for(int X=0;X<N;++X)
    {
        const double SX=Center.X+(X-(N-1)/2)*Step,SZ=Center.Z+(Z-(N-1)/2)*Step;
        V.Add(FVector(SZ-Center.Z,SX-Center.X,Model->Height(SX,SZ)));UV.Add(FVector2D(SZ/Step,SX/Step)*.01);
    }
    for(int Z=0;Z<N-1;++Z)for(int X=0;X<N-1;++X)
    {int A=Z*N+X;Tri.Append({A,A+1,A+N,A+1,A+N+1,A+N});}
    // The generic tangent utility performs costly overlap searches on large grids.
    // Grid neighbours provide normals and tangents in linear time.
    Normals.SetNum(V.Num());Tangents.SetNum(V.Num());
    for(int Z=0;Z<N;++Z)for(int X=0;X<N;++X)
    {
        const FVector DX=V[Z*N+FMath::Min(X+1,N-1)]-V[Z*N+FMath::Max(X-1,0)];
        const FVector DZ=V[FMath::Min(Z+1,N-1)*N+X]-V[FMath::Max(Z-1,0)*N+X];
        Normals[Z*N+X]=FVector::CrossProduct(DZ,DX).GetSafeNormal();Tangents[Z*N+X]=FProcMeshTangent(DZ.GetSafeNormal(),false);
    }
    Mesh->CreateMeshSection(0,V,Tri,Normals,UV,TArray<FColor>(),Tangents,true);
}
