/*  Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM:    nGenEx.lib
    FILE:         ModelFile.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    ModelFile base class (getter-based, no internal pointer binding).

    Option A:
    - ModelFile stores a read-only snapshot (FModelFileInfo) of the model state.
    - Derived loaders/savers use ModelFile::model (SimModel*) plus snapshot values.
*/

#pragma once

class SimModel;

// --------------------------------------------------------------------
// Read-only snapshot of model state (captured at Load/Save entry)
// --------------------------------------------------------------------

struct FModelFileInfo
{
    const char* Name = nullptr;   // pointer to model-owned name buffer (copy if you need persistence)
    int         NumVerts = 0;
    int         NumPolys = 0;
    float       Radius = 0.0f;
};

// --------------------------------------------------------------------
// ModelFile
// --------------------------------------------------------------------

class ModelFile
{
public:
    ModelFile(const char* fname);
    virtual ~ModelFile();

    virtual bool Load(SimModel* m, double scale = 1.0);
    virtual bool Save(SimModel* m);

protected:
    char     filename[256];
    SimModel* model;

    // Getter-based snapshot:
    FModelFileInfo info;
};
