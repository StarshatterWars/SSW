/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    nGenEx.lib
    FILE:         View.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Combined View implementation (Window + View).
*/

#include "View.h"

#include "Bitmap.h"
#include "Fix.h"
#include "SystemFont.h"
#include "Polygon.h"
#include "Screen.h"
#include "Video.h"
#include "GameStructs.h"

#include "CoreMinimal.h"
#include "Math/Color.h"
#include "Math/Vector.h"

static VertexSet vset4(4);

// +--------------------------------------------------------------------+

View::View(Screen* s, int ax, int ay, int aw, int ah)
    : rect(ax, ay, aw, ah)
    , screen(s)
    , window(nullptr)
    , parent(nullptr)
    , shown(true)
    , font(nullptr)
    , children(nullptr)
{
    // If your Screen can provide a Window, bind it here.
    // If not, leave it null and set it later via SetWindow(...)
    //
    // window = (screen ? screen->GetWindow() : nullptr);   // <-- only if this exists
}

View::View(View* inParent, int ax, int ay, int aw, int ah)
    : rect(ax, ay, aw, ah)
    , screen(inParent ? inParent->GetScreen() : nullptr)
    , window(inParent ? inParent->GetWindow() : nullptr)   // inherit window from parent
    , parent(inParent)
    , shown(true)
    , font(nullptr)
    , children(nullptr)
{
    if (parent)
        parent->AddView(this);
}

View::View(Screen* InScreen, View* InParent, int ax, int ay, int aw, int ah)
    : rect(ax, ay, aw, ah)
    , ViewRectPx(ax, ay, aw, ah)
    , screen(InScreen)
    , window(nullptr)
    , parent(InParent)
    , shown(true)
{
    // Inherit render context from parent if present:
    if (parent)
    {
        window = parent->GetWindow();   // your View stores window as View*
        screen = parent->GetScreen();   // parent already knows screen
        parent->AddView(this);
    }
    else
    {
        // Root view: render context will be resolved by Screen in your existing ctor,
        // or you can leave window null and set it later.
        // If your Screen has a root window/view, bind it here if you want.
    }
}

View::~View()
{
    // IMPORTANT:
    // Destroy whatever you actually own.
    // If you are now using the opaque children holder, delete it here.
    if (children)
    {
        //delete children;
        //children = nullptr;
    }

    // If you still truly use view_list and it needs explicit destroy:
    // view_list.destroy();
}

// +--------------------------------------------------------------------+

bool View::AddView(View* v)
{
    if (!v) return false;

    if (!view_list.contains(v))
        view_list.append(v);

    v->parent = this;
    if (!v->screen)
        v->screen = screen;

    return true;
}

bool View::DelView(View* v)
{
    if (!v) return false;

    View* removed = view_list.remove(v);
    if (removed == v)
    {
        v->parent = nullptr;
        return true;
    }

    return false;
}

// +--------------------------------------------------------------------+

void View::MoveTo(const Rect& r)
{
    if (rect.x == r.x &&
        rect.y == r.y &&
        rect.w == r.w &&
        rect.h == r.h)
        return;

    rect = r;

    // Notify self + children (legacy expectation)
    OnWindowMove();

    ListIter<View> v = view_list;
    while (++v)
        v->OnWindowMove();
}

// +--------------------------------------------------------------------+

void View::Paint()
{
    if (!shown)
        return;

    // This replaces Window::Paint iterating views:
    // a view paints itself, then its children.
    Refresh();

    ListIter<View> v = view_list;
    while (++v)
        v->Paint();
}

// +--------------------------------------------------------------------+

static inline void swap(int& a, int& b) { int tmp = a; a = b; b = tmp; }
static inline void sort(int& a, int& b) { if (a > b) swap(a, b); }
static inline void swap(double& a, double& b) { double tmp = a; a = b; b = tmp; }
static inline void sort(double& a, double& b) { if (a > b) swap(a, b); }

// UE helper: FColor -> Starshatter DWORD ARGB (matches Color packing in your Color.h)
static inline DWORD ToStarColorARGB(const FColor& C)
{
    return ((DWORD)C.R << 16) | ((DWORD)C.G << 8) | ((DWORD)C.B) | ((DWORD)C.A << 24);
}

// +--------------------------------------------------------------------+

