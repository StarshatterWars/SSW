/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         Bitmap.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Bitmap Resource class (GPU-first Unreal implementation)

    NOTES
    =====
    - Maintains Starshatter call structure.
    - Internals are implemented using Unreal primitives (FColor, FIntRect) and GPU-backed UTextureRenderTarget2D.
    - CPU pixel buffers (pix/hipix) are not retained by default (GPU-first). Functions that depended on CPU readback
      will warn and return defaults unless a CPU shadow buffer exists.
*/

#include "Bitmap.h"

#include "CoreMinimal.h"

#include "Engine/Engine.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

#include "Engine/Canvas.h"
#include "CanvasTypes.h"
#include "CanvasItem.h"

#include "Templates/EnableIf.h"
#include "Templates/IsIntegral.h"

#include "List.h"       // Starshatter core
#include "Color.h"      // Starshatter core (if still needed elsewhere)

// +--------------------------------------------------------------------+

DWORD GetRealTime();

// +--------------------------------------------------------------------+
// Local helpers (match original structure)
// +--------------------------------------------------------------------+

static inline void swap_i(int& a, int& b) { int tmp = a; a = b; b = tmp; }
static inline void sort_i(int& a, int& b) { if (a > b) swap_i(a, b); }

static inline void swap_d(double& a, double& b) { double tmp = a; a = b; b = tmp; }
static inline void sort_d(double& a, double& b) { if (a > b) swap_d(a, b); }

static int FindBestTexSize(int n, int max_size);


// +--------------------------------------------------------------------+
// Bitmap
// +--------------------------------------------------------------------+

Bitmap::Bitmap()
    : type(BMP_SOLID)
    , width(0)
    , height(0)
    , mapsize(0)
    , ownpix(false)
    , alpha_loaded(false)
    , texture(false)
    , pix(nullptr)
    , hipix(nullptr)
    , last_modified(0)
{
    FMemory::Memzero(filename, sizeof(filename));
    FCStringAnsi::Strcpy(filename, "Bitmap()");
}

Bitmap::Bitmap(int w, int h, ColorIndex* p, int t)
    : type(t)
    , width(w)
    , height(h)
    , mapsize(w* h)
    , ownpix(false)
    , alpha_loaded(false)
    , texture(false)
    , pix(p)
    , hipix(nullptr)
    , last_modified(GetRealTime())
{
    FMemory::Memzero(filename, sizeof(filename));
    FCStringAnsi::Snprintf(filename, sizeof(filename), "Bitmap(%d,%d,index,type=%d)", w, h, (int)t);
}

Bitmap::Bitmap(int w, int h, FColor* p, int t)
    : type(t)
    , width(w)
    , height(h)
    , mapsize(w* h)
    , ownpix(false)
    , alpha_loaded(false)
    , texture(false)
    , pix(nullptr)
    , hipix(p)
    , last_modified(GetRealTime())
{
    FMemory::Memzero(filename, sizeof(filename));
    FCStringAnsi::Snprintf(filename, sizeof(filename), "Bitmap(%d,%d,hicolor,type=%d)", w, h, (int)t);
}

Bitmap::~Bitmap()
{
    if (ownpix) {
        delete[] pix;
        delete[] hipix;
    }

    pix = nullptr;
    hipix = nullptr;
    Texture = nullptr;
    RenderTarget = nullptr;
}

// +--------------------------------------------------------------------+
// Legacy sizing/surface
// +--------------------------------------------------------------------+

int Bitmap::BmpSize() const
{
    return mapsize * PixSize();
}

int Bitmap::RowSize() const
{
    return width;
}

int Bitmap::Pitch() const
{
    return width * PixSize();
}

int Bitmap::PixSize() const
{
    if (hipix) return (int)sizeof(FColor);
    if (pix)   return (int)sizeof(ColorIndex);
    return 0;
}

BYTE* Bitmap::GetSurface()
{
    // GPU-first: only returns CPU surface if we still own CPU buffers.
    if (ownpix) {
        if (hipix) return (BYTE*)hipix;
        if (pix)   return (BYTE*)pix;
    }
    return nullptr;
}

// +--------------------------------------------------------------------+
// GPU backing helpers
// +--------------------------------------------------------------------+

void Bitmap::EnsureRenderTarget()
{
    if (width < 1 || height < 1)
        return;

    if (RenderTarget && RenderTarget->SizeX == width && RenderTarget->SizeY == height)
        return;

    RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
    if (!RenderTarget) {
        UE_LOG(LogTemp, Error, TEXT("Bitmap::EnsureRenderTarget failed (null RenderTarget) for %hs"), filename);
        return;
    }

    RenderTarget->RenderTargetFormat = RTF_RGBA8;
    RenderTarget->bAutoGenerateMips = false;
    RenderTarget->ClearColor = FLinearColor(0, 0, 0, 0);
    RenderTarget->InitAutoFormat(width, height);
    RenderTarget->UpdateResourceImmediate(true);
}

