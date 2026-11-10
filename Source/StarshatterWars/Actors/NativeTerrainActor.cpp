#include "NativeTerrainActor.h"
#include "ProceduralMeshComponent.h"
#include "Terrain.h"
#include "TerrainRegion.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Materials/Material.h"
#include "Components/DirectionalLightComponent.h"
ANativeTerrainActor::ANativeTerrainActor()
{
    Mesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainSurface"));SetRootComponent(Mesh);
    Mesh->bUseAsyncCooking=true;Mesh->bUseComplexAsSimpleCollision=true;
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial")));
}
void ANativeTerrainActor::SetSurfaceSky(bool bEnabled)
{
    if (bEnabled && !SurfaceAtmosphere)
    {
        // Mission geometry uses one Unreal unit per metre. The renderer
        // interprets 100,000 Unreal units as one kilometre.
        constexpr float JanekRenderRadiusKm = 54.0f;
        SurfaceAtmosphere = NewObject<USkyAtmosphereComponent>(this);
        SurfaceAtmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetTopAtComponentTransform;
        SurfaceAtmosphere->SetBottomRadius(JanekRenderRadiusKm);
        SurfaceAtmosphere->SetAtmosphereHeight(0.8f);
        SurfaceAtmosphere->RegisterComponent();

        constexpr TCHAR CloudPath[] = TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Cloud/MI_Cloud_WorldScape.MI_Cloud_WorldScape");
        auto* Material = LoadObject<UMaterialInterface>(nullptr, CloudPath);
        if (Material && Material->GetMaterial() && Material->GetMaterial()->MaterialDomain == MD_Volume)
        {
            SurfaceClouds = NewObject<UVolumetricCloudComponent>(this);
            SurfaceClouds->SetMaterial(Material);
            SurfaceClouds->SetPlanetRadius(JanekRenderRadiusKm);
            SurfaceClouds->SetLayerBottomAltitude(0.04f);
            SurfaceClouds->SetLayerHeight(0.10f);
            SurfaceClouds->RegisterComponent();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Janek surface WorldScape volumetric cloud material could not be loaded."));
        }

        SurfaceSun = NewObject<UDirectionalLightComponent>(this);
        SurfaceSun->SetAtmosphereSunLight(true);
        SurfaceSun->SetIntensity(4.0f);
        SurfaceSun->SetForwardShadingPriority(1);
        SurfaceSun->SetWorldRotation(FRotator(-45.0, -35.0, 0.0));
        SurfaceSun->RegisterComponent();
    }
    if (SurfaceAtmosphere)
        SurfaceAtmosphere->SetVisibility(bEnabled);
    if (SurfaceClouds)
        SurfaceClouds->SetVisibility(bEnabled);
    if (SurfaceSun)
        SurfaceSun->SetVisibility(bEnabled);
}

void ANativeTerrainActor::UpdateTerrain(Terrain* Model,const FVector& Observer,const FVector& RegionOrigin)
{
    if(!Model || !Model->HasHeightfield()){SetActorHiddenInGame(true);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);LastModel=nullptr;return;}
    SetActorHiddenInGame(false);Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    const double Step=FMath::Max(1.,Model->NativeSampleSpacing());
    const FIntPoint Cell(FMath::FloorToInt(Observer.X/(Step*32)),FMath::FloorToInt(Observer.Z/(Step*32)));
    const FVector Center(Cell.X*Step*32,0,Cell.Y*Step*32);
    SetActorLocation(RegionOrigin+FVector(Center.Z,Center.X,0));
    if (SurfaceAtmosphere && SurfaceAtmosphere->IsVisible())
    {
        // Anchor the atmosphere at the graded Janek Station site. Terrain
        // mesh cells follow the observer, so they cannot own this transform.
        const FVector Site = RegionOrigin + FVector(0.0, -25000.0, 0.0);
        if (!SurfaceAtmosphere->GetComponentLocation().Equals(Site))
            SurfaceAtmosphere->SetWorldLocation(Site);
    }
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
