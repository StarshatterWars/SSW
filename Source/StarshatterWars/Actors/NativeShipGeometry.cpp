#include "NativeShipGeometry.h"
#include "ShipActor.h"
#include "ShipWeaponMountComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "StaticMeshResources.h"
#include "SimModel.h"
#include "Surface.h"
#include "MeshCollision.h"
#include "Camera.h"

namespace
{
FTransform SimPose(const Camera& Frame)
{
    const FVector R=Frame.vrt(),U=Frame.vup(),F=Frame.vpn(),P=Frame.Pos();
    return FTransform(FMatrix(FPlane(R,0),FPlane(U,0),FPlane(F,0),FPlane(P,1)));
}
FTransform PartPose(const AShipActor& Actor,const UStaticMeshComponent& Part,const Camera& Frame)
{
    const FTransform ActorFrame(Actor.GetActorQuat(),Actor.GetActorLocation(),FVector::OneVector);
    const FTransform Relative=Part.GetComponentTransform().GetRelativeTransform(ActorFrame);
    const FTransform Permute(FMatrix(FPlane(0,0,1,0),FPlane(1,0,0,0),FPlane(0,1,0,0),FPlane(0,0,0,1)));
    return Relative*Permute*SimPose(Frame);
}
void AddFaces(SimModel& Model,const TArray<FVector>& Vertices,const TArray<int32>& Indices)
{
    constexpr int32 Batch=2048;
    for(int32 First=0;First<Indices.Num()/3;First+=Batch)
    {
        const int32 Count=FMath::Min(Batch,Indices.Num()/3-First);
        auto* SurfaceData=new Surface;SurfaceData->CreateVerts(Count*3);SurfaceData->CreatePolys(Count);
        auto* V=SurfaceData->GetVertexSet();auto* P=SurfaceData->GetPolys();
        for(int32 I=0;I<Count;++I)
        {
            P[I].nverts=3;P[I].vertex_set=V;
            for(int32 C=0;C<3;++C) {const int32 Index=I*3+C;V->loc[Index]=Vertices[Indices[(First+I)*3+C]];P[I].verts[C]=Index;}
        }
        Model.AddSurface(SurfaceData);
    }
}
void Collect(AShipActor const& Actor,TArray<TPair<UStaticMeshComponent*,Weapon*>>& Parts)
{
    if(Actor.HullMesh && Actor.HullMesh->GetStaticMesh()) Parts.Emplace(Actor.HullMesh,nullptr);
    TArray<UShipWeaponMountComponent*> Mounts;Actor.GetComponents(Mounts);
    for(auto* Mount:Mounts) if(Mount && !Mount->bLoadoutHardpoint && Mount->RuntimeWeapon)
    {
        auto Add=[&](AActor* Child)
        {
            if(!IsValid(Child)) return;
            TArray<UStaticMeshComponent*> Meshes;Child->GetComponents(Meshes,true);
            for(auto* Mesh:Meshes) if(Mesh && Mesh->GetStaticMesh()) Parts.Emplace(Mesh,Mount->RuntimeWeapon);
        };
        Add(Mount->GetChildActor());for(const auto& Barrel:Mount->BarrelComponents) if(Barrel) Add(Barrel->GetChildActor());
    }
}
}
TSharedPtr<SimModel> FNativeShipGeometry::Model(UStaticMesh* Mesh)
{
    if(!Mesh) return {};
    if(auto* Cached=Models.Find(Mesh)) return *Cached;
    TArray<FVector> Vertices;TArray<int32> Indices;
    if(UBodySetup* Body=Mesh->GetBodySetup())
    {
        Body->CreatePhysicsMeshes();
        for(const auto& Tri:Body->TriMeshGeometries) if(Tri)
        {
            const int32 Base=Vertices.Num();
            for(int32 I=0;I<int32(Tri->Particles().Size());++I) Vertices.Add(FVector(Tri->Particles().GetX(I)));
            const auto& Buffer=Tri->Elements();
            auto Add=[&](const auto& Faces){for(const auto& Face:Faces) for(int32 C=0;C<3;++C) Indices.Add(Base+int32(Face[C]));};
            if(Buffer.RequiresLargeIndices()) Add(Buffer.GetLargeIndexBuffer());else Add(Buffer.GetSmallIndexBuffer());
        }
    }
    // Editor/CPU-readable assets can lack a cooked triangle mesh during construction.
    if(Indices.IsEmpty())
    {
        Vertices.Reset();
        const FStaticMeshRenderData* Render=Mesh->GetRenderData();
        if(Render && Render->LODResources.Num()>0 && (GIsEditor || Mesh->bAllowCPUAccess))
        {
            const auto& LOD=Render->LODResources[0];
            for(uint32 I=0;I<LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();++I) Vertices.Add(FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(I)));
            for(int32 I=0;I<LOD.IndexBuffer.GetNumIndices();++I) Indices.Add(LOD.IndexBuffer.GetIndex(I));
        }
    }
    TSharedPtr<SimModel> Result;
    if(!Vertices.IsEmpty() && !Indices.IsEmpty()) {Result=MakeShared<SimModel>();AddFaces(*Result,Vertices,Indices);}
    else UE_LOG(LogTemp,Warning,TEXT("[NativeHullCollision] No cooked/CPU triangle geometry for %s; legacy fallback retained"),*Mesh->GetPathName());
    Models.Add(Mesh,Result);return Result;
}
int32 FNativeShipGeometry::Ray(const AShipActor& Actor,const Camera& Frame,const FVector& Origin,const FVector& Direction,double Length,FVector& Hit,Weapon*& HitWeapon)
{
    TArray<TPair<UStaticMeshComponent*,Weapon*>> Parts;Collect(Actor,Parts);
    bool HullAvailable=false,Found=false;double Best=Length;HitWeapon=nullptr;
    for(const auto& Part:Parts)
    {
        auto Data=Model(Part.Key->GetStaticMesh());if(!Data) continue;
        if(Part.Key==Actor.HullMesh) HullAvailable=true;
        FVector Candidate;Poly* Polygon=nullptr;
        if(MeshCollision::Ray(*Data,PartPose(Actor,*Part.Key,Frame),Origin,Direction,Best,true,Candidate,Polygon))
        {Best=FVector::Distance(Origin,Candidate);Hit=Candidate;HitWeapon=Part.Value;Found=true;}
    }
    return Found?1:HullAvailable?0:-1;
}
int32 FNativeShipGeometry::Overlap(const AShipActor& Actor,const Camera& Frame,FNativeShipGeometry& Other,const AShipActor& OtherActor,const Camera& OtherFrame)
{
    auto A=Actor.HullMesh?Model(Actor.HullMesh->GetStaticMesh()):TSharedPtr<SimModel>();
    auto B=OtherActor.HullMesh?Other.Model(OtherActor.HullMesh->GetStaticMesh()):TSharedPtr<SimModel>();
    if(!A || !B) return -1;
    return MeshCollision::Overlap(*A,PartPose(Actor,*Actor.HullMesh,Frame),*B,PartPose(OtherActor,*OtherActor.HullMesh,OtherFrame))?1:0;
}
