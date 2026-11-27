#include "TravelVortexActor.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#if PLATFORM_WINDOWS || PLATFORM_MAC
#include "VortexMeshGenerator.h"
#endif
ATravelVortexActor::ATravelVortexActor()
{
    PrimaryActorTick.bCanEverTick=false; SetActorEnableCollision(false);
    TunnelMesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Tunnel")); SetRootComponent(TunnelMesh);
    TunnelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);TunnelMesh->SetCastShadow(false);
    TunnelMesh->SetGenerateOverlapEvents(false);TunnelMesh->SetCanEverAffectNavigation(false);
#if PLATFORM_WINDOWS || PLATFORM_MAC
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Plasma(TEXT("/ProceduralVortexTunnel/Materials/M_PlasmaVortex.M_PlasmaVortex"));
    PackMaterial=Plasma.Object;
#endif
    SetActorHiddenInGame(true);
}
bool ATravelVortexActor::Configure(float Radius,float Length)
{
    if (bBuilt) return true;
    if (!FMath::IsFinite(Radius) || !FMath::IsFinite(Length) || Radius<=0 || Length<=0 || !PackMaterial) return false;
#if PLATFORM_WINDOWS || PLATFORM_MAC
    FVortexParameters Params;Params.VortexType=EVortexType::Normal;Params.Radius=Radius;Params.Height=Length;
    Params.RadialSegments=24;Params.HeightSegments=32;Params.WaveAmplitude=Radius*.025f;Params.OrganicIntensity=Radius*.012f;
    auto* Generator=NewObject<UVortexMeshGenerator>(this);
    TArray<FVector> Vertices,Normals;TArray<int32> Triangles;TArray<FVector2D> UV;TArray<FProcMeshTangent> Tangents;
    Generator->GenerateVortexVertices(Vertices,Triangles,UV,Params);
    if(Vertices.IsEmpty() || Triangles.Num()%3 || UV.Num()!=Vertices.Num())return false;
    // The traveler views the inside of the authored tunnel.
    for(int32 I=0;I<Triangles.Num();I+=3)Swap(Triangles[I+1],Triangles[I+2]);
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices,Triangles,UV,Normals,Tangents);
    TunnelMesh->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UV,TArray<FLinearColor>(),Tangents,false);
    Material=UMaterialInstanceDynamic::Create(PackMaterial,this);TunnelMesh->SetMaterial(0,Material);
    bBuilt=true;return true;
#else
    return false;
#endif
}
void ATravelVortexActor::SetEnvelope(bool bArrival,float Strength)
{
    const float Amount=FMath::IsFinite(Strength)?FMath::Clamp(Strength,0.f,1.f):0.f;
    SetActorHiddenInGame(!bBuilt || Amount<.01f);
    if(!Material)return;
    const FLinearColor Main=bArrival?FLinearColor(.025f,.3f,1):FLinearColor(1,.32f,.015f);
    const FLinearColor Accent=bArrival?FLinearColor(.025f,.65f,.38f):FLinearColor(1,.8f,.06f);
    Material->SetVectorParameterValue(TEXT("DefaultColor1"),Main);Material->SetVectorParameterValue(TEXT("DefaultColor2"),Accent);
    Material->SetVectorParameterValue(TEXT("NoiseColor1"),Main);
    Material->SetVectorParameterValue(TEXT("NoiseColor2"),Accent);
    Material->SetVectorParameterValue(TEXT("NoiseColor3"),Main*.35f);
    Material->SetScalarParameterValue(TEXT("EmissiveStrength"),Amount*2.f);
}
