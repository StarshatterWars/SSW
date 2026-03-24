/*  Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: Surface.cpp
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    Surface mesh container + mesh ops (UE-native, OPCODE removed)
*/

#include "Surface.h"
#include "SimModel.h"
#include "Segment.h"

#include "CoreMinimal.h"
#include <cstring>
#include <cstdlib>
#include <cmath>

// --------------------------------------------------------------------
// UE-native collision cache (no header changes required)
// --------------------------------------------------------------------

namespace
{
    struct FSurfaceCollisionCache
    {
        bool  bValid = false;
        FBox  LocalBounds = FBox(EForceInit::ForceInit);
        int32 NumTris = 0;
    };

    // One cache per Surface* without touching the class layout:
    static TMap<const Surface*, FSurfaceCollisionCache> GSurfaceCache;

    static void InvalidateSurfaceCache(const Surface* S)
    {
        if (!S) return;
        if (FSurfaceCollisionCache* Cache = GSurfaceCache.Find(S))
        {
            Cache->bValid = false;
            Cache->LocalBounds = FBox(EForceInit::ForceInit);
            Cache->NumTris = 0;
        }
    }

    static void BuildSurfaceCache(const Surface* S)
    {
        if (!S) return;

        FSurfaceCollisionCache& Cache = GSurfaceCache.FindOrAdd(S);
        Cache.bValid = false;
        Cache.LocalBounds = FBox(EForceInit::ForceInit);
        Cache.NumTris = 0;

        VertexSet* V = S->GetVertexSet();
        Poly* P = S->GetPolys();
        const int  NP = S->GetNumPolys();

        if (!V || !P || NP < 1 || V->nverts < 1)
            return;

        // Local bounds from vertex locations:
        for (int i = 0; i < V->nverts; ++i)
            Cache.LocalBounds += V->loc[i];

        // Triangle count estimate:
        int32 Tris = 0;
        for (int i = 0; i < NP; ++i)
        {
            const int nv = P[i].nverts;
            if (nv == 3) Tris += 1;
            else if (nv == 4) Tris += 2;
            else if (nv > 4)  Tris += (nv - 2); // fan triangulation
        }

        Cache.NumTris = Tris;
        Cache.bValid = true;
    }

    // Mesh optimization epsilon (legacy-ish)
    static const double SELECT_EPSILON = 0.05;
    static const double SELECT_TEXTURE = 0.0001;

    static bool MatchVerts(VertexSet* vset, int i, int j)
    {
        if (!vset) return false;

        const FVector& vl1 = vset->loc[i];
        const FVector& vn1 = vset->nrm[i];
        const float    tu1 = vset->tu[i];
        const float    tv1 = vset->tv[i];

        const FVector& vl2 = vset->loc[j];
        const FVector& vn2 = vset->nrm[j];
        const float    tu2 = vset->tu[j];
        const float    tv2 = vset->tv[j];

        double d = 0.0;

        d = std::fabs(vl1.X - vl2.X); if (d > SELECT_EPSILON) return false;
        d = std::fabs(vl1.Y - vl2.Y); if (d > SELECT_EPSILON) return false;
        d = std::fabs(vl1.Z - vl2.Z); if (d > SELECT_EPSILON) return false;

        d = std::fabs(vn1.X - vn2.X); if (d > SELECT_EPSILON) return false;
        d = std::fabs(vn1.Y - vn2.Y); if (d > SELECT_EPSILON) return false;
        d = std::fabs(vn1.Z - vn2.Z); if (d > SELECT_EPSILON) return false;

        d = std::fabs((double)tu1 - (double)tu2); if (d > SELECT_TEXTURE) return false;
        d = std::fabs((double)tv1 - (double)tv2); if (d > SELECT_TEXTURE) return false;

        return true;
    }
}

// --------------------------------------------------------------------
// Surface
// --------------------------------------------------------------------