void Bitmap::DrawBegin(FCanvas*& OutCanvas)
{
    OutCanvas = nullptr;

    EnsureRenderTarget();
    if (!RenderTarget)
    {
        return;
    }

    FTextureRenderTargetResource* RTRes = RenderTarget->GameThread_GetRenderTargetResource();
    if (!RTRes)
    {
        return;
    }

    OutCanvas = new FCanvas(
        RTRes,
        nullptr,
        nullptr,
        GMaxRHIFeatureLevel
    );
}


void Bitmap::DrawEnd(FCanvas*& InOutCanvas)
{
    if (!InOutCanvas) return;

    InOutCanvas->Flush_GameThread();
    delete InOutCanvas;
    InOutCanvas = nullptr;
}

void Bitmap::ClearToTransparent()
{
    EnsureRenderTarget();
    if (!RenderTarget) return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    FCanvasTileItem ClearTile(FVector2D(0.f, 0.f), FVector2D((float)width, (float)height), FColor::Transparent);
    ClearTile.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearTile);

    DrawEnd(Canvas);
    last_modified = GetRealTime();
}

void Bitmap::UploadBGRA(const void* SrcBGRA8, int SrcW, int SrcH, bool bHasAlpha, bool bSRGB)
{
    if (!SrcBGRA8 || SrcW < 1 || SrcH < 1)
        return;

    width = SrcW;
    height = SrcH;
    mapsize = width * height;

    EnsureRenderTarget();
    if (!RenderTarget)
        return;

    UTexture2D* TempTex = UTexture2D::CreateTransient(SrcW, SrcH, PF_B8G8R8A8);
    if (!TempTex)
        return;

    TempTex->SRGB = bSRGB;
    TempTex->NeverStream = true;
    TempTex->MipGenSettings = TMGS_NoMipmaps;

    if (!TempTex->GetPlatformData() || TempTex->GetPlatformData()->Mips.Num() < 1)
        return;

    FTexture2DMipMap& Mip = TempTex->GetPlatformData()->Mips[0];
    void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
    {
        const int32 NumBytes = SrcW * SrcH * 4;
        FMemory::Memcpy(Data, SrcBGRA8, NumBytes);
    }
    Mip.BulkData.Unlock();

    #define UpdateResource UpdateResource
    TempTex->UpdateResource();

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas)
        return;

    // Clear RT:
    {
        FCanvasTileItem ClearTile(FVector2D(0.f, 0.f), FVector2D((float)SrcW, (float)SrcH), FColor::Transparent);
        ClearTile.BlendMode = SE_BLEND_Opaque;
        Canvas->DrawItem(ClearTile);
    }

    // Draw temp texture into RT:
    if (TempTex->GetResource()) {
        FCanvasTileItem Tile(
            FVector2D(0.f, 0.f),
            TempTex->GetResource(),
            FVector2D((float)SrcW, (float)SrcH),
            FLinearColor::White
        );
        Tile.BlendMode = bHasAlpha ? SE_BLEND_Translucent : SE_BLEND_Opaque;
        Canvas->DrawItem(Tile);
    }

    DrawEnd(Canvas);

    // GPU-first: drop CPU buffers by default
    if (ownpix) {
        delete[] pix;   pix = nullptr;
        delete[] hipix; hipix = nullptr;
    }
    ownpix = false;

    alpha_loaded = bHasAlpha;
    last_modified = GetRealTime();
}

// +--------------------------------------------------------------------+
// Non-drawing internals (GPU-first)
// +--------------------------------------------------------------------+

void Bitmap::ClearImage()
{
    if (ownpix) {
        delete[] pix;   pix = nullptr;
        delete[] hipix; hipix = nullptr;
    }

    type = BMP_SOLID;
    width = 0;
    height = 0;
    mapsize = 0;

    ownpix = false;
    alpha_loaded = false;
    texture = false;

    Texture = nullptr;
    RenderTarget = nullptr;

    last_modified = GetRealTime();
}

void Bitmap::CopyBitmap(const Bitmap& rhs)
{
    type = rhs.type;
    width = rhs.width;
    height = rhs.height;
    mapsize = rhs.mapsize;
    alpha_loaded = rhs.alpha_loaded;
    texture = rhs.texture;

    // GPU-first: copy rhs RT by blitting
    RenderTarget = nullptr;
    EnsureRenderTarget();
    if (!RenderTarget) return;

    ClearToTransparent();
    BitBlt(0, 0, rhs, 0, 0, width, height, true);

    // Drop CPU buffers
    if (ownpix) {
        delete[] pix;   pix = nullptr;
        delete[] hipix; hipix = nullptr;
    }
    ownpix = false;

    last_modified = GetRealTime();
}

