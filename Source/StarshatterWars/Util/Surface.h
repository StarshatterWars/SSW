/*  Starshatter Wars
    Fractal Dev Studios
    Copyright(C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: Surface.h
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    Surface: a renderable mesh section (vertex set + polys + segments)
    OPCODE REMOVED: replaced with UE-native collision cache (verts/indices/bounds)
*/

#pragma once

#include "List.h"
#include "Polygon.h"
#include "Video.h"

// UE:
#include "Containers/Array.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/Box.h"
#include "GameStructs.h"
#include "Solid.h"

// Forward declarations:
class SimModel;
class Segment;

struct FSurfaceCollisionCache
{
    TArray<FVector> LocalVerts;
    TArray<int32>   Indices;      // Triangulated indices (triplets)
    FBox            LocalBounds;  // Bounds of LocalVerts
    bool            bValid = false;

    void Reset()
    {
        LocalVerts.Reset();
        Indices.Reset();
        LocalBounds.Init();
        bValid = false;
    }
};

class Surface
{
    friend class Solid;
    friend class SimModel;

public:
    static const char* TYPENAME() { return "Surface"; }

    enum { HIDDEN = 1, LOCKED = 2, SIMPLE = 4, MAX_VERTS = 64000, MAX_POLYS = 16000 };

    Surface();
    ~Surface();

    int            operator == (const Surface& s) const { return this == &s; }

    const char* Name()         const { return name; }
    int            GetNumVerts()     const { return vertex_set ? vertex_set->nverts : 0; }
    int            GetNumSegments()  const { return segments.size(); }
    int            GetNumPolys()     const { return npolys; }
    int            NumIndices()   const { return nindices; }
    bool           IsHidden()     const { return (state & HIDDEN) ? true : false; }
    bool           IsLocked()     const { return (state & LOCKED) ? true : false; }
    bool           IsSimplified() const { return (state & SIMPLE) ? true : false; }

    List<Segment>& GetSegments() { return segments; }

    // Vec3/Point -> FVector:
    const FVector& GetOffset() const { return offset; }

    // UE: orientation stored as FMatrix:
    const FMatrix& GetOrientation() const { return OrientationMatrix; }

    double          GetRadius() const { return radius; }
    VertexSet*      GetVertexSet() const { return vertex_set; }
    FVector*        GetVLoc() const { return vloc; }
    Poly*           GetPolys() const { return polys; }
    SimModel*       GetModel() const { return model; }

    void           SetName(const char* n);
    void           SetHidden(bool b);
    void           SetLocked(bool b);
    void           SetSimplified(bool b);

    void           CreateVerts(int nverts);
    void           CreatePolys(int npolys);
    void           AddIndices(int n) { nindices += n; }
    Poly* AddPolys(int npolys, int nverts);

    VideoPrivateData* GetVideoPrivateData() const { return video_data; }
    void              SetVideoPrivateData(VideoPrivateData* vpd) { video_data = vpd; }

    void           ScaleBy(double factor);

    void           BuildHull();
    void           InitializeCollisionHull();
    void           Normalize();
    void           SelectPolys(List<Poly>&, Material* mtl);
    void           SelectPolys(List<Poly>&, FVector loc);

    // UE-native collision cache (replaces OPCODE)
    void           BuildCollisionCache();
    const FSurfaceCollisionCache& GetCollisionCache() const { return Collision; }

    void           ComputeTangents();
    void           CalcGradients(Poly& p, FVector& tangent, FVector& binormal);

    void           Copy(Surface& s, SimModel* m);
    void           OptimizeMesh();
    void           ExplodeMesh();

private:
    char           name[Solid::NAMELEN];
    SimModel*      model;
    VertexSet*     vertex_set; // for rendering
    FVector*       vloc;       // for shadow hull
    float          radius;
    int            nhull;
    int            npolys;
    int            nindices;
    int            state;
    Poly* polys;
    List<Segment>  segments;

    // Point -> FVector:
    FVector        offset;

    // UE: store orientation in Unreal-native matrix:
    FMatrix        OrientationMatrix;

private:
    FSurfaceCollisionCache Collision;
    VideoPrivateData* video_data;
};
