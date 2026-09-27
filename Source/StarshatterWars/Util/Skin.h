/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         Skin.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Classes for managing run-time selectable skins on solid objects
*/

#pragma once

#include "List.h"

// Forward declarations (keep header light)
class SimModel;
struct Material;

// +--------------------------------------------------------------------+

class SkinCell;

// +--------------------------------------------------------------------+

class Skin
{
public:
    static const char* TYPENAME() { return "Skin"; }
    enum { NAMELEN = 64 };

    Skin(const char* name = 0);
    virtual ~Skin();

    // operations
    void              ApplyTo(SimModel* model) const;
    void              Restore(SimModel* model) const;

    // accessors / mutators
    const char* Name()     const { return name; }
    const char* Path()     const { return path; }
    int               NumCells() const { return cells.size(); }

    void              SetName(const char* n);
    void              SetPath(const char* n);
    void              AddMaterial(const Material* mtl);

protected:
    char              name[NAMELEN];
    char              path[256];
    List<SkinCell>    cells;
};

// +--------------------------------------------------------------------+

class SkinCell
{
    friend class Skin;

public:
    static const char* TYPENAME() { return "SkinCell"; }

    SkinCell(const Material* mtl = 0);
    ~SkinCell();

    int                operator==(const SkinCell& other) const;

    const char* Name() const;
    const Material* Skin() const { return skin; }
    const Material* Orig() const { return orig; }

    void               SetSkin(const Material* mtl);
    void               SetOrig(const Material* mtl);

private:
    const Material* skin;
    const Material* orig;
};

// +--------------------------------------------------------------------+