Surface::Surface()
    : model(nullptr)
    , vertex_set(nullptr)
    , vloc(nullptr)
    , radius(0.0f)
    , nhull(0)
    , npolys(0)
    , nindices(0)
    , state(0)
    , polys(nullptr)
    , offset(FVector::ZeroVector)
    , OrientationMatrix(FMatrix::Identity)
    , video_data(nullptr)
{
    memset(name, 0, sizeof(name));
}

Surface::~Surface()
{
    segments.destroy();

    delete video_data;
    video_data = nullptr;

    delete vertex_set;
    vertex_set = nullptr;

    delete[] vloc;
    vloc = nullptr;

    delete[] polys;
    polys = nullptr;

    InvalidateSurfaceCache(this);
    GSurfaceCache.Remove(this);
}

void Surface::SetName(const char* n)
{
    const int len = (int)sizeof(name);
    memset(name, 0, len);
    strncpy_s(name, n ? n : "", len - 1);
}

void Surface::SetHidden(bool b)
{
    if (b) state |= HIDDEN;
    else   state &= ~HIDDEN;
}

void Surface::SetLocked(bool b)
{
    if (b) state |= LOCKED;
    else   state &= ~LOCKED;
}

void Surface::SetSimplified(bool b)
{
    if (b) state |= SIMPLE;
    else   state &= ~SIMPLE;
}

void Surface::CreateVerts(int nverts)
{
    if (!vertex_set && !vloc && nverts > 0)
    {
        vertex_set = new VertexSet(nverts);
        vloc = new FVector[nverts];

        // Safe init:
        memset(vloc, 0, sizeof(FVector) * nverts);
    }

    InvalidateSurfaceCache(this);
}

void Surface::CreatePolys(int np)
{
    if (!polys && npolys == 0 && np > 0)
    {
        npolys = np;
        polys = new Poly[npolys];
        memset(polys, 0, sizeof(Poly) * npolys);
    }

    InvalidateSurfaceCache(this);
}

Poly* Surface::AddPolys(int np, int nv)
{
    if (!polys || !vertex_set)
        return nullptr;

    if (np <= 0 || nv <= 0)
        return nullptr;

    if (np + npolys >= MAX_POLYS)
        return nullptr;

    if (nv + vertex_set->nverts >= MAX_VERTS)
        return nullptr;

    const int oldVerts = vertex_set->nverts;
    const int oldPolys = npolys;

    // Resize vertex set (keep old content):
    vertex_set->Resize(oldVerts + nv, true);

    // Recreate hull buffer to match new vertex count:
    delete[] vloc;
    vloc = new FVector[vertex_set->nverts];
    memset(vloc, 0, sizeof(FVector) * vertex_set->nverts);

    // Expand poly array:
    Poly* newPolySet = new Poly[oldPolys + np];
    memcpy(newPolySet, polys, sizeof(Poly) * oldPolys);
    memset(newPolySet + oldPolys, 0, sizeof(Poly) * np);

    delete[] polys;
    polys = newPolySet;
    npolys = oldPolys + np;

    // Ensure new polys point at this vset:
    for (int i = 0; i < np; ++i)
        polys[oldPolys + i].vertex_set = vertex_set;

    InvalidateSurfaceCache(this);
    return polys + oldPolys;
}

void Surface::ScaleBy(double factor)
{
    offset *= (float)factor;

    if (vertex_set && vertex_set->nverts > 0)
    {
        for (int i = 0; i < vertex_set->nverts; ++i)
            vertex_set->loc[i] *= (float)factor;
    }

    radius *= (float)factor;

    InvalidateSurfaceCache(this);
}

