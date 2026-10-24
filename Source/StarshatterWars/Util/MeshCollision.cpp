#include "MeshCollision.h"
#include "Surface.h"
#include "SimModel.h"
#include "Algo/Sort.h"

namespace
{
FBox TriangleBounds(const FSurfaceCollisionCache& C, int32 T)
{
    FBox Box(ForceInit);
    for (int32 I=0; I<3; ++I) Box += C.LocalVerts[C.Indices[3*T+I]];
    return Box;
}
int32 BuildNode(FSurfaceCollisionCache& C, int32 First, int32 Count)
{
    const int32 Id = C.Nodes.AddDefaulted();
    FBox Box(ForceInit);
    for (int32 I=First; I<First+Count; ++I) Box += TriangleBounds(C,C.TriangleOrder[I]);
    C.Nodes[Id].Bounds=Box;
    if (Count<=8) { C.Nodes[Id].First=First; C.Nodes[Id].Count=Count; return Id; }
    const FVector Size=Box.GetSize();
    const int32 Axis=Size.X>Size.Y ? (Size.X>Size.Z ? 0:2) : (Size.Y>Size.Z ? 1:2);
    Algo::Sort(MakeArrayView(C.TriangleOrder.GetData()+First,Count),[&C,Axis](int32 A,int32 B)
    { return TriangleBounds(C,A).GetCenter()[Axis]<TriangleBounds(C,B).GetCenter()[Axis]; });
    const int32 Half=Count/2;
    const int32 Left=BuildNode(C,First,Half), Right=BuildNode(C,First+Half,Count-Half);
    C.Nodes[Id].Left=Left; C.Nodes[Id].Right=Right;
    return Id;
}
bool RayBox(const FBox& Box,const FVector& O,const FVector& D,double Limit)
{
    double Near=0,Far=Limit;
    for(int32 I=0;I<3;++I)
    {
        if(FMath::Abs(D[I])<1.e-12) { if(O[I]<Box.Min[I] || O[I]>Box.Max[I])return false; }
        else
        {
            double A=(Box.Min[I]-O[I])/D[I],B=(Box.Max[I]-O[I])/D[I];
            if(A>B)Swap(A,B);
            Near=FMath::Max(Near,A);Far=FMath::Min(Far,B);
            if(Near>Far)return false;
        }
    }
    return true;
}
bool RayTriangle(const FVector& O,const FVector& D,const FVector& A,const FVector& B,const FVector& C,double Limit,double& Distance)
{
    const FVector E=B-A,F=C-A,P=FVector::CrossProduct(D,F);
    const double Det=FVector::DotProduct(E,P);
    if(FMath::Abs(Det)<1.e-12)return false;
    const FVector T=O-A,Q=FVector::CrossProduct(T,E);
    const double U=FVector::DotProduct(T,P)/Det,V=FVector::DotProduct(D,Q)/Det;
    Distance=FVector::DotProduct(F,Q)/Det;
    return U>=-1.e-9 && V>=-1.e-9 && U+V<=1.+1.e-9 && Distance>=0 && Distance<=Limit;
}
bool Separated(const FVector* A,const FVector* B,const FVector& RawAxis)
{
    if(RawAxis.SizeSquared()<1.e-20)return false;
    const FVector Axis=RawAxis.GetSafeNormal();
    double AMin=FVector::DotProduct(A[0],Axis),AMax=AMin;
    double BMin=FVector::DotProduct(B[0],Axis),BMax=BMin;
    for(int32 I=1;I<3;++I)
    {
        const double DA=FVector::DotProduct(A[I],Axis),DB=FVector::DotProduct(B[I],Axis);
        AMin=FMath::Min(AMin,DA);AMax=FMath::Max(AMax,DA);
        BMin=FMath::Min(BMin,DB);BMax=FMath::Max(BMax,DB);
    }
    return AMax<BMin-1.e-6 || BMax<AMin-1.e-6;
}
bool TrianglesOverlap(const FVector* A,const FVector* B)
{
    FVector EA[3],EB[3];
    for(int32 I=0;I<3;++I){EA[I]=A[(I+1)%3]-A[I];EB[I]=B[(I+1)%3]-B[I];}
    const FVector NA=FVector::CrossProduct(EA[0],EA[1]),NB=FVector::CrossProduct(EB[0],EB[1]);
    if(Separated(A,B,NA)||Separated(A,B,NB))return false;
    for(int32 I=0;I<3;++I)
    {
        // In-plane axes are required for coplanar triangles.
        if(Separated(A,B,FVector::CrossProduct(NA,EA[I])) || Separated(A,B,FVector::CrossProduct(NB,EB[I])))return false;
        for(int32 J=0;J<3;++J)if(Separated(A,B,FVector::CrossProduct(EA[I],EB[J])))return false;
    }
    return true;
}
void WorldTriangle(const FSurfaceCollisionCache& C,int32 T,const FTransform& Pose,FVector* V)
{
    for(int32 I=0;I<3;++I)V[I]=Pose.TransformPosition(C.LocalVerts[C.Indices[3*T+I]]);
}
bool NodeOverlap(const FSurfaceCollisionCache& A,int32 AI,const FTransform& AP,const FSurfaceCollisionCache& B,int32 BI,const FTransform& BP)
{
    const auto& AN=A.Nodes[AI];const auto& BN=B.Nodes[BI];
    if(!AN.Bounds.TransformBy(AP).Intersect(BN.Bounds.TransformBy(BP)))return false;
    if(AN.Count && BN.Count)
    {
        for(int32 I=AN.First;I<AN.First+AN.Count;++I)
        {
            FVector AV[3];WorldTriangle(A,A.TriangleOrder[I],AP,AV);
            for(int32 J=BN.First;J<BN.First+BN.Count;++J)
            {
                FVector BV[3];WorldTriangle(B,B.TriangleOrder[J],BP,BV);
                if(TrianglesOverlap(AV,BV))return true;
            }
        }
        return false;
    }
    if(!AN.Count)return NodeOverlap(A,AN.Left,AP,B,BI,BP)||NodeOverlap(A,AN.Right,AP,B,BI,BP);
    return NodeOverlap(A,AI,AP,B,BN.Left,BP)||NodeOverlap(A,AI,AP,B,BN.Right,BP);
}
}