void Bitmap::CopyImage(int w, int h, BYTE* p, int t)
{
    type = t;

    if (!p || w < 1 || h < 1) {
        ClearImage();
        return;
    }

    // Expand 8-bit indices to grayscale BGRA (palette support can be added later).
    TArray<uint8> BGRA;
    BGRA.SetNumUninitialized(w * h * 4);

    const bool bHasAlpha = (t == BMP_TRANSLUCENT);

    for (int i = 0; i < w * h; ++i) {
        const uint8 idx = p[i];
        BGRA[i * 4 + 0] = idx;                      // B
        BGRA[i * 4 + 1] = idx;                      // G
        BGRA[i * 4 + 2] = idx;                      // R
        BGRA[i * 4 + 3] = bHasAlpha ? idx : 255;    // A (best-effort)
    }

    UploadBGRA(BGRA.GetData(), w, h, bHasAlpha, true);
}

void Bitmap::CopyImage(int w, int h, ColorIndex* p, int t)
{
    // Convenience overload to fix “cannot convert ColorIndex* to BYTE*”
    CopyImage(w, h, (BYTE*)p, t);
}

void Bitmap::CopyHighColorImage(int w, int h, DWORD* p, int t)
{
    type = t;

    if (!p || w < 1 || h < 1) {
        ClearImage();
        return;
    }

    // Assumption: p is packed BGRA8 (PF_B8G8R8A8).
    // If your source is actually ARGB/RGBA, add a swizzle here.
    const bool bHasAlpha = (t == BMP_TRANSLUCENT) || (t == BMP_TRANSPARENT);
    UploadBGRA((const void*)p, w, h, bHasAlpha, true);
}

void Bitmap::CopyAlphaImage(int w, int h, BYTE* a)
{
    if (!a || w < 1 || h < 1)
        return;

    // Best-effort only if we still have CPU hi buffer:
    if (hipix && ownpix && width == w && height == h) {
        type = BMP_TRANSLUCENT;
        alpha_loaded = true;

        for (int i = 0; i < mapsize; ++i) {
            hipix[i].A = a[i];
        }

        UploadBGRA(hipix, w, h, true, true);
        last_modified = GetRealTime();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Bitmap::CopyAlphaImage: no CPU hi buffer (GPU-first). No-op for %hs"), filename);
}

void Bitmap::CopyAlphaRedChannel(int w, int h, DWORD* p)
{
    if (!p || w < 1 || h < 1)
        return;

    if (hipix && ownpix && width == w && height == h) {
        type = BMP_TRANSLUCENT;
        alpha_loaded = true;

        const uint8* Bytes = reinterpret_cast<const uint8*>(p); // BGRA bytes
        for (int i = 0; i < mapsize; ++i) {
            const uint8 R = Bytes[i * 4 + 2]; // BGRA => R at +2
            hipix[i].A = R;
        }

        UploadBGRA(hipix, w, h, true, true);
        last_modified = GetRealTime();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Bitmap::CopyAlphaRedChannel: no CPU hi buffer (GPU-first). No-op for %hs"), filename);
}

void Bitmap::AutoMask(DWORD mask)
{
    // Best-effort only if we still have CPU hi buffer:
    if (!(hipix && ownpix) || mapsize < 1) {
        UE_LOG(LogTemp, Warning, TEXT("Bitmap::AutoMask: no CPU hi buffer (GPU-first). No-op for %hs"), filename);
        return;
    }

    type = BMP_TRANSLUCENT;
    alpha_loaded = true;

    const uint8 MaskB = (uint8)((mask >> 0) & 0xff);
    const uint8 MaskG = (uint8)((mask >> 8) & 0xff);
    const uint8 MaskR = (uint8)((mask >> 16) & 0xff);

    for (int i = 0; i < mapsize; ++i) {
        const FColor& C = hipix[i];
        hipix[i].A = (C.R == MaskR && C.G == MaskG && C.B == MaskB) ? 0 : 255;
    }

    UploadBGRA(hipix, width, height, true, true);
    last_modified = GetRealTime();
}

void Bitmap::FillColor(FColor c)
{
    if (width < 1 || height < 1)
        return;

    EnsureRenderTarget();
    if (!RenderTarget) return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    FCanvasTileItem Tile(FVector2D(0.f, 0.f), FVector2D((float)width, (float)height), c);
    Tile.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(Tile);

    DrawEnd(Canvas);

    alpha_loaded = (c.A < 255) || (type == BMP_TRANSLUCENT);
    last_modified = GetRealTime();
}

void Bitmap::ScaleTo(int NewW, int NewH)
{
    if (NewW < 1 || NewH < 1)
        return;

    if (width < 1 || height < 1) {
        width = NewW;
        height = NewH;
        mapsize = width * height;
        EnsureRenderTarget();
        last_modified = GetRealTime();
        return;
    }

    EnsureRenderTarget();
    if (!RenderTarget) return;

    UTextureRenderTarget2D* OldRT = RenderTarget;
    const int OldW = width;
    const int OldH = height;

    // Create new RT
    RenderTarget = nullptr;
    width = NewW;
    height = NewH;
    mapsize = width * height;

    EnsureRenderTarget();
    if (!RenderTarget) {
        RenderTarget = OldRT;
        width = OldW;
        height = OldH;
        mapsize = width * height;
        return;
    }

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) {
        RenderTarget = OldRT;
        width = OldW;
        height = OldH;
        mapsize = width * height;
        return;
    }

    // Clear new:
    FCanvasTileItem ClearTile(FVector2D(0.f, 0.f), FVector2D((float)NewW, (float)NewH), FColor::Transparent);
    ClearTile.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearTile);

    // Draw old into new scaled:
    UTexture* SrcTex = OldRT;
    if (SrcTex && SrcTex->GetResource()) {
        FCanvasTileItem Tile(FVector2D(0.f, 0.f), SrcTex->GetResource(), FVector2D((float)NewW, (float)NewH), FLinearColor::White);
        Tile.BlendMode = alpha_loaded ? SE_BLEND_Translucent : SE_BLEND_Opaque;
        Canvas->DrawItem(Tile);
    }

    DrawEnd(Canvas);

    last_modified = GetRealTime();
}

