/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         Skin.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Classes for rendering solid meshes of polygons
*/

#include "Skin.h"
#include "Solid.h"
#include "SimModel.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSkin, Log, All);

// +--------------------------------------------------------------------+

Skin::Skin(const char* n)
{
    if (n && *n) {
        strncpy_s(name, n, NAMELEN);
        name[NAMELEN - 1] = 0;
    }
    else {
        memset(name, 0, NAMELEN);
    }

    memset(path, 0, 256);
}

// +--------------------------------------------------------------------+

Skin::~Skin()
{
    cells.destroy();
}

// +--------------------------------------------------------------------+

void
Skin::SetName(const char* n)
{
    if (n && *n) {
        strncpy_s(name, n, NAMELEN);
        name[NAMELEN - 1] = 0;
    }
}

void
Skin::SetPath(const char* n)
{
    if (n && *n) {
        strncpy_s(path, n, 256);
        path[255] = 0;
    }
    else {
        memset(path, 0, 256);
    }
}

// +--------------------------------------------------------------------+

void
Skin::AddMaterial(const Material* mtl)
{
    if (!mtl) return;

    bool found = false;

    ListIter<SkinCell> iter = cells;
    while (++iter && !found) {
        SkinCell* s = iter.value();

        if (s->skin && !strcmp(s->skin->name, mtl->name)) {
            s->skin = mtl;
            found = true;
        }
    }

    if (!found) {
        // MemDebug.h removed; use standard allocation:
        SkinCell* s = new SkinCell(mtl);
        cells.append(s);
    }
}

// +--------------------------------------------------------------------+

void
Skin::ApplyTo(SimModel* model) const
{
    if (model) {
        for (int i = 0; i < cells.size(); i++) {
            SkinCell* s = cells[i];

            if (s->skin) {
                s->orig = model->ReplaceMaterial(s->skin);
            }
        }
    }
}

// +--------------------------------------------------------------------+

void
Skin::Restore(SimModel* model) const
{
    if (model) {
        for (int i = 0; i < cells.size(); i++) {
            SkinCell* s = cells[i];

            if (s->orig) {
                model->ReplaceMaterial(s->orig);
                s->orig = 0;
            }
        }
    }
}

// +--------------------------------------------------------------------+
// +--------------------------------------------------------------------+
// +--------------------------------------------------------------------+

SkinCell::SkinCell(const Material* mtl)
    : skin(mtl), orig(0)
{
}

SkinCell::~SkinCell()
{
    // NOTE:
    // In the original code, SkinCell owned `skin` and deleted it.
    // However, Skin::AddMaterial assigns pointers to shared Material instances (e.g., from Model),
    // so deleting here risks double-free / invalid free.
    // We do not delete `skin` in the Unreal port.
    skin = 0;
}

// +--------------------------------------------------------------------+

int
SkinCell::operator == (const SkinCell& other) const
{
    if (skin == other.skin)
        return true;

    if (skin && other.skin)
        return !strcmp(skin->name, other.skin->name);

    return false;
}

// +--------------------------------------------------------------------+

const char*
SkinCell::Name() const
{
    if (skin)
        return skin->name;

    return "Invalid Skin Cell";
}

// +--------------------------------------------------------------------+

void
SkinCell::SetSkin(const Material* mtl)
{
    skin = mtl;
}

void
SkinCell::SetOrig(const Material* mtl)
{
    orig = mtl;
}