void Surface::BuildCollisionCache()
{
    if(Collision.bValid)return;
    Collision.Reset();
    if(vertex_set && vertex_set->loc && polys)
    {
        Collision.LocalVerts.Append(vertex_set->loc,vertex_set->nverts);
        for(int32 P=0;P<npolys;++P)
        {
            const Poly& Face=polys[P];
            if(Face.nverts<3 || Face.nverts>Poly::MAX_VERTS)continue;
            for(int32 I=1;I+1<Face.nverts;++I)
            {
                const int32 A=Face.verts[0],B=Face.verts[I],C=Face.verts[I+1];
                if(!Collision.LocalVerts.IsValidIndex(A)||!Collision.LocalVerts.IsValidIndex(B)||!Collision.LocalVerts.IsValidIndex(C))continue;
                const FVector VA=Collision.LocalVerts[A],VB=Collision.LocalVerts[B],VC=Collision.LocalVerts[C];
                if(VA.ContainsNaN()||VB.ContainsNaN()||VC.ContainsNaN()||FVector::CrossProduct(VB-VA,VC-VA).SizeSquared()<1.e-20)continue;
                Collision.Indices.Append({A,B,C});Collision.PolyIds.Add(P);
                Collision.LocalBounds+=VA;Collision.LocalBounds+=VB;Collision.LocalBounds+=VC;
            }
        }
    }
    for(int32 I=0;I<Collision.PolyIds.Num();++I)Collision.TriangleOrder.Add(I);
    if(!Collision.TriangleOrder.IsEmpty())BuildNode(Collision,0,Collision.TriangleOrder.Num());
    Collision.bValid=true;
}

bool MeshCollision::RaySphere(const FVector& O,const FVector& D,double Length,const FVector& Center,double Radius,FVector& Hit)
{
    if(Length<0 || Radius<0 || !FMath::IsFinite(Length))return false;
    const FVector Offset=O-Center;
    const double C=Offset.SizeSquared()-Radius*Radius;
    if(C<=0){Hit=O;return true;}
    const double B=FVector::DotProduct(Offset,D),Disc=B*B-C;
    if(Disc<0)return false;
    const double T=-B-FMath::Sqrt(Disc);
    if(T<0 || T>Length)return false;
    Hit=O+D*T;return true;
}
bool MeshCollision::HasGeometry(SimModel& Model)
{
    ListIter<Surface> It=Model.GetSurfaces();
    while(++It){Surface* S=It.value();if(S){S->BuildCollisionCache();if(!S->GetCollisionCache().Nodes.IsEmpty())return true;}}
    return false;
}
bool MeshCollision::Ray(SimModel& Model,const FTransform& Pose,const FVector& Origin,const FVector& Direction,double Length,bool Translucent,FVector& Hit,Poly*& HitPoly)
{
    const FVector O=Pose.InverseTransformPosition(Origin),D=Pose.InverseTransformVectorNoScale(Direction);
    double Closest=Length;bool Found=false;HitPoly=nullptr;
    ListIter<Surface> It=Model.GetSurfaces();
    while(++It)
    {
        Surface* S=It.value();if(!S)continue;S->BuildCollisionCache();const auto& C=S->GetCollisionCache();
        if(C.Nodes.IsEmpty())continue;
        TArray<int32,TInlineAllocator<64>> Stack;Stack.Add(0);
        while(!Stack.IsEmpty())
        {
            const auto& Node=C.Nodes[Stack.Pop(EAllowShrinking::No)];
            if(!RayBox(Node.Bounds,O,D,Closest))continue;
            if(!Node.Count){Stack.Add(Node.Left);Stack.Add(Node.Right);continue;}
            for(int32 I=Node.First;I<Node.First+Node.Count;++I)
            {
                const int32 T=C.TriangleOrder[I];Poly* P=&S->GetPolys()[C.PolyIds[T]];
                if(!Translucent && P->material && !P->material->IsSolid())continue;
                double Distance=0;
                if(RayTriangle(O,D,C.LocalVerts[C.Indices[3*T]],C.LocalVerts[C.Indices[3*T+1]],C.LocalVerts[C.Indices[3*T+2]],Closest,Distance))
                {Closest=Distance;HitPoly=P;Found=true;}
            }
        }
    }
    if(Found)Hit=Origin+Direction*Closest;
    return Found;
}
bool MeshCollision::Overlap(SimModel& A,const FTransform& AP,SimModel& B,const FTransform& BP)
{
    ListIter<Surface> AI=A.GetSurfaces();
    while(++AI)
    {
        Surface* AS=AI.value();if(!AS)continue;AS->BuildCollisionCache();const auto& AC=AS->GetCollisionCache();if(AC.Nodes.IsEmpty())continue;
        ListIter<Surface> BI=B.GetSurfaces();
        while(++BI)
        {
            Surface* BS=BI.value();if(!BS)continue;BS->BuildCollisionCache();const auto& BC=BS->GetCollisionCache();
            if(!BC.Nodes.IsEmpty() && NodeOverlap(AC,0,AP,BC,0,BP))return true;
        }
    }
    return false;
}
