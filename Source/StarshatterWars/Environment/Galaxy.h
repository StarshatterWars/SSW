/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Galaxy.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Galaxy (list of star systems) for a single campaign.
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Text.h"
#include "Term.h"
#include "List.h"

// Minimal Unreal include required for FVector usage in Geometry conversions:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Star;
class StarSystem;
class SimElement;
class SimContact;
class SimSystem;
class SimScene;
class SimProjector;
class SimLight;

class UTexture2D;
class UStarshatterEnvironmentSubsystem;

// +--------------------------------------------------------------------+

class Galaxy
{
public:
    Galaxy(const char* InName);
    virtual ~Galaxy();

    int operator==(const Galaxy& s) const { return name == s.name; }

    // operations:
    virtual void         ExecFrame();

    // NEW: DT/environment-backed bootstrap path
    void                 ClearSystems();
    void                 LoadFromEnvironmentSubsystem(UStarshatterEnvironmentSubsystem* Env);

    // accessors:
    const char* Name()         const { return name; }
    const char* Description()  const { return description; }
    List<StarSystem>& GetSystemList() { return systems; }
    List<Star>& Stars() { return stars; }
    double               Radius()       const { return radius; }

    StarSystem* GetSystem(const char* InName);
    StarSystem* FindSystemByRegion(const char* RgnName);

    static void          InitializeFromEnvironment(UStarshatterEnvironmentSubsystem* Env);
    static void          Close();
    static Galaxy* GetInstance();

protected:
    char                 filename[64];
    Text                 name;
    Text                 description;
    double               radius;           // radius in parsecs

    List<StarSystem>     systems;
    List<Star>           stars;
};