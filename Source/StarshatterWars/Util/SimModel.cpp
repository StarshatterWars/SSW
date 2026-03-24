/*  Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: SimModel.cpp
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    Model mesh container + MAG loaders (MAG5/MAG6).
    - UE-friendly math types (FVector) are already used by VertexSet/Plane/etc.
    - No OPCODE (collision is handled elsewhere via UE/native methods).
*/

#include "SimModel.h"
#include "Bitmap.h"
#include "Surface.h"
#include "Segment.h"
#include "ModelFile.h"
#include "GameStructs.h"
#include "DataLoader.h"
#include "List.h"

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "Math/Color.h"

#include <cstdlib>   // qsort

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterModel, Log, All);

// +--------------------------------------------------------------------+
// Helpers (local to this TU)

static inline FColor ScaleColor(const FColor& In, float Scalar)
{
    FLinearColor Lin = FLinearColor::FromSRGBColor(In);
    Lin *= Scalar;

    Lin.R = FMath::Clamp(Lin.R, 0.0f, 1.0f);
    Lin.G = FMath::Clamp(Lin.G, 0.0f, 1.0f);
    Lin.B = FMath::Clamp(Lin.B, 0.0f, 1.0f);
    Lin.A = FMath::Clamp(Lin.A, 0.0f, 1.0f);

    return Lin.ToFColor(true);
}

static inline FColor ColorBytes(uint8 r, uint8 g, uint8 b, uint8 a = 255)
{
    return FColor(r, g, b, a);
}

inline bool Collinear(const double* a, const double* b, const double* c)
{
    const FVector ab((float)(b[0] - a[0]), (float)(b[1] - a[1]), (float)(b[2] - a[2]));
    const FVector ac((float)(c[0] - a[0]), (float)(c[1] - a[1]), (float)(c[2] - a[2]));
    const FVector cross = FVector::CrossProduct(ab, ac);
    return cross.IsNearlyZero();
}

static int modelcomp(const void* a, const void* b)
{
    Poly* pa = (Poly*)a;
    Poly* pb = (Poly*)b;

    if (pa->sortval == pb->sortval) {
        return 0;
    }

    // descending:
    return (pa->sortval < pb->sortval) ? 1 : -1;
}
struct HomogenousPlane
{
    double distance;
    double normal_x;
    double normal_y;
    double normal_z;
    double normal_w;
};

static void LoadPlane(Plane& p, DataLoader* l, BYTE*& fp)
{
    HomogenousPlane tmp{};
    l->fread(&tmp, sizeof(HomogenousPlane), 1, fp);

    p.normal.X = tmp.normal_x;
    p.normal.Y = tmp.normal_y;
    p.normal.Z = tmp.normal_z;
    p.distance = tmp.distance;
}

static void LoadFlags(DWORD* out_flags, DataLoader* l, BYTE*& fp)
{
    DWORD magic_flags = 0;
    l->fread(&magic_flags, sizeof(DWORD), 1, fp);

    const DWORD magic_mask = 0x0fc3;
    *out_flags = magic_flags & magic_mask;
}

// +--------------------------------------------------------------------+
// SimModel

SimModel::SimModel()
    : nverts(0)
    , npolys(0)
    , radius(0.0f)
    , luminous(false)
    , dynamic(false)
{
    FMemory::Memset(name, 0, sizeof(name));
    extents[0] = extents[1] = extents[2] = extents[3] = extents[4] = extents[5] = 0.0f;
}

SimModel::SimModel(const SimModel& m)
    : nverts(0)
    , npolys(0)
    , radius(0.0f)
    , luminous(false)
    , dynamic(false)
{
    FMemory::Memset(name, 0, sizeof(name));
    extents[0] = extents[1] = extents[2] = extents[3] = extents[4] = extents[5] = 0.0f;

    operator=(m);
}

SimModel::~SimModel()
{
    surfaces.destroy();
    materials.destroy();
}