Rect View::ClipRect(const Rect& r)
{
    Rect clip_rect = r;

    clip_rect.x += rect.x;
    clip_rect.y += rect.y;

    if (clip_rect.x < rect.x) {
        clip_rect.w -= rect.x - clip_rect.x;
        clip_rect.x = rect.x;
    }

    if (clip_rect.y < rect.y) {
        clip_rect.h -= rect.y - clip_rect.y;
        clip_rect.y = rect.y;
    }

    if (clip_rect.x + clip_rect.w > rect.x + rect.w)
        clip_rect.w = rect.x + rect.w - clip_rect.x;

    if (clip_rect.y + clip_rect.h > rect.y + rect.h)
        clip_rect.h = rect.y + rect.h - clip_rect.y;

    return clip_rect;
}

// +--------------------------------------------------------------------+

bool View::ClipLine(int& x1, int& y1, int& x2, int& y2)
{
    // vertical lines:
    if (x1 == x2) {
    clip_vertical:
        sort(y1, y2);
        if (x1 < 0 || x1 >= rect.w) return false;
        if (y1 < 0) y1 = 0;
        if (y2 >= rect.h) y2 = rect.h;
        return true;
    }

    // horizontal lines:
    if (y1 == y2) {
    clip_horizontal:
        sort(x1, x2);
        if (y1 < 0 || y1 >= rect.h) return false;
        if (x1 < 0) x1 = 0;
        if (x2 > rect.w) x2 = rect.w;
        return true;
    }

    // sort left to right:
    if (x1 > x2) {
        swap(x1, x2);
        swap(y1, y2);
    }

    double m = (double)(y2 - y1) / (double)(x2 - x1);
    double b = (double)y1 - (m * x1);

    // clip:
    if (x1 < 0) { x1 = 0; y1 = (int)b; }
    if (x1 >= rect.w)  return false;
    if (x2 < 0)        return false;
    if (x2 > rect.w - 1) { x2 = rect.w - 1; y2 = (int)(m * x2 + b); }

    if (y1 < 0 && y2 < 0)             return false;
    if (y1 >= rect.h && y2 >= rect.h) return false;

    if (y1 < 0) { y1 = 0; x1 = (int)(-b / m); }
    if (y1 >= rect.h) { y1 = rect.h - 1; x1 = (int)((y1 - b) / m); }
    if (y2 < 0) { y2 = 0; x2 = (int)(-b / m); }
    if (y2 >= rect.h) { y2 = rect.h - 1; x2 = (int)((y2 - b) / m); }

    if (x1 == x2) goto clip_vertical;
    if (y1 == y2) goto clip_horizontal;

    return true;
}

// +--------------------------------------------------------------------+

bool View::ClipLine(double& x1, double& y1, double& x2, double& y2)
{
    // vertical lines:
    if (x1 == x2) {
    clip_vertical:
        sort(y1, y2);
        if (x1 < 0 || x1 >= rect.w) return false;
        if (y1 < 0) y1 = 0;
        if (y2 >= rect.h) y2 = rect.h;
        return true;
    }

    // horizontal lines:
    if (y1 == y2) {
    clip_horizontal:
        sort(x1, x2);
        if (y1 < 0 || y1 >= rect.h) return false;
        if (x1 < 0) x1 = 0;
        if (x2 > rect.w) x2 = rect.w;
        return true;
    }

    // sort left to right:
    if (x1 > x2) {
        swap(x1, x2);
        swap(y1, y2);
    }

    double m = (y2 - y1) / (x2 - x1);
    double b = y1 - (m * x1);

    // clip:
    if (x1 < 0) { x1 = 0; y1 = b; }
    if (x1 >= rect.w)  return false;
    if (x2 < 0)        return false;
    if (x2 > rect.w - 1) { x2 = rect.w - 1; y2 = (m * x2 + b); }

    if (y1 < 0 && y2 < 0)             return false;
    if (y1 >= rect.h && y2 >= rect.h) return false;

    if (y1 < 0) { y1 = 0; x1 = (-b / m); }
    if (y1 >= rect.h) { y1 = rect.h - 1; x1 = ((y1 - b) / m); }
    if (y2 < 0) { y2 = 0; x2 = (-b / m); }
    if (y2 >= rect.h) { y2 = rect.h - 1; x2 = ((y2 - b) / m); }

    if (x1 == x2) goto clip_vertical;
    if (y1 == y2) goto clip_horizontal;

    return true;
}