void Surface::BuildHull()
{
    if (npolys < 1 || !vertex_set || vertex_set->nverts < 1)
        return;

    // Ensure vloc exists and is large enough for worst case (vertex count):
    if (!vloc)
    {
        vloc = new FVector[vertex_set->nverts];
        memset(vloc, 0, sizeof(FVector) * vertex_set->nverts);
    }

    nhull = 0;
    radius = 0.0f;

    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;

        for (int n = 0; n < p->nverts; ++n)
        {
            const WORD v = p->verts[n];
            WORD h = 0;

            const FVector& vl = vertex_set->loc[v];
            const float dist = vl.Size();
            if (dist > radius) radius = dist;

            for (h = 0; h < nhull; ++h)
            {
                const FVector& hl = vloc[h];

                double dx = vl.X - hl.X; if (dx < -SELECT_EPSILON || dx > SELECT_EPSILON) continue;
                double dy = vl.Y - hl.Y; if (dy < -SELECT_EPSILON || dy > SELECT_EPSILON) continue;
                double dz = vl.Z - hl.Z; if (dz < -SELECT_EPSILON || dz > SELECT_EPSILON) continue;

                // match
                break;
            }

            if (h >= nhull)
            {
                vloc[h] = vl;
                nhull = h + 1;
            }

            p->vlocs[n] = h;
        }
    }

    InitializeCollisionHull(); // UE cache
}

void Surface::InitializeCollisionHull()
{
    // UE-native cache (AABB + tri count), no OPCODE:
    BuildSurfaceCache(this);
}

void Surface::Normalize()
{
    if (npolys < 1 || !vertex_set || vertex_set->nverts < 1)
        return;

    // STEP ONE: init planes per poly
    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;

        if (p->nverts >= 3)
        {
            p->plane = Plane(
                vertex_set->loc[p->verts[0]],
                vertex_set->loc[p->verts[2]],
                vertex_set->loc[p->verts[1]]
            );
        }
    }

    // STEP TWO: average adjacent planes per vertex
    List<Poly> faces;

    for (int v = 0; v < vertex_set->nverts; ++v)
    {
        faces.clear();
        SelectPolys(faces, vertex_set->loc[v]);

        if (faces.size())
        {
            FVector nrm(0, 0, 0);

            for (int i = 0; i < faces.size(); ++i)
                nrm += faces[i]->plane.normal;

            nrm.Normalize();
            vertex_set->nrm[v] = nrm;
        }
        else if (vertex_set->loc[v].Size() > 0.0f)
        {
            FVector nrm = vertex_set->loc[v];
            nrm.Normalize();
            vertex_set->nrm[v] = nrm;
        }
        else
        {
            vertex_set->nrm[v] = FVector(0, 1, 0);
        }
    }

    // STEP THREE: blend in flatness per poly
    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;

        for (int n = 0; n < p->nverts; ++n)
        {
            const int v = p->verts[n];

            vertex_set->nrm[v] =
                vertex_set->nrm[v] * (1.0f - p->flatness) +
                p->plane.normal * (p->flatness);
        }
    }

    InvalidateSurfaceCache(this);
}

void Surface::SelectPolys(List<Poly>& selection, FVector locIn)
{
    if (!polys || !vertex_set)
        return;

    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;

        for (int n = 0; n < p->nverts; ++n)
        {
            const int v = p->verts[n];
            const FVector& vl = vertex_set->loc[v];

            double dx = vl.X - locIn.X; if (dx < -SELECT_EPSILON || dx > SELECT_EPSILON) continue;
            double dy = vl.Y - locIn.Y; if (dy < -SELECT_EPSILON || dy > SELECT_EPSILON) continue;
            double dz = vl.Z - locIn.Z; if (dz < -SELECT_EPSILON || dz > SELECT_EPSILON) continue;

            selection.append(p);
            break;
        }
    }
}

void Surface::SelectPolys(List<Poly>& selection, Material* mtl)
{
    if (!polys)
        return;

    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;
        if (p->material == mtl)
            selection.append(p);
    }
}