SimModel& SimModel::operator=(const SimModel& m)
{
    if (this == &m)
        return *this;

    surfaces.destroy();
    materials.destroy();

    FMemory::Memcpy(name, m.name, NAMELEN);

    nverts = m.nverts;
    npolys = m.npolys;
    radius = m.radius;
    luminous = m.luminous;
    dynamic = m.dynamic;

    extents[0] = m.extents[0];
    extents[1] = m.extents[1];
    extents[2] = m.extents[2];
    extents[3] = m.extents[3];
    extents[4] = m.extents[4];
    extents[5] = m.extents[5];

    // Deep copy materials:
    SimModel* src = (SimModel*)&m;
    ListIter<Material> m_iter = src->materials;
    while (++m_iter) {
        Material* matl1 = m_iter.value();
        Material* matl2 = new Material;

        FMemory::Memcpy(matl2, matl1, sizeof(Material));
        matl2->thumbnail = nullptr;

        materials.append(matl2);
    }

    // Deep copy surfaces (Surface::Copy will remap materials by name):
    ListIter<Surface> s_iter = src->surfaces;
    while (++s_iter) {
        Surface* surf1 = s_iter.value();
        Surface* surf2 = new Surface;

        surf2->Copy(*surf1, this);
        surfaces.append(surf2);
    }

    return *this;
}

int SimModel::NumSegments() const
{
    int nsegments = 0;

    for (int i = 0; i < surfaces.size(); i++) {
        const Surface* s = surfaces[i];
        nsegments += s ? s->GetNumSegments() : 0;
    }

    return nsegments;
}

const Material* SimModel::FindMaterial(const char* mtl_name) const
{
    if (!mtl_name || !mtl_name[0])
        return nullptr;

    SimModel* self = (SimModel*)this;
    ListIter<Material> iter = self->materials;
    while (++iter) {
        Material* m = iter.value();
        if (m && FCStringAnsi::Strcmp(m->name, mtl_name) == 0)
            return m;
    }

    return nullptr;
}

const Material* SimModel::ReplaceMaterial(const Material* mtl)
{
    if (!mtl)
        return nullptr;

    Material* existing = (Material*)FindMaterial(mtl->name);
    if (existing) {
        // Preserve the existing thumbnail pointer (do not overwrite ownership)
        Bitmap* thumb = existing->thumbnail;

        FMemory::Memcpy(existing, mtl, sizeof(Material));

        existing->thumbnail = thumb;
        return existing;
    }

    Material* clone = new Material;
    FMemory::Memcpy(clone, mtl, sizeof(Material));

    // Do not copy/own thumbnails by default
    clone->thumbnail = nullptr;

    materials.append(clone);
    return clone;
}

void SimModel::GetAllTextures(List<Bitmap>& textures)
{
    ListIter<Material> m_iter = materials;
    while (++m_iter) {
        Material* m = m_iter.value();
        if (!m) continue;

        if (m->tex_diffuse && !textures.contains(m->tex_diffuse))   textures.append(m->tex_diffuse);
        if (m->tex_specular && !textures.contains(m->tex_specular))  textures.append(m->tex_specular);
        if (m->tex_emissive && !textures.contains(m->tex_emissive))  textures.append(m->tex_emissive);
        if (m->tex_bumpmap && !textures.contains(m->tex_bumpmap))   textures.append(m->tex_bumpmap);
        if (m->tex_detail && !textures.contains(m->tex_detail))    textures.append(m->tex_detail);
    }
}


void SimModel::AddSurface(Surface* s)
{
    if (!s)
        return;

    s->model = this;
    surfaces.append(s);
}

Poly* SimModel::AddPolys(int nsurf, int nadd_polys, int nadd_verts)
{
    nverts = 0;
    npolys = 0;
    
    if (nsurf < 0 || nadd_polys <= 0 || nadd_verts <= 0)
        return nullptr;

    // Ensure surface exists:
    while (surfaces.size() <= nsurf)
    {
        Surface* s = new Surface;
        if (!s)
            return nullptr;

        s->model = this;
        s->SetName("surface");
        surfaces.append(s);
    }

    Surface* target = surfaces[nsurf];
    if (!target)
        return nullptr;

    // Ensure backing storage exists:
    if (!target->vertex_set)
        target->CreateVerts(0);

    if (!target->polys)
        target->CreatePolys(0);

    Poly* added = target->AddPolys(nadd_polys, nadd_verts);

    // Recompute aggregate counters:
    

    for (int i = 0; i < surfaces.size(); i++)
    {
        Surface* s = surfaces[i];
        if (!s) continue;

        nverts += s->GetNumVerts();
        npolys += s->GetNumPolys();
    }

    return added;
}

void SimModel::OptimizeMaterials()
{
    // Keep as-is for now (MAG material indices are meaningful).
}