void
View::DrawLine(int x1, int y1, int x2, int y2, const FColor& color, int blend)
{
    if (!screen || !screen->GetVideo()) return;

    if (ClipLine(x1, y1, x2, y2)) {
        float points[4];

        points[0] = (float)(rect.x + x1);
        points[1] = (float)(rect.y + y1);
        points[2] = (float)(rect.x + x2);
        points[3] = (float)(rect.y + y2);

        Video* video = screen->GetVideo();
        video->DrawScreenLines(1, points, color, blend);
    }
}

// +--------------------------------------------------------------------+

void
View::DrawRect(int x1, int y1, int x2, int y2, const FColor& color, int blend)
{
    if (!screen || !screen->GetVideo()) return;

    sort(x1, x2);
    sort(y1, y2);

    if (x1 > rect.w || x2 < 0 || y1 > rect.h || y2 < 0)
        return;

    float points[16];

    points[0] = (float)(rect.x + x1);
    points[1] = (float)(rect.y + y1);
    points[2] = (float)(rect.x + x2);
    points[3] = (float)(rect.y + y1);

    points[4] = (float)(rect.x + x2);
    points[5] = (float)(rect.y + y1);
    points[6] = (float)(rect.x + x2);
    points[7] = (float)(rect.y + y2);

    points[8] = (float)(rect.x + x2);
    points[9] = (float)(rect.y + y2);
    points[10] = (float)(rect.x + x1);
    points[11] = (float)(rect.y + y2);

    points[12] = (float)(rect.x + x1);
    points[13] = (float)(rect.y + y2);
    points[14] = (float)(rect.x + x1);
    points[15] = (float)(rect.y + y1);

    Video* video = screen->GetVideo();
    video->DrawScreenLines(4, points, color, blend);
}

void
View::DrawRect(const Rect& r, const FColor& color, int blend)
{
    DrawRect(r.x, r.y, r.x + r.w, r.y + r.h, color, blend);
}

// +--------------------------------------------------------------------+

void
View::FillRect(int x1, int y1, int x2, int y2, const FColor& color, int blend)
{
    if (!screen)
        return;

    Video* video = screen->GetVideo();
    if (!video)
        return;

    sort(x1, x2);
    sort(y1, y2);

    if (x1 > rect.w || x2 < 0 || y1 > rect.h || y2 < 0)
        return;

    vset4.space = VertexSet::SCREEN_SPACE;

    for (int i = 0; i < 4; i++) {
        // VertexSet::diffuse is FColor* in your UE port:
        vset4.diffuse[i] = color;

        // If specular is DWORD*, keep it DWORD:
        // (If specular is also FColor*, change this to FColor::Black)
        vset4.specular[i] = FColor::Black;
    }

    vset4.s_loc[0].X = (float)(rect.x + x1) - 0.5f;
    vset4.s_loc[0].Y = (float)(rect.y + y1) - 0.5f;
    vset4.s_loc[0].Z = 0.0f;
    vset4.rw[0] = 1.0f;
    vset4.tu[0] = 0.0f;
    vset4.tv[0] = 0.0f;

    vset4.s_loc[1].X = (float)(rect.x + x2) - 0.5f;
    vset4.s_loc[1].Y = (float)(rect.y + y1) - 0.5f;
    vset4.s_loc[1].Z = 0.0f;
    vset4.rw[1] = 1.0f;
    vset4.tu[1] = 1.0f;
    vset4.tv[1] = 0.0f;

    vset4.s_loc[2].X = (float)(rect.x + x2) - 0.5f;
    vset4.s_loc[2].Y = (float)(rect.y + y2) - 0.5f;
    vset4.s_loc[2].Z = 0.0f;
    vset4.rw[2] = 1.0f;
    vset4.tu[2] = 1.0f;
    vset4.tv[2] = 1.0f;

    vset4.s_loc[3].X = (float)(rect.x + x1) - 0.5f;
    vset4.s_loc[3].Y = (float)(rect.y + y2) - 0.5f;
    vset4.s_loc[3].Z = 0.0f;
    vset4.rw[3] = 1.0f;
    vset4.tu[3] = 0.0f;
    vset4.tv[3] = 1.0f;

    Poly poly(0);
    poly.nverts = 4;
    poly.vertex_set = &vset4;
    poly.verts[0] = 0;
    poly.verts[1] = 1;
    poly.verts[2] = 2;
    poly.verts[3] = 3;

    video->DrawScreenPolys(1, &poly, blend);
}