void Bitmap::MakeIndexed()
{
    UE_LOG(LogTemp, Warning, TEXT("Bitmap::MakeIndexed: palette/indexed not supported in GPU-first Bitmap (%hs)."), filename);
}

void Bitmap::MakeHighColor()
{
    // Already hi-color on GPU. If we only have CPU pix and no RT, expand/upload.
    if (!RenderTarget && pix && width > 0 && height > 0) {
        CopyImage(width, height, pix, type);
    }
}

void Bitmap::MakeTexture()
{
    if (width < 1 || height < 1) {
        ClearImage();
        return;
    }

    EnsureRenderTarget();
    if (!RenderTarget) return;

    // Starshatter used Game::MaxTexSize/Aspect. Unreal varies per platform;
    // keep conservative defaults (or wire to your project settings).
    const int MaxTexSize = 4096;
    const int MaxAspect = 8;

    int bestW = FindBestTexSize(width, MaxTexSize);
    int bestH = FindBestTexSize(height, MaxTexSize);

    int aspect = 1;
    if (bestW > bestH) {
        aspect = bestW / bestH;
        if (aspect > MaxAspect)
            bestH = bestW / MaxAspect;
    }
    else {
        aspect = bestH / bestW;
        if (aspect > MaxAspect)
            bestW = bestH / MaxAspect;
    }

    if (bestW != width || bestH != height) {
        ScaleTo(bestW, bestH);
    }

    texture = true;
    last_modified = GetRealTime();
}

// +--------------------------------------------------------------------+
// Sampling / per-pixel set (best-effort GPU-first)
// +--------------------------------------------------------------------+

ColorIndex Bitmap::GetIndex(int x, int y) const
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return (ColorIndex)0;

    if (pix)
        return *(pix + y * width + x);

    UE_LOG(LogTemp, Warning, TEXT("Bitmap::GetIndex: GPU readback not enabled; returning 0 for %hs"), filename);
    return (ColorIndex)0;
}

FColor Bitmap::GetColor(int x, int y) const
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return FColor::Black;

    if (hipix)
        return *(hipix + y * width + x);

    UE_LOG(LogTemp, Warning, TEXT("Bitmap::GetColor: GPU readback not enabled; returning Black for %hs"), filename);
    return FColor::Black;
}

void Bitmap::SetIndex(int x, int y, uint8 value)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return;

    // Legacy CPU bitmap path (optional)
    if (pix) {
        pix[y * width + x] = value;
        last_modified = GetRealTime();
        return;
    }

    // GPU-first path: write grayscale color
    SetColor(x, y, FColor(value, value, value, 255));
}

