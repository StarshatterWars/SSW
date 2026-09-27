/*  Starshatter Wars
    Fractal Dev Studios
    Copyright(C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: Solid.h
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    Classes for rendering solid meshes of polygons
*/

#pragma once

#include "Polygon.h"
#include "Graphic.h"
#include "Video.h"
#include "List.h"

// Minimal Unreal includes for FVector/FMatrix:
#include "Math/Vector.h"
#include "Math/Matrix.h"

// +--------------------------------------------------------------------+

class Solid;
class SimModel;
class ModelFile;
class Surface;
class Segment;
class Shadow;
class SimLight;
class SimProjector;

// Forward declare Unreal render asset type (replacing Bitmap where used as a render asset):
class Bitmap;

// +--------------------------------------------------------------------+
// Legacy type mapping notes (Starshatter -> Unreal):
//   Vec3  -> FVector
//   Point -> FVector
//   Matrix (orientation) -> FMatrix
// Keep Starshatter core types (Text, List, Color, etc.) where still used.
// +--------------------------------------------------------------------+

class Solid : public Graphic
{
public:
    static const char* TYPENAME() { return "Solid"; }

    enum { NAMELEN = 64 };

    static bool    IsCollisionEnabled();
    static void    EnableCollision(bool enable);

    Solid();
    virtual ~Solid();

    // operations
    virtual void   Render(Video* video, DWORD flags);
    virtual void   SelectDetail(SimProjector* p);
    virtual void   ProjectScreenRect(SimProjector* p);
    virtual void   Update();

    // accessors / mutators
    SimModel*   GetModel() const { return model; }

    void           GetAllTextures(List<Bitmap>& textures);

    virtual bool   IsDynamic() const;
    virtual void   SetDynamic(bool d);
    virtual void   SetLuminous(bool l);

    // UE: orientation is stored as FMatrix:
    virtual void   SetOrientation(const FMatrix& o);
    virtual void   SetOrientation(const Solid& match);

    const FMatrix& Orientation() const { return OrientationMatrix; }

    float          Roll()  const { return roll; }
    float          Pitch() const { return pitch; }
    float          Yaw()   const { return yaw; }

    virtual bool   IsSolid() const { return true; }

    // stencil shadows
    virtual void   CreateShadows(int nlights = 1);
    virtual void   UpdateShadows(List<SimLight>& lights);
    List<Shadow>& GetShadows() { return shadows; }

    bool           Load(const char* mag_file, double scale = 1.0);
    bool           Load(ModelFile* loader, double scale = 1.0);
    void           UseModel(SimModel* model);
    void           ClearModel();
    bool           Rescale(double scale);

    // collision detection
    virtual int    CollidesWith(Graphic& o);
    virtual int    CheckRayIntersection(FVector pt, FVector vpn, double len, FVector& ipt,
        bool treat_translucent_polys_as_solid = true);
    virtual Poly* GetIntersectionPoly() const { return intersection_poly; }

    // buffer management
    virtual void   DeletePrivateData();
    virtual void   InvalidateSurfaceData();
    virtual void   InvalidateSegmentData();

protected:
    SimModel*      model;
    bool           own_model;

    float          roll, pitch, yaw;

    // UE: store orientation in Unreal-native matrix:
    FMatrix        OrientationMatrix;

    Poly* intersection_poly;

    List<Shadow>   shadows;
};