void
View::FillRect(const Rect& r, const FColor& color, int blend)
{
    FillRect(r.x, r.y, r.x + r.w, r.y + r.h, color, blend);
}

// +--------------------------------------------------------------------+

void
View::DrawLines(int nPts, const FVector* pts, const FColor& color, int blend)
{
    if (nPts < 2 || nPts > 16)
        return;

    if (!screen || !screen->GetVideo())
        return;

    float f[64];
    int   n = 0;

    for (int i = 0; i < nPts - 1; i++) {
        f[n++] = (float)rect.x + (float)pts[i].X;
        f[n++] = (float)rect.y + (float)pts[i].Y;
        f[n++] = (float)rect.x + (float)pts[i + 1].X;
        f[n++] = (float)rect.y + (float)pts[i + 1].Y;
    }

    Video* video = screen->GetVideo();
    video->DrawScreenLines(nPts - 1, f, color, blend);
}

void
View::DrawPoly(int nPts, const FVector* pts, const FColor& color, int blend)
{
    if (nPts < 3 || nPts > 8)
        return;

    if (!screen || !screen->GetVideo())
        return;

    float f[32];
    int   n = 0;

    for (int i = 0; i < nPts - 1; i++) {
        f[n++] = (float)rect.x + (float)pts[i].X;
        f[n++] = (float)rect.y + (float)pts[i].Y;
        f[n++] = (float)rect.x + (float)pts[i + 1].X;
        f[n++] = (float)rect.y + (float)pts[i + 1].Y;
    }

    f[n++] = (float)rect.x + (float)pts[nPts - 1].X;
    f[n++] = (float)rect.y + (float)pts[nPts - 1].Y;
    f[n++] = (float)rect.x + (float)pts[0].X;
    f[n++] = (float)rect.y + (float)pts[0].Y;

    Video* video = screen->GetVideo();
    video->DrawScreenLines(nPts, f, color, blend);
}

void
View::FillPoly(int nPts, const FVector* pts, const FColor& color, int blend)
{
    if (nPts < 3 || nPts > 4)
        return;

    if (!screen)
        return;

    Video* video = screen->GetVideo();
    if (!video)
        return;

    vset4.space = VertexSet::SCREEN_SPACE;

    for (int i = 0; i < nPts; i++) {
        // VertexSet::diffuse is FColor*
        vset4.diffuse[i] = color;

        // Screen-space vertex position
        vset4.s_loc[i].X = (float)(rect.x + (int)pts[i].X) - 0.5f;
        vset4.s_loc[i].Y = (float)(rect.y + (int)pts[i].Y) - 0.5f;
        vset4.s_loc[i].Z = 0.0f;

        vset4.rw[i] = 1.0f;
        vset4.tu[i] = 0.0f;
        vset4.tv[i] = 0.0f;

        if (vset4.specular)
            vset4.specular[i] = FColor::Black;
    }

    Poly poly(0);
    poly.nverts = nPts;
    poly.vertex_set = &vset4;

    poly.verts[0] = 0;
    poly.verts[1] = 1;
    poly.verts[2] = 2;
    poly.verts[3] = 3;

    video->DrawScreenPolys(1, &poly, blend);
}

// +--------------------------------------------------------------------+

void
View::DrawBitmap(int x1, int y1, int x2, int y2, Bitmap* img, int blend)
{
    Rect clip_rect;
    clip_rect.w = rect.w;
    clip_rect.h = rect.h;

    ClipBitmap(x1, y1, x2, y2, img, FColor::White, blend, clip_rect);
}

void
View::FadeBitmap(int x1, int y1, int x2, int y2, Bitmap* img, const FColor& c, int blend)
{
    Rect clip_rect;
    clip_rect.w = rect.w;
    clip_rect.h = rect.h;

    ClipBitmap(x1, y1, x2, y2, img, c, blend, clip_rect);
}