void Bitmap::SetColor(int x, int y, FColor c)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return;

    if (hipix) {
        *(hipix + y * width + x) = c;
        last_modified = GetRealTime();
        return;
    }

    EnsureRenderTarget();
    if (!RenderTarget) return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    FCanvasTileItem Tile(FVector2D((float)x, (float)y), FVector2D(1.f, 1.f), c);
    Tile.BlendMode = (c.A < 255) ? SE_BLEND_Translucent : SE_BLEND_Opaque;
    Canvas->DrawItem(Tile);

    DrawEnd(Canvas);

    alpha_loaded = alpha_loaded || (c.A < 255);
    last_modified = GetRealTime();
}

// Linear-index helpers (fix “SetColor does not take 2 arguments” call sites)
void Bitmap::SetColor(int i, FColor c)
{
    if (width < 1 || height < 1) return;
    if (i < 0 || i >= mapsize) return;

    const int x = i % width;
    const int y = i / width;
    SetColor(x, y, c);
}

// +--------------------------------------------------------------------+
// BitBlt (GPU-first)
// +--------------------------------------------------------------------+

void Bitmap::BitBlt(int x, int y, const Bitmap& srcBmp, int sx, int sy, int w, int h, bool blend)
{
    if (w < 1 || h < 1)
        return;

    if (x >= width || y >= height)
        return;

    if (sx >= srcBmp.Width() || sy >= srcBmp.Height())
        return;

    EnsureRenderTarget();
    if (!RenderTarget) return;

    // Prefer GPU path: src must have RT
    if (!srcBmp.RenderTarget) {
        UE_LOG(LogTemp, Warning, TEXT("Bitmap::BitBlt: src has no RenderTarget (GPU-first). dst=%hs"), filename);
        return;
    }

    // Compute clipped regions
    int dstX = x;
    int dstY = y;
    int srcX = sx;
    int srcY = sy;
    int bltW = w;
    int bltH = h;

    // Clip left/top
    if (dstX < 0) { srcX += -dstX; bltW -= -dstX; dstX = 0; }
    if (dstY < 0) { srcY += -dstY; bltH -= -dstY; dstY = 0; }

    // Clip right/bottom against dst
    bltW = FMath::Min(bltW, width - dstX);
    bltH = FMath::Min(bltH, height - dstY);

    // Clip right/bottom against src
    bltW = FMath::Min(bltW, srcBmp.Width() - srcX);
    bltH = FMath::Min(bltH, srcBmp.Height() - srcY);

    if (bltW < 1 || bltH < 1)
        return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    UTexture* SrcTex = srcBmp.RenderTarget;
    FTexture* SrcRes = (SrcTex ? SrcTex->GetResource() : nullptr);
    if (!SrcRes) {
        DrawEnd(Canvas);
        return;
    }

    const float U0 = (float)srcX / (float)srcBmp.Width();
    const float V0 = (float)srcY / (float)srcBmp.Height();
    const float U1 = (float)(srcX + bltW) / (float)srcBmp.Width();
    const float V1 = (float)(srcY + bltH) / (float)srcBmp.Height();

    FCanvasTileItem Tile(FVector2D((float)dstX, (float)dstY), SrcRes, FVector2D((float)bltW, (float)bltH), FLinearColor::White);
    Tile.UV0 = FVector2D(U0, V0);
    Tile.UV1 = FVector2D(U1, V1);

    const bool bSrcHasAlpha = srcBmp.alpha_loaded || srcBmp.type == BMP_TRANSLUCENT || srcBmp.type == BMP_TRANSPARENT;
    const bool bDoAlpha = blend || bSrcHasAlpha;
    Tile.BlendMode = bDoAlpha ? SE_BLEND_Translucent : SE_BLEND_Opaque;

    Canvas->DrawItem(Tile);
    DrawEnd(Canvas);

    alpha_loaded = alpha_loaded || bDoAlpha;
    last_modified = GetRealTime();
}

// +--------------------------------------------------------------------+
// ClipLine (unchanged from original logic; uses UE math only)
// +--------------------------------------------------------------------+

