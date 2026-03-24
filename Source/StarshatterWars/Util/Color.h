// /*  Project nGenEx
//     Fractal Dev Games
//     Copyright (C) 2024. All Rights Reserved.
//     SUBSYSTEM:    SSW
//     FILE:         Game.cpp
//     AUTHOR:       Carlos Bott
// */

#pragma once

#include "CoreMinimal.h"
#include "Types.h"

#include "Math/Color.h"   // FColor

// +--------------------------------------------------------------------+

class Video;

// +--------------------------------------------------------------------+

struct ColorFormat
{
    ColorFormat(int palsize)
        : pal(palsize), bpp(8),
        rdown(0), rshift(0), rmask(0),
        gdown(0), gshift(0), gmask(0),
        bdown(0), bshift(0), bmask(0),
        adown(0), ashift(0), amask(0) {
    }

    ColorFormat(int size, BYTE r, BYTE rs, BYTE g, BYTE gs, BYTE b, BYTE bs, BYTE a = 0, BYTE as = 0)
        : pal(0), bpp(size),
        rdown(8 - r), rshift(rs), rmask(((1 << r) - 1) << rs),
        gdown(8 - g), gshift(gs), gmask(((1 << g) - 1) << gs),
        bdown(8 - b), bshift(bs), bmask(((1 << b) - 1) << bs),
        adown(8 - a), ashift(as), amask(((1 << a) - 1) << as) {
    }

    int   pal;
    int   bpp;
    BYTE  rdown, rshift;
    BYTE  gdown, gshift;
    BYTE  bdown, bshift;
    BYTE  adown, ashift;

    DWORD rmask;
    DWORD gmask;
    DWORD bmask;
    DWORD amask;
};

// +--------------------------------------------------------------------+

class Color
{
    friend class ColorIndex;
    friend class ColorValue;

public:
    static const char* TYPENAME() { return "Color"; }

    enum Misc { SHADE_LEVELS = 64 }; // max 128!
    enum Mask {
        RMask = 0x00ff0000,
        GMask = 0x0000ff00,
        BMask = 0x000000ff,
        AMask = 0xff000000,
        RGBMask = 0x00ffffff
    };
    enum Shift {
        RShift = 16,
        GShift = 8,
        BShift = 0,
        AShift = 24
    };

    Color() : rgba(0, 0, 0, 0) {}
    Color(const Color& c) : rgba(c.rgba) {}

    // UE-native storage:
    Color(BYTE r, BYTE g, BYTE b, BYTE a = 255) : rgba(r, g, b, a) {}

    // Palette index ctor (kept for legacy 8-bit modes):
    Color(BYTE index);

    Color& operator= (const Color& c) { rgba = c.rgba; return *this; }
    int      operator==(const Color& c) const { return rgba == c.rgba; }
    int      operator!=(const Color& c) const { return rgba != c.rgba; }
    Color& operator+=(const Color& c);       // simple summation

    Color    operator+(DWORD d) const;

    Color    operator+(const Color& c) const;  // color alpha blending
    Color    operator*(const Color& c) const;  // color modulation
    Color    operator*(double scale)   const;
    Color    dim(double scale)         const;

    // NOTE: "value" is interpreted in the legacy packed layout:
    // A<<24 | R<<16 | G<<8 | B
    void     Set(DWORD value)
    {
        const BYTE a = (BYTE)((value & AMask) >> AShift);
        const BYTE r = (BYTE)((value & RMask) >> RShift);
        const BYTE g = (BYTE)((value & GMask) >> GShift);
        const BYTE b = (BYTE)((value & BMask) >> BShift);
        rgba = FColor(r, g, b, a);
    }

    void     Set(BYTE r, BYTE g, BYTE b, BYTE a = 255)
    {
        rgba = FColor(r, g, b, a);
    }

    // Legacy packed layout:
    DWORD    Value() const
    {
        return ((DWORD)rgba.R << RShift) |
            ((DWORD)rgba.G << GShift) |
            ((DWORD)rgba.B << BShift) |
            ((DWORD)rgba.A << AShift);
    }