void Surface::ComputeTangents()
{
    if (!vertex_set || vertex_set->nverts < 1)
        return;

    if (vertex_set->tangent) // already created
        return;

    vertex_set->CreateTangents();

    FVector tangent(0, 0, 0);
    FVector binormal(0, 0, 0);

    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;
        if (!p || p->nverts < 3) continue;

        CalcGradients(*p, tangent, binormal);

        for (int n = 0; n < p->nverts; ++n)
        {
            const int v = p->verts[n];
            vertex_set->tangent[v] = tangent;
            vertex_set->binormal[v] = binormal;
        }
    }
}

void Surface::CalcGradients(Poly& p, FVector& tangent, FVector& binormal)
{
    VertexSet* vset = p.vertex_set;
    if (!vset || p.nverts < 3)
        return;

    const FVector P = vset->loc[p.verts[1]] - vset->loc[p.verts[0]];
    const FVector Q = vset->loc[p.verts[2]] - vset->loc[p.verts[0]];

    const float s1 = vset->tu[p.verts[1]] - vset->tu[p.verts[0]];
    const float t1 = vset->tv[p.verts[1]] - vset->tv[p.verts[0]];
    const float s2 = vset->tu[p.verts[2]] - vset->tu[p.verts[0]];
    const float t2 = vset->tv[p.verts[2]] - vset->tv[p.verts[0]];

    float inv = 1.0f;
    const float denom = s1 * t2 - s2 * t1;
    if (std::fabs(denom) > 0.0001f)
        inv = 1.0f / denom;

    tangent.X = (t2 * P.X - t1 * Q.X) * inv;
    tangent.Y = (t2 * P.Y - t1 * Q.Y) * inv;
    tangent.Z = (t2 * P.Z - t1 * Q.Z) * inv;
    tangent.Normalize();

    binormal.X = (s1 * Q.X - s2 * P.X) * inv;
    binormal.Y = (s1 * Q.Y - s2 * P.Y) * inv;
    binormal.Z = (s1 * Q.Z - s2 * P.Z) * inv;
    binormal.Normalize();
}

void Surface::ExplodeMesh()
{
    if (!vertex_set || vertex_set->nverts < 3 || !polys || npolys < 1)
        return;

    // Count max verts (sum of poly vertex counts)
    int newVerts = 0;
    for (int i = 0; i < npolys; ++i)
        newVerts += polys[i].nverts;

    VertexSet* newV = new VertexSet(newVerts);
    int v = 0;

    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;
        p->vertex_set = newV;

        for (int j = 0; j < p->nverts; ++j)
        {
            const int src = p->verts[j];

            newV->loc[v] = vertex_set->loc[src];
            newV->nrm[v] = vertex_set->nrm[src];
            newV->tu[v] = vertex_set->tu[src];
            newV->tv[v] = vertex_set->tv[src];
            newV->rw[v] = vertex_set->rw[src];
            newV->diffuse[v] = vertex_set->diffuse[src];
            newV->specular[v] = vertex_set->specular[src];

            p->verts[j] = (WORD)v;
            ++v;
        }
    }

    delete vertex_set;
    vertex_set = newV;

    delete[] vloc;
    vloc = new FVector[vertex_set->nverts];
    memset(vloc, 0, sizeof(FVector) * vertex_set->nverts);

    ComputeTangents();
    BuildHull();

    InvalidateSurfaceCache(this);
}