bool Bitmap::ClipLine(int& x1, int& y1, int& x2, int& y2)
{
    if (x1 == x2) {
    clip_vertical:
        sort_i(y1, y2);
        if (x1 < 0 || x1 >= width) return false;
        if (y1 < 0) y1 = 0;
        if (y2 >= height) y2 = height;
        return true;
    }

    if (y1 == y2) {
    clip_horizontal:
        sort_i(x1, x2);
        if (y1 < 0 || y1 >= height) return false;
        if (x1 < 0) x1 = 0;
        if (x2 > width) x2 = width;
        return true;
    }

    if (x1 > x2) {
        swap_i(x1, x2);
        swap_i(y1, y2);
    }

    double m = (double)(y2 - y1) / (double)(x2 - x1);
    double b = (double)y1 - (m * x1);

    if (x1 < 0) { x1 = 0; y1 = (int)b; }
    if (x1 >= width)  return false;
    if (x2 < 0)       return false;
    if (x2 > width - 1) { x2 = width - 1; y2 = (int)(m * x2 + b); }

    if (y1 < 0 && y2 < 0) return false;
    if (y1 >= height && y2 >= height) return false;

    if (y1 < 0) { y1 = 0; x1 = (int)(-b / m); }
    if (y1 >= height) { y1 = height - 1; x1 = (int)((y1 - b) / m); }
    if (y2 < 0) { y2 = 0; x2 = (int)(-b / m); }
    if (y2 >= height) { y2 = height - 1; x2 = (int)((y2 - b) / m); }

    if (x1 == x2) goto clip_vertical;
    if (y1 == y2) goto clip_horizontal;

    return true;
}

bool Bitmap::ClipLine(double& x1, double& y1, double& x2, double& y2)
{
    if (x1 == x2) {
    clip_vertical:
        sort_d(y1, y2);
        if (x1 < 0 || x1 >= width) return false;
        if (y1 < 0) y1 = 0;
        if (y2 >= height) y2 = height;
        return true;
    }

    if (y1 == y2) {
    clip_horizontal:
        sort_d(x1, x2);
        if (y1 < 0 || y1 >= height) return false;
        if (x1 < 0) x1 = 0;
        if (x2 > width) x2 = width;
        return true;
    }

    if (x1 > x2) {
        swap_d(x1, x2);
        swap_d(y1, y2);
    }

    double m = (double)(y2 - y1) / (double)(x2 - x1);
    double b = (double)y1 - (m * x1);

    if (x1 < 0) { x1 = 0; y1 = b; }
    if (x1 >= width)  return false;
    if (x2 < 0)       return false;
    if (x2 > width - 1) { x2 = width - 1; y2 = (m * x2 + b); }

    if (y1 < 0 && y2 < 0) return false;
    if (y1 >= height && y2 >= height) return false;

    if (y1 < 0) { y1 = 0; x1 = (-b / m); }
    if (y1 >= height) { y1 = height - 1; x1 = ((y1 - b) / m); }
    if (y2 < 0) { y2 = 0; x2 = (-b / m); }
    if (y2 >= height) { y2 = height - 1; x2 = ((y2 - b) / m); }

    if (x1 == x2) goto clip_vertical;
    if (y1 == y2) goto clip_horizontal;

    return true;
}

// +--------------------------------------------------------------------+
// Drawing API (UE primitives; regenerated)
// +--------------------------------------------------------------------+

void Bitmap::DrawLine(int x1, int y1, int x2, int y2, FColor color)
{
    EnsureRenderTarget();
    if (!RenderTarget) return;

    // Clip for safety (retain original semantics)
    int cx1 = x1, cy1 = y1, cx2 = x2, cy2 = y2;
    if (!ClipLine(cx1, cy1, cx2, cy2))
        return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    FCanvasLineItem Line(FVector2D((float)cx1, (float)cy1), FVector2D((float)cx2, (float)cy2));
    Line.SetColor(FLinearColor(color));
    Canvas->DrawItem(Line);

    DrawEnd(Canvas);

    alpha_loaded = alpha_loaded || (color.A < 255);
    last_modified = GetRealTime();
}

void Bitmap::DrawRect(int x1, int y1, int x2, int y2, FColor color)
{
    sort_i(x1, x2);
    sort_i(y1, y2);

    int fw = x2 - x1;
    int fh = y2 - y1;
    if (fw == 0 || fh == 0) return;

    EnsureRenderTarget();
    if (!RenderTarget) return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    const int left = x1;
    const int right = x2;
    const int top = y1;
    const int bottom = y2;

    auto DrawH = [&](int px, int py, int w)
        {
            if (w <= 0) return;
            FCanvasTileItem T(FVector2D((float)px, (float)py), FVector2D((float)w, 1.f), color);
            T.BlendMode = (color.A < 255) ? SE_BLEND_Translucent : SE_BLEND_Opaque;
            Canvas->DrawItem(T);
        };

    auto DrawV = [&](int px, int py, int h)
        {
            if (h <= 0) return;
            FCanvasTileItem T(FVector2D((float)px, (float)py), FVector2D(1.f, (float)h), color);
            T.BlendMode = (color.A < 255) ? SE_BLEND_Translucent : SE_BLEND_Opaque;
            Canvas->DrawItem(T);
        };

    // Top
    if (top >= 0 && top < height) {
        const int sx = FMath::Max(0, left);
        const int ex = FMath::Min(width, right);
        DrawH(sx, top, ex - sx);
    }

    // Bottom
    if (bottom >= 0 && bottom < height) {
        const int sx = FMath::Max(0, left);
        const int ex = FMath::Min(width, right);
        DrawH(sx, bottom, ex - sx);
    }

    // Left
    if (left >= 0 && left < width) {
        const int sy = FMath::Max(0, top);
        const int ey = FMath::Min(height, bottom);
        DrawV(left, sy, ey - sy);
    }

    // Right
    if (right >= 0 && right < width) {
        const int sy = FMath::Max(0, top);
        const int ey = FMath::Min(height, bottom);
        DrawV(right, sy, ey - sy);
    }

    DrawEnd(Canvas);

    alpha_loaded = alpha_loaded || (color.A < 255);
    last_modified = GetRealTime();
}

