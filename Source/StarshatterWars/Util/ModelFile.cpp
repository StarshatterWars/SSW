/*  Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM:    nGenEx.lib
    FILE:         ModelFile.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    ModelFile base class (getter-based, no internal pointer binding).
    - No actual IO in the base class.
    - Derived classes implement format-specific read/write.
*/

#include "ModelFile.h"
#include "SimModel.h"

#include "CoreMinimal.h"

#include <cstring>

DEFINE_LOG_CATEGORY_STATIC(LogModelFile, Log, All);

// --------------------------------------------------------------------
// Construction / Destruction
// --------------------------------------------------------------------

ModelFile::ModelFile(const char* fname)
    : model(nullptr)
{
    FMemory::Memzero(filename, sizeof(filename));

    if (fname && *fname)
    {
#if PLATFORM_WINDOWS
        strcpy_s(filename, sizeof(filename), fname);
#else
        FCStringAnsi::Strncpy(filename, fname, sizeof(filename) - 1);
#endif
    }

    info = FModelFileInfo{};
}

ModelFile::~ModelFile()
{
}

// --------------------------------------------------------------------
// Snapshot helper
// --------------------------------------------------------------------

static FModelFileInfo MakeModelInfo(const SimModel* m)
{
    FModelFileInfo out{};

    if (m)
    {
        out.Name = m->GetName();
        out.NumVerts = m->GetNumVerts();
        out.NumPolys = m->GetNumPolys();
        out.Radius = (float)m->GetRadius();
    }

    return out;
}

// --------------------------------------------------------------------
// Base Load / Save (no IO here)
// --------------------------------------------------------------------

bool ModelFile::Load(SimModel* m, double /*scale*/)
{
    model = m;
    info = MakeModelInfo(model);

    // Base class does not implement actual file reading.
    return false;
}

bool ModelFile::Save(SimModel* m)
{
    model = m;
    info = MakeModelInfo(model);

    // Base class does not implement actual file writing.
    return false;
}