void SimModel::ScaleBy(double factor)
{
    ListIter<Surface> iter = surfaces;
    while (++iter) {
        Surface* s = iter.value();
        if (s) s->ScaleBy(factor);
    }

    radius *= (float)factor;
    for (int i = 0; i < 6; i++)
        extents[i] *= (float)factor;
}

void SimModel::Normalize()
{
    ListIter<Surface> iter = surfaces;
    while (++iter) {
        Surface* s = iter.value();
        if (s) s->Normalize();
    }
}

void SimModel::SelectPolys(List<Poly>& out, Material* mtl)
{
    ListIter<Surface> iter = surfaces;
    while (++iter) {
        Surface* s = iter.value();
        if (s) s->SelectPolys(out, mtl);
    }
}

void SimModel::SelectPolys(List<Poly>& out, FVector loc)
{
    ListIter<Surface> iter = surfaces;
    while (++iter) {
        Surface* s = iter.value();
        if (s) s->SelectPolys(out, loc);
    }
}

void SimModel::ComputeTangents()
{
    ListIter<Surface> iter = surfaces;
    while (++iter) {
        Surface* s = iter.value();
        if (s) s->ComputeTangents();
    }
}

void SimModel::DeletePrivateData()
{
    ListIter<Surface> s_iter = surfaces;
    while (++s_iter) {
        Surface* s = s_iter.value();
        if (!s) continue;

        if (VideoPrivateData* vpd = s->GetVideoPrivateData()) {
            delete vpd;
            s->SetVideoPrivateData(nullptr);
        }

        ListIter<Segment> seg_iter = s->GetSegments();
        while (++seg_iter) {
            Segment* seg = seg_iter.value();
            if (!seg) continue;

            if (VideoPrivateData* svpd = seg->GetVideoPrivateData()) {
                delete svpd;
                seg->SetVideoPrivateData(nullptr);
            }
        }
    }
}

// +--------------------------------------------------------------------+
// Loading

bool SimModel::Load(const char* mag_file, double scale)
{
    BYTE* block = nullptr;
    DataLoader* loader = DataLoader::GetLoader();
    bool result = false;

    radius = 0.0f;
    extents[0] = extents[1] = extents[2] = extents[3] = extents[4] = extents[5] = 0.0f;

    if (!loader) {
        UE_LOG(LogStarshatterModel, Warning, TEXT("MAG Open Failed: no DataLoader for '%hs'"), mag_file ? mag_file : "");
        return false;
    }

    const int size = loader->LoadBuffer(mag_file, block);
    if (!size || !block) {
        UE_LOG(LogStarshatterModel, Warning, TEXT("MAG Open Failed: could not open '%hs'"), mag_file ? mag_file : "");
        return false;
    }

    // Name:
    FMemory::Memset(name, 0, sizeof(name));
    FCStringAnsi::Strncpy(name, mag_file ? mag_file : "", sizeof(name));

    // Validate header:
    char file_id[5]{};
    FMemory::Memcpy(file_id, block, 4);
    file_id[4] = '\0';

    int version = 0;
    if (FCStringAnsi::Strcmp(file_id, "MAG6") == 0)      version = 6;
    else if (FCStringAnsi::Strcmp(file_id, "MAG5") == 0) version = 5;
    else if (FCStringAnsi::Strcmp(file_id, "MAG4") == 0) version = 4;

    if (version == 0) {
        UE_LOG(LogStarshatterModel, Warning, TEXT("MAG Open Failed: '%hs' invalid type '%hs'"),
            mag_file ? mag_file : "", file_id);
        loader->ReleaseBuffer(block);
        return false;
    }

    surfaces.destroy();
    materials.destroy();
    nverts = 0;
    npolys = 0;

    switch (version) {
    case 4:
    case 5:
        result = LoadMag5(block, size, scale);
        break;
    case 6:
        result = LoadMag6(block, size, scale);
        break;
    default:
        break;
    }

    loader->ReleaseBuffer(block);
    return result;
}

bool SimModel::Load(ModelFile* mod_file, double scale)
{
    return mod_file ? mod_file->Load(this, scale) : false;
}

// +--------------------------------------------------------------------+
// MAG5