void
View::ClipBitmap(int x1, int y1, int x2, int y2, Bitmap* img, const FColor& c, int blend, const Rect& clip_rect)
{
    if (!screen || !img)
        return;

    Video* video = screen->GetVideo();
    if (!video)
        return;

    Rect clip = clip_rect;

    // clip the clip rect to the window rect:
    if (clip.x < 0) {
        clip.w -= clip.x;
        clip.x = 0;
    }

    if (clip.x + clip.w > rect.w) {
        clip.w -= (clip.x + clip.w - rect.w);
    }

    if (clip.y < 0) {
        clip.h -= clip.y;
        clip.y = 0;
    }

    if (clip.y + clip.h > rect.h) {
        clip.h -= (clip.y + clip.h - rect.h);
    }

    // now clip the bitmap to the validated clip rect:
    sort(x1, x2);
    sort(y1, y2);

    if (x1 > clip.x + clip.w || x2 < clip.x || y1 > clip.y + clip.h || y2 < clip.y)
        return;

    vset4.space = VertexSet::SCREEN_SPACE;

    // VertexSet::diffuse is FColor* in your UE port:
    for (int i = 0; i < 4; i++) {
        vset4.diffuse[i] = c;

        if (vset4.specular)
            vset4.specular[i] = FColor::Black;
    }

    float u1 = 0.0f;
    float u2 = 1.0f;
    float v1 = 0.0f;
    float v2 = 1.0f;

    const float iw = (float)(x2 - x1);
    const float ih = (float)(y2 - y1);

    const int x3 = clip.x + clip.w;
    const int y3 = clip.y + clip.h;

    if (iw <= 0.0f || ih <= 0.0f)
        return;

    if (x1 < clip.x) {
        u1 = (clip.x - x1) / iw;
        x1 = clip.x;
    }

    if (x2 > x3) {
        u2 = 1.0f - (x2 - x3) / iw;
        x2 = x3;
    }

    if (y1 < clip.y) {
        v1 = (clip.y - y1) / ih;
        y1 = clip.y;
    }

    if (y2 > y3) {
        v2 = 1.0f - (y2 - y3) / ih;
        y2 = y3;
    }

    vset4.s_loc[0].X = (float)(rect.x + x1) - 0.5f;
    vset4.s_loc[0].Y = (float)(rect.y + y1) - 0.5f;
    vset4.s_loc[0].Z = 0.0f;
    vset4.rw[0] = 1.0f;
    vset4.tu[0] = u1;
    vset4.tv[0] = v1;

    vset4.s_loc[1].X = (float)(rect.x + x2) - 0.5f;
    vset4.s_loc[1].Y = (float)(rect.y + y1) - 0.5f;
    vset4.s_loc[1].Z = 0.0f;
    vset4.rw[1] = 1.0f;
    vset4.tu[1] = u2;
    vset4.tv[1] = v1;

    vset4.s_loc[2].X = (float)(rect.x + x2) - 0.5f;
    vset4.s_loc[2].Y = (float)(rect.y + y2) - 0.5f;
    vset4.s_loc[2].Z = 0.0f;
    vset4.rw[2] = 1.0f;
    vset4.tu[2] = u2;
    vset4.tv[2] = v2;

    vset4.s_loc[3].X = (float)(rect.x + x1) - 0.5f;
    vset4.s_loc[3].Y = (float)(rect.y + y2) - 0.5f;
    vset4.s_loc[3].Z = 0.0f;
    vset4.rw[3] = 1.0f;
    vset4.tu[3] = u1;
    vset4.tv[3] = v2;

    Material mtl;
    mtl.tex_diffuse = img;

    Poly poly(0);
    poly.nverts = 4;
    poly.vertex_set = &vset4;
    poly.material = &mtl;
    poly.verts[0] = 0;
    poly.verts[1] = 1;
    poly.verts[2] = 2;
    poly.verts[3] = 3;

    video->SetRenderState(Video::TEXTURE_WRAP, 0);
    video->DrawScreenPolys(1, &poly, blend);
    video->SetRenderState(Video::TEXTURE_WRAP, 1);
}

// +--------------------------------------------------------------------+