void Surface::OptimizeMesh()
{
    if (!vertex_set || vertex_set->nverts < 3 || !polys || npolys < 1)
        return;

    const int nverts = vertex_set->nverts;

    // maps
    BYTE* usedMap = new BYTE[nverts];
    WORD* dstMap = new WORD[nverts];

    memset(usedMap, 0, nverts * sizeof(BYTE));
    memset(dstMap, 0, nverts * sizeof(WORD));

    // mark used vertices
    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;
        for (int j = 0; j < p->nverts; ++j)
        {
            const WORD vi = p->verts[j];
            if (vi < nverts)
                usedMap[vi] = 1;
        }
    }

    // count used
    int usedCount = 0;
    for (int i = 0; i < nverts; ++i)
        if (usedMap[i]) ++usedCount;

    VertexSet* newV = new VertexSet(usedCount);
    int out = 0;

    // compress + weld (MatchVerts)
    for (int i = 0; i < nverts; ++i)
    {
        if (!usedMap[i]) continue;

        // assign new index
        dstMap[i] = (WORD)out;

        newV->loc[out] = vertex_set->loc[i];
        newV->nrm[out] = vertex_set->nrm[i];
        newV->tu[out] = vertex_set->tu[i];
        newV->tv[out] = vertex_set->tv[i];
        newV->rw[out] = vertex_set->rw[i];
        newV->diffuse[out] = vertex_set->diffuse[i];
        newV->specular[out] = vertex_set->specular[i];

        // weld duplicates
        for (int j = i + 1; j < nverts; ++j)
        {
            if (!usedMap[j]) continue;

            if (MatchVerts(vertex_set, i, j))
            {
                usedMap[j] = 0;
                dstMap[j] = (WORD)out;
            }
        }

        ++out;
    }

    // remap polys
    for (int i = 0; i < npolys; ++i)
    {
        Poly* p = polys + i;
        p->vertex_set = newV;

        for (int j = 0; j < p->nverts; ++j)
            p->verts[j] = dstMap[p->verts[j]];
    }

    // finalize
    delete vertex_set;
    vertex_set = newV;

    // rebuild hull buffer
    delete[] vloc;
    vloc = new FVector[vertex_set->nverts];
    memset(vloc, 0, sizeof(FVector) * vertex_set->nverts);

    delete[] usedMap;
    delete[] dstMap;

    ComputeTangents();
    BuildHull();

    InvalidateSurfaceCache(this);
}

void Surface::Copy(Surface& s, SimModel* m)
{
    segments.destroy();

    delete video_data;
    video_data = nullptr;

    delete vertex_set;
    vertex_set = nullptr;

    delete[] vloc;
    vloc = nullptr;

    delete[] polys;
    polys = nullptr;

    memcpy(name, s.name, Solid::NAMELEN);

    model = m;
    radius = s.radius;
    nhull = s.nhull;
    npolys = s.npolys;
    nindices = s.nindices;
    state = s.state;
    offset = s.offset;
    OrientationMatrix = s.OrientationMatrix;

    // Clone vertex set:
    vertex_set = s.vertex_set ? s.vertex_set->Clone() : nullptr;

    // Hull buffer:
    if (vertex_set && vertex_set->nverts > 0)
    {
        vloc = new FVector[vertex_set->nverts];
        memset(vloc, 0, sizeof(FVector) * vertex_set->nverts);
    }

    // Poly copy:
    if (s.npolys > 0)
    {
        polys = new Poly[s.npolys];
        memcpy(polys, s.polys, sizeof(Poly) * s.npolys);

        for (int i = 0; i < npolys; ++i)
        {
            polys[i].vertex_set = vertex_set;

            if (s.polys[i].material && model)
                polys[i].material = (Material*)model->FindMaterial(s.polys[i].material->name);
        }
    }

    // Segment deep copy (segments list holds Segment*):
    ListIter<Segment> it = s.segments;
    while (++it)
    {
        Segment* src = it.value();
        if (!src) continue;

        Segment* dst = new Segment;
        dst->npolys = src->npolys;

        // Rebase poly pointer into our poly array:
        const ptrdiff_t polyOffset = (src->polys - s.polys);
        dst->polys = polys + polyOffset;

        dst->material = dst->polys && dst->polys[0].material ? dst->polys[0].material : nullptr;
        dst->model = model;
        dst->video_data = nullptr;

        segments.append(dst);
    }

    InvalidateSurfaceCache(this);
    BuildHull();
}
