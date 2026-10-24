#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MeshCollision.h"
#include "Solid.h"
#include "Surface.h"
#include "SimModel.h"
#include "Math/RotationMatrix.h"
namespace
{
class FAuditSolid : public Solid
{
public:
    void BoundsRadius(float R){radius=R;}
};
Surface* AddTriangle(SimModel& Model,const FVector& A,const FVector& B,const FVector& C)
{
    Surface* S=new Surface;S->CreateVerts(3);S->CreatePolys(1);
    auto* V=S->GetVertexSet();V->loc[0]=A;V->loc[1]=B;V->loc[2]=C;
    Poly& P=S->GetPolys()[0];P.nverts=3;P.verts[0]=0;P.verts[1]=1;P.verts[2]=2;P.vertex_set=V;
    Model.AddSurface(S);return S;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshCollisionTest,"Starshatter.Audit.F03.MeshCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMeshCollisionTest::RunTest(const FString& Parameters)
{
    SimModel Model;
    Surface* Near=AddTriangle(Model,FVector(-2,-2,2),FVector(2,-2,2),FVector(0,2,2));
    AddTriangle(Model,FVector(-2,-2,5),FVector(2,-2,5),FVector(0,2,5));
    // Force BVH subdivision with faces outside the test ray.
    for(int32 I=0;I<20;++I)AddTriangle(Model,FVector(20+I,0,1),FVector(20+I,2,1),FVector(21+I,0,1));
    FAuditSolid Solid;Solid.UseModel(&Model);Solid.BoundsRadius(100);
    FVector Hit(999);
    TestTrue(TEXT("Ray intersects nearest actual face"),Solid.CheckRayIntersection(FVector::ZeroVector,FVector(0,0,2),10,Hit)!=0);
    TestTrue(TEXT("Hit is mesh entry, not sphere center"),Hit.Equals(FVector(0,0,2),1.e-6));
    TestTrue(TEXT("Hit retains polygon identity"),Solid.GetIntersectionPoly()==Near->GetPolys());
    TestEqual(TEXT("Sphere overlap does not fill mesh openings"),Solid.CheckRayIntersection(FVector(8,8,0),FVector::UpVector,10,Hit),0);
    TestEqual(TEXT("Short ray misses"),Solid.CheckRayIntersection(FVector::ZeroVector,FVector::UpVector,1,Hit),0);
    TestEqual(TEXT("Backwards ray misses"),Solid.CheckRayIntersection(FVector::ZeroVector,-FVector::UpVector,10,Hit),0);
    TestEqual(TEXT("Zero direction misses"),Solid.CheckRayIntersection(FVector::ZeroVector,FVector::ZeroVector,10,Hit),0);
    TestTrue(TEXT("Back face is also collidable"),Solid.CheckRayIntersection(FVector(0,0,8),-FVector::UpVector,10,Hit)!=0 && Hit.Equals(FVector(0,0,5)));
    Material Transparent;Transparent.blend=Material::MTL_TRANSLUCENT;Near->GetPolys()[0].material=&Transparent;
    Solid.CheckRayIntersection(FVector::ZeroVector,FVector::UpVector,10,Hit,false);
    TestTrue(TEXT("Transparent filter reaches next solid face"),Hit.Equals(FVector(0,0,5)));
    Near->GetPolys()[0].material=nullptr;
    const FMatrix Rotation=FRotationMatrix(FRotator(30,70,15));
    const FVector Translation(125,-30,42);Solid.SetOrientation(Rotation);Solid.MoveTo(Translation);
    const FVector Direction=Rotation.TransformVector(FVector::UpVector);
    TestTrue(TEXT("Rotated translated mesh hit"),Solid.CheckRayIntersection(Translation,Direction,10,Hit)!=0);
    TestTrue(TEXT("World hit follows rigid body transform"),Hit.Equals(Translation+Direction*2,1.e-6));
    Solid.SetOrientation(FMatrix::Identity);Solid.MoveTo(FVector::ZeroVector);
    Near->ScaleBy(2);Solid.CheckRayIntersection(FVector::ZeroVector,FVector::UpVector,10,Hit);
    TestTrue(TEXT("Scale invalidates triangle cache"),Hit.Equals(FVector(0,0,4)));
    for(int32 I=0;I<3;++I)Near->GetVertexSet()->loc[I].Z=1;
    Near->BuildHull();Solid.CheckRayIntersection(FVector::ZeroVector,FVector::UpVector,10,Hit);
    TestTrue(TEXT("Hull rebuild refreshes directly edited vertices"),Hit.Equals(FVector(0,0,1)));

    SimModel AModel,BModel;
    AddTriangle(AModel,FVector(0,0,0),FVector(4,0,0),FVector(0,4,0));
    AddTriangle(BModel,FVector(0,0,0),FVector(4,0,0),FVector(0,4,0));
    FAuditSolid A,B;A.UseModel(&AModel);B.UseModel(&BModel);A.BoundsRadius(100);B.BoundsRadius(100);
    B.MoveTo(FVector(0,0,2));TestEqual(TEXT("Overlapping bounds with separated hulls do not collide"),A.CollidesWith(B),0);
    B.MoveTo(FVector(3,3,0));TestEqual(TEXT("Coplanar disjoint triangles with overlapping boxes do not collide"),A.CollidesWith(B),0);
    B.MoveTo(FVector(1,1,0));TestTrue(TEXT("Coplanar intersecting triangles collide"),A.CollidesWith(B)!=0);
    B.SetOrientation(FRotationMatrix(FRotator(90,0,0)));B.MoveTo(FVector(1,1,-1));
    TestTrue(TEXT("Crossing triangles collide"),A.CollidesWith(B)!=0);
    Solid::EnableCollision(false);TestEqual(TEXT("Global collision toggle preserved"),A.CollidesWith(B),0);Solid::EnableCollision(true);

    FVector SphereHit;
    TestTrue(TEXT("Easy AI sphere fallback has finite surface impact"),MeshCollision::RaySphere(FVector(0,0,-10),FVector::UpVector,20,FVector::ZeroVector,2,SphereHit));
    TestTrue(TEXT("Sphere impact is entry point"),SphereHit.Equals(FVector(0,0,-2)));
    TestFalse(TEXT("Sphere beyond segment misses"),MeshCollision::RaySphere(FVector(0,0,-10),FVector::UpVector,7,FVector::ZeroVector,2,SphereHit));
    TestTrue(TEXT("Sphere containing ray origin returns origin"),MeshCollision::RaySphere(FVector::ZeroVector,FVector::UpVector,2,FVector::ZeroVector,2,SphereHit)&&SphereHit.IsZero());
    return true;
}
#endif