void
View::TileBitmap(int x1, int y1, int x2, int y2, Bitmap* img, int blend)
{
    if (!screen)
        return;

    Video* video = screen->GetVideo();
    if (!video)
        return;

    if (!img || !img->Width() || !img->Height())
        return;

    vset4.space = VertexSet::SCREEN_SPACE;

    // VertexSet::diffuse is FColor* in your UE port:
    for (int i = 0; i < 4; i++) {
        vset4.diffuse[i] = FColor::White;

        if (vset4.specular)
            vset4.specular[i] = FColor::Black;
    }

    // Tile scale based on window rect vs bitmap size (original intent):
    const float xscale = (float)rect.w / (float)img->Width();
    const float yscale = (float)rect.h / (float)img->Height();

    vset4.s_loc[0].X = (float)(rect.x + x1) - 0.5f;
    vset4.s_loc[0].Y = (float)(rect.y + y1) - 0.5f;
    vset4.s_loc[0].Z = 0.0f;
    vset4.rw[0] = 1.0f;
    vset4.tu[0] = 0.0f;
    vset4.tv[0] = 0.0f;

    vset4.s_loc[1].X = (float)(rect.x + x2) - 0.5f;
    vset4.s_loc[1].Y = (float)(rect.y + y1) - 0.5f;
    vset4.s_loc[1].Z = 0.0f;
    vset4.rw[1] = 1.0f;
    vset4.tu[1] = xscale;
    vset4.tv[1] = 0.0f;

    vset4.s_loc[2].X = (float)(rect.x + x2) - 0.5f;
    vset4.s_loc[2].Y = (float)(rect.y + y2) - 0.5f;
    vset4.s_loc[2].Z = 0.0f;
    vset4.rw[2] = 1.0f;
    vset4.tu[2] = xscale;
    vset4.tv[2] = yscale;

    vset4.s_loc[3].X = (float)(rect.x + x1) - 0.5f;
    vset4.s_loc[3].Y = (float)(rect.y + y2) - 0.5f;
    vset4.s_loc[3].Z = 0.0f;
    vset4.rw[3] = 1.0f;
    vset4.tu[3] = 0.0f;
    vset4.tv[3] = yscale;

    Material mtl;
    mtl.tex_diffuse = img;

    Poly poly(0);
    poly.nverts = 4;
    poly.vertex_set = &vset4;
    poly.material = &mtl;
    poly.verts[0] = 0;
    poly.verts[1] = 1;
    poly.verts[2] = 2;
    poly.verts[3] = 3;

    video->DrawScreenPolys(1, &poly, blend);
}


// +--------------------------------------------------------------------+

static float ellipse_pts[256];

void
View::DrawEllipse(int x1, int y1, int x2, int y2, const FColor& color, int blend)
{
    Video* video = screen ? screen->GetVideo() : 0;
    if (!video) return;

    sort(x1, x2);
    sort(y1, y2);

    if (x1 > rect.w || x2 < 0 || y1 > rect.h || y2 < 0)
        return;

    double w2 = (x2 - x1) / 2.0;
    double h2 = (y2 - y1) / 2.0;
    double cx = rect.x + x1 + w2;
    double cy = rect.y + y1 + h2;
    double r = w2;
    int    ns = 4;
    int    np = 0;

    if (h2 > r) r = h2;
    if (r > 2 * ns) ns = (int)(r / 2);
    if (ns > 64) ns = 64;

    double theta = 0;
    double dt = (PI / 2) / ns;

    // quadrant 1 (lower right):
    if (cx < (rect.x + rect.w) && cy < (rect.y + rect.h)) {
        theta = 0;
        np = 0;

        for (int i = 0; i < ns; i++) {
            double ex1 = x1 + w2 + cos(theta) * w2;
            double ey1 = y1 + h2 + sin(theta) * h2;

            theta += dt;

            double ex2 = x1 + w2 + cos(theta) * w2;
            double ey2 = y1 + h2 + sin(theta) * h2;

            if (ClipLine(ex1, ey1, ex2, ey2)) {
                ellipse_pts[np++] = (float)(rect.x + ex1);
                ellipse_pts[np++] = (float)(rect.y + ey1);
                ellipse_pts[np++] = (float)(rect.x + ex2);
                ellipse_pts[np++] = (float)(rect.y + ey2);
            }
        }

        video->DrawScreenLines(np / 4, ellipse_pts, color, blend);
    }

    // quadrant 2 (lower left):
    if (cx > rect.x && cy < (rect.y + rect.h)) {
        theta = 90 * DEGREES;
        np = 0;

        for (int i = 0; i < ns; i++) {
            double ex1 = x1 + w2 + cos(theta) * w2;
            double ey1 = y1 + h2 + sin(theta) * h2;

            theta += dt;

            double ex2 = x1 + w2 + cos(theta) * w2;
            double ey2 = y1 + h2 + sin(theta) * h2;

            if (ClipLine(ex1, ey1, ex2, ey2)) {
                ellipse_pts[np++] = (float)(rect.x + ex1);
                ellipse_pts[np++] = (float)(rect.y + ey1);
                ellipse_pts[np++] = (float)(rect.x + ex2);
                ellipse_pts[np++] = (float)(rect.y + ey2);
            }
        }

        video->DrawScreenLines(np / 4, ellipse_pts, color, blend);
    }

    // quadrant 3 (upper left):
    if (cx > rect.x && cy > rect.y) {
        theta = 180 * DEGREES;
        np = 0;

        for (int i = 0; i < ns; i++) {
            double ex1 = x1 + w2 + cos(theta) * w2;
            double ey1 = y1 + h2 + sin(theta) * h2;

            theta += dt;

            double ex2 = x1 + w2 + cos(theta) * w2;
            double ey2 = y1 + h2 + sin(theta) * h2;

            if (ClipLine(ex1, ey1, ex2, ey2)) {
                ellipse_pts[np++] = (float)(rect.x + ex1);
                ellipse_pts[np++] = (float)(rect.y + ey1);
                ellipse_pts[np++] = (float)(rect.x + ex2);
                ellipse_pts[np++] = (float)(rect.y + ey2);
            }
        }

        video->DrawScreenLines(np / 4, ellipse_pts, color, blend);
    }

    // quadrant 4 (upper right):
    if (cx < (rect.x + rect.w) && cy > rect.y) {
        theta = 270 * DEGREES;
        np = 0;

        for (int i = 0; i < ns; i++) {
            double ex1 = x1 + w2 + cos(theta) * w2;
            double ey1 = y1 + h2 + sin(theta) * h2;

            theta += dt;

            double ex2 = x1 + w2 + cos(theta) * w2;
            double ey2 = y1 + h2 + sin(theta) * h2;

            if (ClipLine(ex1, ey1, ex2, ey2)) {
                ellipse_pts[np++] = (float)(rect.x + ex1);
                ellipse_pts[np++] = (float)(rect.y + ey1);
                ellipse_pts[np++] = (float)(rect.x + ex2);
                ellipse_pts[np++] = (float)(rect.y + ey2);
            }
        }

        video->DrawScreenLines(np / 4, ellipse_pts, color, blend);
    }
}