void Bitmap::DrawRect(const FIntRect& r, FColor color)
{
    if (r.Width() == 0 || r.Height() == 0) return;
    DrawRect(r.Min.X, r.Min.Y, r.Max.X, r.Max.Y, color);
}

void Bitmap::FillRect(int x1, int y1, int x2, int y2, FColor color)
{
    if (width < 1 || height < 1) return;

    // Clip
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > width)  x2 = width;
    if (y2 > height) y2 = height;

    const int fw = x2 - x1;
    const int fh = y2 - y1;
    if (fw <= 0 || fh <= 0) return;

    EnsureRenderTarget();
    if (!RenderTarget) return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    FCanvasTileItem Tile(FVector2D((float)x1, (float)y1), FVector2D((float)fw, (float)fh), color);
    Tile.BlendMode = (color.A < 255) ? SE_BLEND_Translucent : SE_BLEND_Opaque;
    Canvas->DrawItem(Tile);

    DrawEnd(Canvas);

    alpha_loaded = alpha_loaded || (color.A < 255);
    last_modified = GetRealTime();
}

void Bitmap::FillRect(const FIntRect& r, FColor color)
{
    FillRect(r.Min.X, r.Min.Y, r.Max.X, r.Max.Y, color);
}

void Bitmap::DrawEllipse(int x1, int y1, int x2, int y2, FColor color, BYTE quad)
{
    // Preserve original midpoint algorithm, but draw points using UE tiles.
    sort_i(x1, x2);
    sort_i(y1, y2);

    const int fw = x2 - x1;
    const int fh = y2 - y1;
    if (fw < 1 || fh < 1) return;

    if (x1 >= width || x2 < 0) return;
    if (y1 >= height || y2 < 0) return;

    double a = fw / 2.0;
    double b = fh / 2.0;
    double a2 = a * a;
    double b2 = b * b;

    int x = 0;
    int y = (int)b;
    int x0 = (x1 + x2) / 2;
    int y0 = (y1 + y2) / 2;

    // Clip super-giant ellipses (retain original)
    if (x1 < 0 && y1 < 0 && x2 > width && y2 > height) {
        double r2 = (a2 < b2) ? a2 : b2;
        if (r2 > 32 * height) return;

        double ul = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
        double ur = (x2 - x0) * (x2 - x0) + (y1 - y0) * (y1 - y0);
        double ll = (x1 - x0) * (x1 - x0) + (y2 - y0) * (y2 - y0);
        double lr = (x2 - x0) * (x2 - x0) + (y2 - y0) * (y2 - y0);

        if (ul > r2 && ur > r2 && ll > r2 && lr > r2) return;
    }

    DrawEllipsePoints(x0, y0, x, y, color, quad);

    // region 1
    double d1 = (b2)-(a2 * b) + (0.25 * a2);
    while ((a2) * (y - 0.5) > (b2) * (x + 1)) {
        if (d1 < 0)
            d1 += b2 * (2 * x + 3);
        else {
            d1 += b2 * (2 * x + 3) + a2 * (-2 * y + 2);
            y--;
        }
        x++;
        DrawEllipsePoints(x0, y0, x, y, color, quad);
    }

    // region 2
    double d2 = b2 * (x + 0.5) * (x + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
    while (y > 0) {
        if (d2 < 0) {
            d2 += b2 * (2 * x + 2) + a2 * (-2 * y + 3);
            x++;
        }
        else
            d2 += a2 * (-2 * y + 3);
        y--;
        DrawEllipsePoints(x0, y0, x, y, color, quad);
    }

    alpha_loaded = alpha_loaded || (color.A < 255);
    last_modified = GetRealTime();
}

void Bitmap::DrawEllipsePoints(int x0, int y0, int x, int y, FColor c, BYTE quad)
{
    EnsureRenderTarget();
    if (!RenderTarget) return;

    const int left = x0 - x;
    const int right = x0 + x + 1;
    const int top = y0 - y;
    const int bottom = y0 + y + 1;

    if (left >= width || right < 0) return;
    if (top >= height || bottom < 0) return;

    FCanvas* Canvas = nullptr;
    DrawBegin(Canvas);
    if (!Canvas) return;

    auto DrawPoint = [&](int px, int py)
        {
            if (px < 0 || py < 0 || px >= width || py >= height) return;
            FCanvasTileItem P(FVector2D((float)px, (float)py), FVector2D(1.f, 1.f), c);
            P.BlendMode = (c.A < 255) ? SE_BLEND_Translucent : SE_BLEND_Opaque;
            Canvas->DrawItem(P);
        };

    if ((quad & 1) && left >= 0 && top >= 0)            DrawPoint(left, top);
    if ((quad & 2) && right < width && top >= 0)        DrawPoint(right, top);
    if ((quad & 4) && left >= 0 && bottom < height)     DrawPoint(left, bottom);
    if ((quad & 8) && right < width && bottom < height) DrawPoint(right, bottom);

    DrawEnd(Canvas);
}

// +--------------------------------------------------------------------+
// Default bitmap + cache (kept structurally; GPU-first adjustments)
// +--------------------------------------------------------------------+

Bitmap* Bitmap::Default()
{
    static Bitmap def;

    if (!def.width) {
        def.width = def.height = 64;
        def.mapsize = 64 * 64;
        def.type = BMP_SOLID;

        // Procedural grayscale like original default, but upload as BGRA.
        TArray<uint8> BGRA;
        BGRA.SetNumUninitialized(64 * 64 * 4);

        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                const double dx = (x - 32.0);
                const double dy = (y - 32.0);
                double distance = FMath::Sqrt(dx * dx + dy * dy);
                if (distance > 31.0) distance = 31.0;

                uint8 v = (uint8)(24 + (uint8)distance);
                if (x == 0 || y == 0) v = 255;
                else if (x == 32 || y == 32) v = 251;

                const int i = (y * 64 + x) * 4;
                BGRA[i + 0] = v;
                BGRA[i + 1] = v;
                BGRA[i + 2] = v;
                BGRA[i + 3] = 255;
            }
        }

        def.UploadBGRA(BGRA.GetData(), 64, 64, false, true);
    }

    return &def;
}

