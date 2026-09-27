/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         PCX.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    PCX image file loader
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+

enum { PCX_OK, PCX_NOMEM, PCX_TOOBIG, PCX_NOFILE };

// NOTE: This struct matches legacy PCX file layout exactly.
// Do NOT reorder or modify fields.
struct PcxHeader
{
    char  manufacturer;     // Always 10
    char  version;          // Usually 5
    char  encoding;         // Always 1
    char  bits_per_pixel;   // 8 for indexed
    short xmin, ymin;
    short xmax, ymax;
    short hres;
    short vres;
    char  palette16[48];
    char  reserved;
    char  color_planes;
    short bytes_per_line;
    short palette_type;
    char  filler[58];
};

// +--------------------------------------------------------------------+

struct PcxImage
{
    static const char* TYPENAME() { return "PcxImage"; }

    // Constructors
    PcxImage();
    PcxImage(short w, short h, unsigned long* hibits);
    PcxImage(short w, short h, unsigned char* bits, unsigned char* colors);

    ~PcxImage();

    // File I/O
    int Load(const char* filename);      // MUST match PCX.cpp
    int Save(const char* filename);

    int LoadBuffer(unsigned char* buf, int len);

    // Data
    PcxHeader      hdr{};
    unsigned char* bitmap = nullptr;     // 8-bit indexed data
    unsigned long* himap = nullptr;     // 32-bit ARGB data
    unsigned char  pal[768]{};           // RGB palette
    unsigned long  imagebytes = 0;
    unsigned short width = 0;
    unsigned short height = 0;
};