    void     SetRed(BYTE r) { rgba.R = r; }
    void     SetGreen(BYTE g) { rgba.G = g; }
    void     SetBlue(BYTE b) { rgba.B = b; }
    void     SetAlpha(BYTE a) { rgba.A = a; }

    DWORD    Red()    const { return rgba.R; }
    DWORD    Green()  const { return rgba.G; }
    DWORD    Blue()   const { return rgba.B; }
    DWORD    Alpha()  const { return rgba.A; }

    float    fRed()   const { return (float)(rgba.R / 255.0f); }
    float    fGreen() const { return (float)(rgba.G / 255.0f); }
    float    fBlue()  const { return (float)(rgba.B / 255.0f); }
    float    fAlpha() const { return (float)(rgba.A / 255.0f); }

    // UE-safe accessor if you need to pass directly into UE APIs:
    const FColor& ToFColor() const { return rgba; }
    FColor& ToFColor() { return rgba; }

    BYTE     Index()  const
    {
        const DWORD packed = Value();
        return table[((packed & RMask) >> (RShift + 3) << 10) |
            ((packed & GMask) >> (GShift + 3) << 5) |
            ((packed & BMask) >> (BShift + 3))];
    }

    inline BYTE  IndexedBlend(BYTE dst) const;
    static DWORD FormattedBlend(DWORD c1, DWORD c2);

    DWORD    TextureFormat(int keep_alpha = 0) const;
    DWORD    Formatted()                      const;
    DWORD    Unfaded()                        const;
    Color    ShadeColor(int shade)            const;
    DWORD    Shaded(int shade)                const;
    Color    Faded()                          const;

    // some useful colors
    static   Color          White;
    static   Color          Black;
    static   Color          Gray;
    static   Color          LightGray;
    static   Color          DarkGray;
    static   Color          BrightRed;
    static   Color          BrightBlue;
    static   Color          BrightGreen;
    static   Color          DarkRed;
    static   Color          DarkBlue;
    static   Color          DarkGreen;
    static   Color          Yellow;
    static   Color          Cyan;
    static   Color          Magenta;
    static   Color          Tan;
    static   Color          Brown;
    static   Color          Violet;
    static   Color          Orange;

    static void UseVideo(Video* v);
    static void UseFormat(const ColorFormat& cf);
    static void UseTextureFormat(const ColorFormat& cf, int alpha_level = 0);
    static void WithTextureFormat(int alpha_level = 0);
    static void SetPalette(PALETTEENTRY* pal, int palsize, BYTE* invpal = 0);
    static void SavePalette(const char* basename);
    static void SetFade(double f, Color c = Black, int build_shade = false);

    static const ColorFormat& GetTextureFormat(int alpha_level = 0) { return texture_format[alpha_level]; }
    static const ColorFormat& GetScreenFormat() { return format; }

    // indexed color initialization:
    static void BuildShadeTable();
    static void SaveShadeTable(const char* basename);
    static void BuildBlendTable();

    static double GetFade() { return fade; }
    static Color  GetFadeColor() { return fade_color; }

    static Color Scale(const Color& c1, const Color& c2, double scale);
    static Color Unformat(DWORD formatted_color);

private:
    // UE-native storage replaces legacy packed DWORD:
    FColor   rgba;

    static   bool           standard_format;
    static   PALETTEENTRY   palette[256];
    static   BYTE           table[32768];
    static   ColorFormat    format;
    static   int            texture_alpha_level;
    static   ColorFormat    texture_format[4];
    static   double         fade;
    static   Color          fade_color;
    static   Video* video;
};

// +--------------------------------------------------------------------+

class ColorValue
{
    friend class Color;

public:
    static const char* TYPENAME() { return "ColorValue"; }

    ColorValue() : r(0), g(0), b(0), a(0) {}
    ColorValue(const ColorValue& c) : r(c.r), g(c.g), b(c.b), a(c.a) {}
    ColorValue(const Color& c)
        : r(c.fRed()),
        g(c.fGreen()),
        b(c.fBlue()),
        a(c.fAlpha()) {
    }