// Use global namespace to avoid any UE name collisions:
static ::List<Bitmap> bitmap_cache;

Bitmap* Bitmap::GetBitmapByID(HANDLE bmp_id)
{
    for (int i = 0; i < bitmap_cache.size(); i++) {
        if (bitmap_cache[i]->Handle() == bmp_id)
            return bitmap_cache[i];
    }
    return nullptr;
}

Bitmap* Bitmap::CheckCache(const char* in_filename)
{
    if (!in_filename) return nullptr;

    for (int i = 0; i < bitmap_cache.size(); i++) {
        if (!FCStringAnsi::Stricmp(bitmap_cache[i]->GetFilename(), in_filename))
            return bitmap_cache[i];
    }
    return nullptr;
}

void Bitmap::AddToCache(Bitmap* bmp)
{
    bitmap_cache.append(bmp);
}

void Bitmap::CacheUpdate()
{
    for (int i = 0; i < bitmap_cache.size(); i++) {
        Bitmap* bmp = bitmap_cache[i];
        if (bmp && bmp->IsTexture())
            bmp->MakeTexture();
    }
}

void Bitmap::ClearCache()
{
    bitmap_cache.destroy();
}

DWORD Bitmap::CacheMemoryFootprint()
{
    DWORD result = sizeof(bitmap_cache);
    result += bitmap_cache.size() * sizeof(Bitmap*);

    for (int i = 0; i < bitmap_cache.size(); i++) {
        Bitmap* bmp = bitmap_cache[i];
        if (!bmp) continue;

        if (bmp->pix)
            result += bmp->mapsize * (DWORD)sizeof(ColorIndex);

        if (bmp->hipix)
            result += bmp->mapsize * (DWORD)sizeof(FColor);
    }

    return result;
}

// +--------------------------------------------------------------------+
// FindBestTexSize (unchanged logic)
// +--------------------------------------------------------------------+

static int FindBestTexSize(int n, int max_size)
{
    int delta = 100000;
    int best = 1;

    for (int i = 0; i < 12; i++) {
        int size = 1 << i;
        if (size > max_size) break;

        int dx = FMath::Abs(n - size);
        if (size < n) dx *= 4;

        if (dx < delta) {
            delta = dx;
            best = size;
        }
    }

    return best;
}