bool SimModel::LoadMag5(BYTE* block, int /*len*/, double scale)
{
    bool result = false;

    DataLoader* loader = DataLoader::GetLoader();
    BYTE* fp = block + 4;

    int ntex = 0;
    int nsurfs = 0;

    loader->fread(&ntex, sizeof(ntex), 1, fp);
    loader->fread(&nsurfs, sizeof(nsurfs), 1, fp);

    // Default material:
    Material* mtl = new Material;
    if (mtl)
    {
        // Ka/Kd/Ks/Ke are FColor:
        mtl->Ka = FColor(192, 192, 192, 255); // light gray
        mtl->Kd = FColor(192, 192, 192, 255); // light gray
        mtl->Ks = FColor(51, 51, 51, 255); // ~0.2
        mtl->Ke = FColor(0, 0, 0, 255); // not emissive by default

        mtl->power = 20.0f;

        // These are legacy fields (Color is your engine type):
        mtl->ambient_value = 1.0f;
        mtl->ambient_color = FColor(192, 192, 192);
        mtl->diffuse_value = 1.0f;
        mtl->diffuse_color = FColor(192, 192, 192);
        mtl->specular_value = 0.2f;
        mtl->specular_color = FColor::White;

        FCStringAnsi::Strcpy(mtl->name, "(default)");
        materials.append(mtl);
    }

    // Texture list -> materials:
    for (int i = 0; i < ntex; i++)
    {
        mtl = new Material;
        char tname[32] = {};

        if (mtl)
        {
            // Ka/Kd/Ks/Ke are FColor:
            mtl->Ka = FColor(128, 128, 128, 255); // ~0.5 gray
            mtl->Kd = FColor(255, 255, 255, 255); // white
            mtl->Ks = FColor(51, 51, 51, 255); // ~0.2
            mtl->Ke = FColor(0, 0, 0, 255); // not emissive by default

            mtl->power = 20.0f;

            // Legacy fields (Color is your engine type):
            mtl->ambient_value = 1.0f;
            mtl->ambient_color = FColor(128, 128, 128);
            mtl->diffuse_value = 1.0f;
            mtl->diffuse_color = FColor::White;
            mtl->specular_value = 0.2f;
            mtl->specular_color = FColor::White;

            loader->fread(tname, 32, 1, fp);

            // UE texture:
            loader->LoadTexture(tname, mtl->tex_diffuse, 0, true);

            FCStringAnsi::Strncpy(mtl->name, tname, Material::NAMELEN);

            if (char* dot = FCStringAnsi::Strrchr(mtl->name, '.'))  *dot = 0;
            if (char* plus = FCStringAnsi::Strrchr(mtl->name, '+')) *plus = 0;

            materials.append(mtl);
        }
    }

    loader->fread(&nverts, 4, 1, fp);
    loader->fread(&npolys, 4, 1, fp);

    const int mag_nverts = nverts;
    int next_vert = nverts;

    // Plan on creating four verts per poly:
    nverts = npolys * 4;

    Surface* s = new Surface;
    if (!s)
        return false;

    s->SetName("default");
    s->model = this;
    s->vertex_set = new VertexSet(nverts);
    s->vloc = new FVector[nverts];

    FMemory::Memset(s->vertex_set->loc, 0, nverts * sizeof(FVector));
    FMemory::Memset(s->vertex_set->nrm, 0, nverts * sizeof(FVector));
    FMemory::Memset(s->vertex_set->diffuse, 0, nverts * sizeof(DWORD));
    FMemory::Memset(s->vertex_set->specular, 0, nverts * sizeof(DWORD));
    FMemory::Memset(s->vertex_set->tu, 0, nverts * sizeof(float));
    FMemory::Memset(s->vertex_set->tv, 0, nverts * sizeof(float));
    FMemory::Memset(s->vertex_set->rw, 0, nverts * sizeof(float));
    FMemory::Memset(s->vloc, 0, nverts * sizeof(FVector));

    s->npolys = npolys;
    s->polys = new Poly[npolys];
    FMemory::Memset(s->polys, 0, sizeof(Poly) * npolys);

    surfaces.append(s);

    VertexSet* vset = s->vertex_set;

    // Vertex set:
    int v = 0;
    for (v = 0; v < mag_nverts; v++)
    {
        FVector vert(0, 0, 0);
        FVector norm(0, 0, 0);
        DWORD   vstate = 0;

        loader->fread(&vert, sizeof(FVector), 1, fp);
        loader->fread(&norm, sizeof(FVector), 1, fp);
        loader->fread(&vstate, sizeof(DWORD), 1, fp);

        Swap(vert.Y, vert.Z);
        vert *= (float)scale;

        vset->loc[v] = vert;
        vset->nrm[v] = norm;

        const float d = vert.Size();
        radius = FMath::Max(radius, d);

        extents[0] = FMath::Max(extents[0], vert.X);
        extents[1] = FMath::Min(extents[1], vert.X);
        extents[2] = FMath::Max(extents[2], vert.Y);
        extents[3] = FMath::Min(extents[3], vert.Y);
        extents[4] = FMath::Max(extents[4], vert.Z);
        extents[5] = FMath::Min(extents[5], vert.Z);
    }

    while (v < nverts)
        vset->nrm[v++] = FVector(1, 0, 0);

    // Polys:
    FVector dummy_center(0, 0, 0);
    DWORD   dummy_flags = 0;
    DWORD   dummy_color = 0;
    Plane   dummy_plane;
    int     texture_num = 0;
    int     poly_nverts = 0;
    int     vert_index_buffer[32] = {};
    float   texture_index_buffer[32] = {};

    for (int n = 0; n < npolys; n++)
    {
        Poly& poly = s->polys[n];
        poly.vertex_set = vset;

        loader->fread(&dummy_flags, sizeof(DWORD), 1, fp);
        loader->fread(&dummy_center, sizeof(FVector), 1, fp);

        LoadPlane(dummy_plane, loader, fp);

        loader->fread(&dummy_color, sizeof(DWORD), 1, fp);
        loader->fread(&texture_num, sizeof(int), 1, fp);

        if (texture_num >= 0 && texture_num < ntex)
        {
            const int mtl_num = texture_num + 1;
            poly.material = materials[mtl_num];
            poly.sortval = texture_num;

            const bool flag_translucent = (dummy_flags & 0x04) ? true : false;
            const bool flag_transparent = (dummy_flags & 0x08) ? true : false;

            // Luminous:
            if (dummy_flags & 2)
            {
                Material* lm = materials[mtl_num];

                // FColor assignments:
                lm->Ka = ColorBytes(0, 0, 0, 0);
                lm->Kd = ColorBytes(0, 0, 0, 0);
                lm->Ks = ColorBytes(0, 0, 0, 0);
                lm->Ke = ColorBytes(255, 255, 255, 255);

                if (lm->tex_diffuse && !lm->tex_emissive)
                    lm->tex_emissive = lm->tex_diffuse;
            }

            if (flag_translucent && flag_transparent)
                materials[mtl_num]->blend = Material::MTL_ADDITIVE;
            else if (flag_translucent || flag_transparent)
                materials[mtl_num]->blend = Material::MTL_TRANSLUCENT;
        }
        else
        {
            poly.material = materials.first();
            poly.sortval = 1000;
        }

        // Flat shaded flag stored in visible:
        poly.visible = (BYTE)(dummy_flags & 1);

        loader->fread(&poly_nverts, sizeof(int), 1, fp);
        loader->fread(vert_index_buffer, sizeof(int), poly_nverts, fp);

        if (poly_nverts == 3)      s->nindices += 3;
        else if (poly_nverts == 4) s->nindices += 6;

        poly.nverts = poly_nverts;

        for (int vi = 0; vi < poly_nverts; vi++)
        {
            v = vert_index_buffer[vi];

            if (vset->rw[v] > 0)
            {
                vset->CopyVertex(next_vert, v);
                v = next_vert++;
            }

            vset->rw[v] = 1;
            poly.verts[vi] = v;
        }

        // tu:
        loader->fread(texture_index_buffer, sizeof(float), poly_nverts, fp);
        for (int vi = 0; vi < poly_nverts; vi++)
        {
            v = poly.verts[vi];
            vset->tu[v] = texture_index_buffer[vi];
        }

        // tv:
        loader->fread(texture_index_buffer, sizeof(float), poly_nverts, fp);
        for (int vi = 0; vi < poly_nverts; vi++)
        {
            v = poly.verts[vi];
            vset->tv[v] = texture_index_buffer[vi];
        }

        fp += 16;
    }

    // Pass 2: planes + flat normals:
    for (int n = 0; n < npolys; n++)
    {
        Poly& poly = s->polys[n];

        poly.plane = Plane(
            vset->loc[poly.verts[0]],
            vset->loc[poly.verts[2]],
            vset->loc[poly.verts[1]]
        );

        if (poly.visible)
        {
            for (int vi = 0; vi < poly.nverts; vi++)
            {
                v = poly.verts[vi];
                vset->nrm[v] = poly.plane.normal;
            }
        }
    }

    // Sort polys by material:
    qsort((void*)s->polys, s->npolys, sizeof(Poly), modelcomp);

    // Build segments:
    Segment* segment = nullptr;
    for (int n = 0; n < npolys; n++)
    {
        if (segment && segment->material == s->polys[n].material)
        {
            segment->npolys++;
        }
        else
        {
            segment = new Segment;
            segment->npolys = 1;
            segment->polys = &s->polys[n];
            segment->material = segment->polys->material;
            segment->model = this;

            s->segments.append(segment);
        }
    }

    s->BuildHull();

    result = (nverts != 0) && (npolys != 0);
    return result;
}