    ColorValue(float ar, float ag, float ab, float aa = 1.0f) : r(ar), g(ag), b(ab), a(aa) {}

    int         operator==(const ColorValue& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
    int         operator!=(const ColorValue& c) const { return r != c.r || g != c.g || b != c.b || a != c.a; }
    ColorValue& operator+=(const ColorValue& c);       // simple summation

    ColorValue  operator+(const ColorValue& c)   const;  // color alpha blending
    ColorValue  operator*(const ColorValue& c)   const;  // color modulation
    ColorValue  operator*(double scale)          const;
    ColorValue  dim(double scale)                const;

    void     Set(float ar, float ag, float ab, float aa = 1.0f) { r = ar; g = ag; b = ab; a = aa; }

    void     SetRed(float ar) { r = ar; }
    void     SetGreen(float ag) { g = ag; }
    void     SetBlue(float ab) { b = ab; }
    void     SetAlpha(float aa) { a = aa; }

    float    Red()    const { return r; }
    float    Green()  const { return g; }
    float    Blue()   const { return b; }
    float    Alpha()  const { return a; }

    Color    ToColor()                          const;
    DWORD    TextureFormat(int keep_alpha = 0)  const { return ToColor().TextureFormat(keep_alpha); }
    DWORD    Formatted()                        const { return ToColor().Formatted(); }
    DWORD    Unfaded()                          const { return ToColor().Unfaded(); }
    Color    ShadeColor(int shade)              const { return ToColor().ShadeColor(shade); }
    DWORD    Shaded(int shade)                  const { return ToColor().Shaded(shade); }
    Color    Faded()                            const { return ToColor().Faded(); }

private:
    float    r, g, b, a;
};

// +--------------------------------------------------------------------+

class ColorIndex
{
    friend class Color;

public:
    static const char* TYPENAME() { return "ColorIndex"; }

    ColorIndex() : index(0) {}
    ColorIndex(const ColorIndex& c) : index(c.index) {}
    ColorIndex(BYTE r, BYTE g, BYTE b) { index = Color(r, g, b).Index(); }
    ColorIndex(BYTE i) : index(i) {}

    ColorIndex& operator= (const ColorIndex& c) { index = c.index; return *this; }
    int         operator==(const ColorIndex& c) const { return index == c.index; }
    int         operator!=(const ColorIndex& c) const { return index != c.index; }

    BYTE     Index()  const { return index; }

    DWORD    Red()    const { return Color::palette[index].peRed; }
    DWORD    Green()  const { return Color::palette[index].peGreen; }
    DWORD    Blue()   const { return Color::palette[index].peBlue; }

    float    fRed()   const { return (float)(Red() / 255.0f); }
    float    fGreen() const { return (float)(Green() / 255.0f); }
    float    fBlue()  const { return (float)(Blue() / 255.0f); }

    DWORD    TextureFormat()   const { return texture_palette[index]; }
    DWORD    Unfaded()         const { return unfaded_palette[index]; }
    DWORD    Formatted()       const { return formatted_palette[index]; }
    DWORD    Shaded(int shade) const { return shade_table[shade * 256 + index]; }
    ColorIndex Faded()         const { return ColorIndex(index); }

    // note: this will only work in 8-bit color mode...
    ColorIndex ShadeColor(int s) const { return ColorIndex((BYTE)(shade_table[s * 256 + index])); }

    // for poly shading optimization
    static DWORD* ShadeTable() { return shade_table; }

    BYTE     IndexedBlend(BYTE dst) const { return blend_table[dst * 256 + index]; }

private:
    BYTE     index;

    static   DWORD* texture_palette;
    static   DWORD   texture_palettes[4][256];
    static   DWORD   unfaded_palette[256];
    static   DWORD   formatted_palette[256];
    static   DWORD   shade_table[256 * 256];
    static   BYTE    blend_table[256 * 256];
};

inline BYTE Color::IndexedBlend(BYTE dst) const { return ColorIndex::blend_table[dst * 256 + Index()]; }