void
View::FillEllipse(int x1, int y1, int x2, int y2, const FColor& color, int blend)
{
    Video* video = screen ? screen->GetVideo() : 0;
    if (!video) return;

    sort(x1, x2);
    sort(y1, y2);

    if (x1 > rect.w || x2 < 0 || y1 > rect.h || y2 < 0)
        return;

    double w2 = (x2 - x1) / 2.0;
    double h2 = (y2 - y1) / 2.0;
    double cx = x1 + w2;
    double cy = y1 + h2;
    double r = w2;
    int    ns = 4;

    if (h2 > r) r = h2;
    if (r > 2 * ns) ns = (int)(r / 2);
    if (ns > 64) ns = 64;

    double theta = -PI / 2;
    double dt = PI / ns;

    for (int i = 0; i < ns; i++) {
        double ex1 = cos(theta) * w2;
        double ey1 = sin(theta) * h2;

        theta += dt;

        double ex2 = cos(theta) * w2;
        double ey2 = sin(theta) * h2;

        FVector pts[4];

        pts[0].X = (float)(cx - ex1);
        pts[0].Y = (float)(cy + ey1);

        pts[1].X = (float)(cx + ex1);
        pts[1].Y = (float)(cy + ey1);

        pts[2].X = (float)(cx + ex2);
        pts[2].Y = (float)(cy + ey2);

        pts[3].X = (float)(cx - ex2);
        pts[3].Y = (float)(cy + ey2);

        if (pts[0].X > rect.w && pts[3].X > rect.w) continue;
        if (pts[1].X < 0 && pts[2].X < 0) continue;
        if (pts[0].Y > rect.h) return;
        if (pts[2].Y < 0) continue;

        if (pts[0].X < 0) pts[0].X = 0;
        if (pts[3].X < 0) pts[3].X = 0;
        if (pts[1].X > rect.w) pts[1].X = (float)rect.w;
        if (pts[2].X > rect.w) pts[2].X = (float)rect.w;

        if (pts[0].Y < 0) pts[0].Y = 0;
        if (pts[1].Y < 0) pts[1].Y = 0;
        if (pts[2].Y > rect.h) pts[2].Y = (float)rect.h;
        if (pts[3].Y > rect.h) pts[3].Y = (float)rect.h;

        FillPoly(4, pts, color, blend);
    }
}

// +--------------------------------------------------------------------+

