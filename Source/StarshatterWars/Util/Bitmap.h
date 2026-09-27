/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         Bitmap.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Bitmap Resource class (GPU-first Unreal implementation)

    NOTES
    =====
    - Maintains Starshatter call structure.
    - Uses Unreal primitives: FColor, FIntRect, UTexture2D/UTextureRenderTarget2D.
    - Palette/indexed mode is best-effort for legacy compatibility; GPU path is RGBA8.
*/

#pragma once

#include "Res.h"
#include "Types.h"
#include "Geometry.h"

// Unreal minimal includes for public API primitives:
#include "Math/Color.h"
#include "Math/IntRect.h"

#include "COlor.h"

class UTexture2D;
class UTextureRenderTarget2D;
class FCanvas;

// +--------------------------------------------------------------------+

class Bitmap : public Resource
{
public:
    static const char* TYPENAME() { return "Bitmap"; }

    enum BMP_TYPES { BMP_SOLID, BMP_TRANSPARENT, BMP_TRANSLUCENT };

    Bitmap();
    Bitmap(int w, int h, ColorIndex* p = 0, int t = BMP_SOLID);
    Bitmap(int w, int h, FColor* p, int t = BMP_SOLID);
    virtual ~Bitmap();

    // Legacy format flags:
    int         IsIndexed()   const { return pix != 0; }
    int         IsHighColor() const { return hipix != 0; }
    int         IsDual()      const { return IsIndexed() && IsHighColor(); }

    void        SetType(int t) { type = t; }
    int         Type()         const { return type; }
    bool        IsSolid()      const { return type == BMP_SOLID; }
    bool        IsTransparent()const { return type == BMP_TRANSPARENT; }
    bool        IsTranslucent()const { return type == BMP_TRANSLUCENT; }

    // Dimensions:
    int         Width()        const { return width; }
    int         Height()       const { return height; }

    // Legacy CPU buffers (may be null in GPU-first mode):
    ColorIndex* Pixels()       const { return pix; }
    FColor* HiPixels()     const { return hipix; }

    // Legacy sizing helpers:
    int         BmpSize()      const;
    int         RowSize()      const;

    // Pixel access (legacy call structure):
    ColorIndex  GetIndex(int x, int y) const;
    FColor      GetColor(int x, int y) const;
    void        SetIndex(int x, int y, uint8 value);
    void        SetColor(int x, int y, FColor c);

    // Legacy linear-index helpers (to match older call sites):
    void        SetColor(int i, FColor c);
    void        SetIndex(int i, ColorIndex c);

    // Fill/clear:
    void        FillColor(FColor c);
    void        ClearImage();

    // Copy/blit:
    void        BitBlt(int x, int y, const Bitmap& srcImage, int sx, int sy, int w, int h, bool blend = false);
    void        CopyBitmap(const Bitmap& rhs);
    void        CopyImage(int w, int h, BYTE* p, int t = BMP_SOLID);
    void        CopyImage(int w, int h, ColorIndex* p, int t = BMP_SOLID); // convenience overload
    void        CopyHighColorImage(int w, int h, DWORD* p, int t = BMP_SOLID);
    void        CopyAlphaImage(int w, int h, BYTE* p);
    void        CopyAlphaRedChannel(int w, int h, DWORD* p);
    void        AutoMask(DWORD mask = 0);

    // CPU surface API (legacy; may be null in GPU-first mode):
    virtual BYTE* GetSurface();
    virtual int   Pitch() const;
    virtual int   PixSize() const;

    // Clipping:
    bool        ClipLine(int& x1, int& y1, int& x2, int& y2);
    bool        ClipLine(double& x1, double& y1, double& x2, double& y2);

    // Drawing API (Unreal primitives):
    void        DrawLine(int x1, int y1, int x2, int y2, FColor color);
    void        DrawRect(int x1, int y1, int x2, int y2, FColor color);
    void        DrawRect(const FIntRect& r, FColor color);
    void        FillRect(int x1, int y1, int x2, int y2, FColor color);
    void        FillRect(const FIntRect& r, FColor color);
    void        DrawEllipse(int x1, int y1, int x2, int y2, FColor color, BYTE quad = 0x0f);
    void        DrawEllipsePoints(int x0, int y0, int x, int y, FColor c, BYTE quad);

    // Resampling/format:
    void        ScaleTo(int w, int h);
    void        MakeIndexed();
    void        MakeHighColor();
    void        MakeTexture();

    // Texture state:
    bool        IsTexture() const { return texture; }
    void        TakeOwnership() { ownpix = true; }

    // Name:
    const char* GetFilename() const { return filename; }
    void        SetFilename(const char* s);

    // Timestamp:
    DWORD       LastModified() const { return last_modified; }

    // Cache/utility:
    static Bitmap* Default();

    static Bitmap* GetBitmapByID(HANDLE bmp_id);
    static Bitmap* CheckCache(const char* filename);
    static void    AddToCache(Bitmap* bmp);
    static void    CacheUpdate();
    static void    ClearCache();
    static DWORD   CacheMemoryFootprint();

    // GPU-first accessors (optional for callers):
    UTexture2D* GetTexture()     const { return Texture; }
    UTextureRenderTarget2D* GetRenderTarget()const { return RenderTarget; }

protected:
    // GPU backing:
    void        EnsureRenderTarget();
    void        DrawBegin(FCanvas*& OutCanvas);
    void        DrawEnd(FCanvas*& InOutCanvas);
    void        ClearToTransparent();
    void        UploadBGRA(const void* SrcBGRA8, int SrcW, int SrcH, bool bHasAlpha, bool bSRGB);

    // CPU staging for texture upload (optional path):
    void        EnsureHiPixels();
    void        MarkDirty();

protected:
    int         type = BMP_SOLID;
    int         width = 0;
    int         height = 0;
    int         mapsize = 0;

    bool        ownpix = false;
    bool        alpha_loaded = false;
    bool        texture = false;

    // Legacy CPU buffers (prefer to phase out; keep for compatibility):
    ColorIndex* pix = nullptr;
    FColor* hipix = nullptr;

    // CPU staging buffer for GPU upload (BGRA8):
    TArray<FColor> HiPixelsCPU;
    bool           bDirtyCPU = false;

    // Unreal GPU backing:
    UTexture2D* Texture = nullptr;
    UTextureRenderTarget2D* RenderTarget = nullptr;

    // Metadata:
    DWORD       last_modified = 0;
    char        filename[64] = {};
};
