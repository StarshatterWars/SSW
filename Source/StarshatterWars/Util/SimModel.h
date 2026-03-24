/*  Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: SimModel.h
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    SimModel
    - Owns renderable geometry: surfaces, materials, segments
    - UE-safe replacement for legacy Model
*/

#pragma once

#include "List.h"
#include "Polygon.h"
#include "Video.h"
#include "Surface.h"

// Unreal math:
#include "Math/Vector.h"

// Forward declarations
class Solid;
class Segment;
class ModelFile;
class Bitmap;

// --------------------------------------------------------------------

class SimModel
{
    friend class Solid;
    friend class SimModelFile;

public:
    static const char* TYPENAME() { return "SimModel"; }

    enum { MAX_VERTS = 64000, MAX_POLYS = 16000 };

    SimModel();
    SimModel(const SimModel& m);
    ~SimModel();

    SimModel& operator=(const SimModel& m);
    int operator==(const SimModel& that) const { return this == &that; }

    // ------------------------------------------------------------
    // Loading
    // ------------------------------------------------------------
    bool Load(const char* mag_file, double scale = 1.0);
    bool Load(ModelFile* loader, double scale = 1.0);

    // ------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------
    const char* GetName() const { return name; }

    int                 GetNumVerts()  const { return nverts; }
    int                 GetNumPolys()  const { return npolys; }
    int                 NumSegments()  const;
    int                 NumSurfaces()  const { return surfaces.size(); }
    int                 NumMaterials() const { return materials.size(); }

    double              GetRadius()      const { return radius; }

    bool                IsDynamic()   const { return dynamic; }
    void                SetDynamic(bool d) { dynamic = d; }

    bool                IsLuminous()  const { return luminous; }
    void                SetLuminous(bool l) { luminous = l; }

    List<Surface>&      GetSurfaces() { return surfaces; }
    List<Material>&     GetMaterials() { return materials; }

    void                AddSurface(Surface* s);

    bool                Rescale(double scale);

    // Optional (if you want a getter):
    const List<Surface>& GetSurfaces() const { return surfaces; }

    const Material* FindMaterial(const char* mtl_name) const;
    const Material* ReplaceMaterial(const Material* mtl);

    void            GetAllTextures(List<Bitmap>& textures);

    Poly*           AddPolys(int nsurf, int nadd_polys, int nadd_verts);

    void            OptimizeMaterials();

    // ------------------------------------------------------------
    // Geometry ops
    // ------------------------------------------------------------
    void   ScaleBy(double factor);
    void   Normalize();
    void   ComputeTangents();

    void   SelectPolys(List<Poly>& selection, Material* mtl);
    void   SelectPolys(List<Poly>& selection, FVector loc);

    // ------------------------------------------------------------
    // Memory / render cache
    // ------------------------------------------------------------
    void DeletePrivateData();

private:
    // Legacy MAG loaders (still implemented in .cpp)
    bool LoadMag5(BYTE* block, int len, double scale);
    bool LoadMag6(BYTE* block, int len, double scale);

private:
    char            name[64]{};

    List<Surface>   surfaces;
    List<Material>  materials;

    int             nverts = 0;
    int             npolys = 0;

    float           radius = 0.0f;
    float           extents[6]{};

    bool            luminous = false;
    bool            dynamic = false;
};