// +--------------------------------------------------------------------+
// MAG6

struct MaterialMag6
{
    char  name[Material::NAMELEN];
    char  shader[Material::NAMELEN];
    float power;
    float brilliance;
    float bump;
    DWORD blend;
    bool  shadow;
    bool  luminous;

    FColor ambient_color;
    FColor diffuse_color;
    FColor specular_color;
    FColor emissive_color;

    float ambient_value;
    float diffuse_value;
    float specular_value;
    float emissive_value;

    BYTE  tex_diffuse;
    BYTE  tex_specular;
    BYTE  tex_bumpmap;
    BYTE  tex_emissive;
};

bool SimModel::LoadMag6(BYTE* block, int /*len*/, double scale)
{
    bool result = false;

    DataLoader* loader = DataLoader::GetLoader();
    BYTE* fp = block + 4;

    int ntex = 0;
    int nmtls = 0;
    int nsurfs = 0;

    // Texture block and material count:
    List<Bitmap> textures;

    loader->fread(&ntex, sizeof(ntex), 1, fp);
    loader->fread(&nmtls, sizeof(nmtls), 1, fp);
    loader->fread(&nsurfs, sizeof(nsurfs), 1, fp);

    // Read texture list:
    if (ntex > 0) {
        char* buffer = new char[ntex];
        char* p = buffer;

        loader->fread(buffer, ntex, 1, fp);

        while (p < buffer + ntex) {
            Bitmap* tex = nullptr;
            loader->LoadTexture(p, tex, /*type*/0, /*mipmaps*/true);
            textures.append(tex);

            p += FCStringAnsi::Strlen(p) + 1;
        }

        delete[] buffer;
    }

    // Materials:
    for (int i = 0; i < nmtls; i++) {
        MaterialMag6 m6{};
        Material* mtl = new Material;

        loader->fread(&m6, sizeof(m6), 1, fp);
        if (!mtl) continue;

        FMemory::Memcpy(mtl->name, m6.name, Material::NAMELEN);
        FMemory::Memcpy(mtl->shader, m6.shader, Material::NAMELEN);

        mtl->ambient_value = m6.ambient_value;
        mtl->ambient_color = m6.ambient_color;
        mtl->diffuse_value = m6.diffuse_value;
        mtl->diffuse_color = m6.diffuse_color;
        mtl->specular_value = m6.specular_value;
        mtl->specular_color = m6.specular_color;
        mtl->emissive_value = m6.emissive_value;
        mtl->emissive_color = m6.emissive_color;

        // NOTE:
        // If Ka/Kd/Ks/Ke are FColor in your Material, you cannot assign "Color"/"FLinearColor" directly.
        // Keep your existing FColor conversion logic here (we'll fix it in the next pass if needed).

        mtl->power = m6.power;
        mtl->brilliance = m6.brilliance;
        mtl->bump = m6.bump;
        mtl->blend = m6.blend;
        mtl->shadow = m6.shadow;
        mtl->luminous = m6.luminous;

        if (m6.tex_diffuse && m6.tex_diffuse <= textures.size()) mtl->tex_diffuse = textures[m6.tex_diffuse - 1];
        if (m6.tex_specular && m6.tex_specular <= textures.size()) mtl->tex_specular = textures[m6.tex_specular - 1];
        if (m6.tex_emissive && m6.tex_emissive <= textures.size()) mtl->tex_emissive = textures[m6.tex_emissive - 1];
        if (m6.tex_bumpmap && m6.tex_bumpmap <= textures.size()) mtl->tex_bumpmap = textures[m6.tex_bumpmap - 1];

        materials.append(mtl);
    }

    // Surfaces:
    for (int i = 0; i < nsurfs; i++) {
        int  nverts_local = 0;
        int  npolys_local = 0;
        BYTE namelen = 0;
        char surf_name[128]{};

        loader->fread(&nverts_local, 4, 1, fp);
        loader->fread(&npolys_local, 4, 1, fp);
        loader->fread(&namelen, 1, 1, fp);

        if (namelen > 0) {
            const int safe_len = (namelen < (BYTE)(sizeof(surf_name) - 1)) ? namelen : (int)(sizeof(surf_name) - 1);
            loader->fread(surf_name, 1, safe_len, fp);
            surf_name[safe_len] = 0;

            if (safe_len < namelen)
                fp += (namelen - safe_len);
        }
        else {
            surf_name[0] = 0;
        }

        Surface* surface = new Surface;
        surface->model = this;
        surface->SetName(surf_name);
        surface->CreateVerts(nverts_local);
        surface->CreatePolys(npolys_local);

        VertexSet* vset = surface->GetVertexSet();
        Poly* polys = surface->GetPolys();

        FMemory::Memset(polys, 0, sizeof(Poly) * npolys_local);

        // Vertex set:
        for (int v = 0; v < nverts_local; v++) {
            loader->fread(&vset->loc[v], sizeof(float), 3, fp);
            loader->fread(&vset->nrm[v], sizeof(float), 3, fp);
            loader->fread(&vset->tu[v], sizeof(float), 1, fp);
            loader->fread(&vset->tv[v], sizeof(float), 1, fp);

            vset->loc[v] *= (float)scale;

            const FVector vert = vset->loc[v];
            const double d = (double)vert.Size();
            if (d > radius) radius = (float)d;

            if (vert.X > extents[0]) extents[0] = vert.X;
            if (vert.X < extents[1]) extents[1] = vert.X;
            if (vert.Y > extents[2]) extents[2] = vert.Y;
            if (vert.Y < extents[3]) extents[3] = vert.Y;
            if (vert.Z > extents[4]) extents[4] = vert.Z;
            if (vert.Z < extents[5]) extents[5] = vert.Z;
        }

        // Polys:
        for (int n = 0; n < npolys_local; n++) {
            Poly& poly = polys[n];

            BYTE poly_nverts = 0;
            BYTE material_index = 0;
            WORD poly_verts[8]{};

            loader->fread(&poly_nverts, sizeof(BYTE), 1, fp);
            loader->fread(&material_index, sizeof(BYTE), 1, fp);
            loader->fread(&poly_verts[0], sizeof(WORD), poly_nverts, fp);

            if (poly_nverts >= 3) {
                poly.nverts = poly_nverts;
                for (int vi = 0; vi < poly_nverts; vi++)
                    poly.verts[vi] = poly_verts[vi];
            }
            else {
                poly.sortval = 666;
            }

            if (material_index > 0 && material_index <= (BYTE)materials.size()) {
                poly.material = materials[material_index - 1];
                poly.sortval = material_index;
            }
            else if (materials.size()) {
                poly.material = materials.first();
                poly.sortval = 1;
            }
            else {
                poly.sortval = 1000;
            }

            if (poly.nverts == 3)      surface->AddIndices(3);
            else if (poly.nverts == 4) surface->AddIndices(6);

            poly.vertex_set = vset;

            if (poly.nverts >= 3) {
                poly.plane = Plane(
                    vset->loc[poly.verts[0]],
                    vset->loc[poly.verts[2]],
                    vset->loc[poly.verts[1]]
                );
            }
        }

        qsort((void*)polys, npolys_local, sizeof(Poly), modelcomp);

        // Segments:
        Segment* segment = nullptr;
        for (int n = 0; n < npolys_local; n++) {
            if (segment && segment->material == polys[n].material) {
                segment->npolys++;
            }
            else {
                segment = new Segment;
                segment->npolys = 1;
                segment->polys = &polys[n];
                segment->material = segment->polys->material;
                segment->model = this;

                surface->GetSegments().append(segment);
            }
        }

        surface->BuildHull();
        surfaces.append(surface);

        this->nverts += nverts_local;
        this->npolys += npolys_local;
    }

    result = (nverts != 0) && (npolys != 0);
    return result;
}