void View::ExecFrame()
{ 
    // usually overridden
}

void View::DrawTextRect(const char* txt, int count, Rect& txt_rect, DWORD flags)
{
    if (!font)
        return;

    if (!txt)
        return;

    if (count <= 0)
        count = (int)strlen(txt);

    // Clip the rect to this view:
    Rect clip_rect = txt_rect;

    if (clip_rect.x < 0) {
        const int dx = -clip_rect.x;
        clip_rect.x += dx;
        clip_rect.w -= dx;
    }

    if (clip_rect.y < 0) {
        const int dy = -clip_rect.y;
        clip_rect.y += dy;
        clip_rect.h -= dy;
    }

    if (clip_rect.w < 1 || clip_rect.h < 1)
        return;

    if (clip_rect.x + clip_rect.w > rect.w)
        clip_rect.w = rect.w - clip_rect.x;

    if (clip_rect.y + clip_rect.h > rect.h)
        clip_rect.h = rect.h - clip_rect.y;

    // Convert to window space:
    clip_rect.x += rect.x;
    clip_rect.y += rect.y;

    // Draw:
    font->DrawText(txt, count, clip_rect, flags);
    font->SetAlpha(1.0);

    // If calc-only, return measured rect back to caller:
    if (flags & DT_CALCRECT) {
        txt_rect.w = clip_rect.w;
        txt_rect.h = clip_rect.h;
    }
}

void
View::SetStatusColor(SYSTEM_STATUS status)
{
    switch (status) {
    default:
    case SYSTEM_STATUS::NOMINAL:     StatusColor = TextColor;			break;
    case SYSTEM_STATUS::DEGRADED:    StatusColor = FColor(255, 255, 0); break;
    case SYSTEM_STATUS::CRITICAL:    StatusColor = FColor(255, 0, 0);	break;
    case SYSTEM_STATUS::DESTROYED:   StatusColor = FColor(0, 0, 0);		break;
    }
}

void
View::SetTextColor(FColor TColor)
{
    TextColor = TColor;
}

void 
View::SetHUDColor(FColor HColor)
{
    HudColor = HColor;
}

void View::DrawTextRect(const FString& Text, int Count, Rect& TxtRect, DWORD Flags)
{
    if (Text.IsEmpty())
        return;

    FTCHARToUTF8 Conv(*Text);
    DrawTextRect(Conv.Get(), Count, TxtRect, Flags);
}

void View::Print(int x1, int y1, const char* fmt, ...)
{
    if (!font || !fmt)
        return;

    if (x1 < 0 || y1 < 0 || x1 >= rect.w || y1 >= rect.h)
        return;

    x1 += rect.x;
    y1 += rect.y;

    char msgbuf[512];

    va_list args;
    va_start(args, fmt);

#if PLATFORM_WINDOWS
    vsnprintf_s(msgbuf, sizeof(msgbuf), _TRUNCATE, fmt, args);
#else
    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
#endif

    va_end(args);

    font->DrawString(msgbuf, (int)strlen(msgbuf), x1, y1, rect);
}

// ------------------------------------------------------------
// Unreal-friendly FString Print
// ------------------------------------------------------------
void View::Print(int x1, int y1, const FString& InText)
{
    if (InText.IsEmpty())
        return;

    FTCHARToUTF8 Conv(*InText);
    Print(x1, y1, "%s", Conv.Get());
}

bool View::OnMouseButtonDown(int32 Button, const FVector2D& Pos) { return false; }
bool View::OnMouseButtonUp(int32 Button, const FVector2D& Pos) { return false; }
bool View::OnMouseMove(const FVector2D& Pos) { return false; }
bool View::OnKeyDown(int32 Key, bool bRepeat) { return false; }

void View::Show() {}
void View::Hide() {}

bool View::OnMouseDown(int32 Button, int32 x, int32 y)
{
    return OnMouseButtonDown(Button, FVector2D((float)x, (float)y));
}

bool View::OnMouseUp(int32 Button, int32 x, int32 y)
{
    return OnMouseButtonUp(Button, FVector2D((float)x, (float)y));
}

bool View::OnMouseMove(int32 x, int32 y)
{
    return OnMouseMove(FVector2D((float)x, (float)y));
}

bool View::OnKeyDown(int32 Key)
{
    return OnKeyDown(Key, false);
}

bool View::OnKeyUp(int32 Key)
{
    return false;
}

