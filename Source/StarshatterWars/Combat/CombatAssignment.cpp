/*  Project Starshatter 4.5
    Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CombatAssignment.cpp
    AUTHOR:       Carlos  Bott 

    UNREAL PORT:
    - Maintains all variables and methods (names, signatures, members).
    - Uses UE-friendly FString + logging.
    - Keeps class as pure C++ (no UObject).
*/

#include "CombatAssignment.h"
#include "CoreMinimal.h"
#include "CombatGroup.h"
#include "Mission.h"

// +--------------------------------------------------------------------+

CombatAssignment::CombatAssignment(int t, CombatGroup* obj, CombatGroup* rsc)
    : type(t)
    , objective(obj)
    , resource(rsc)
{
}

// +--------------------------------------------------------------------+

CombatAssignment::~CombatAssignment()
{
}

// +--------------------------------------------------------------------+
// This is used to sort assignments into a priority list.
// Higher priorities should come first in the list, so the
// sense of the operator is "backwards" from the usual.

int
CombatAssignment::operator < (const CombatAssignment& a) const
{
    if (!objective)
        return 0;

    if (!a.objective)
        return 1;

    // Preserve original logic:
    // "higher plan value sorts earlier" -> treat as "less than"
    return objective->GetPlanValue() > a.objective->GetPlanValue();
}

// +--------------------------------------------------------------------+

const char*
CombatAssignment::GetDescription() const
{
    // Preserve signature returning const char*.
    // Use a static buffer like original.
    static char Desc[256];
    Desc[0] = 0;

    // Mission::RoleName(type) in your port should return something convertible to const TCHAR*
    // If it returns const char*, this still works; if it returns FString/TCHAR*, adapt that function.
    const TCHAR* RoleT = ANSI_TO_TCHAR(Mission::RoleName(type));

    const TCHAR* ObjNameT =
        objective ? ANSI_TO_TCHAR(objective->Name().data()) : TEXT("(null)");

    const TCHAR* ResNameT =
        resource ? ANSI_TO_TCHAR(resource->Name().data()) : TEXT("");

    if (!resource)
    {
        // "%s %s"
        FCStringAnsi::Snprintf(
            Desc,
            UE_ARRAY_COUNT(Desc),
            "%s %s",
            TCHAR_TO_ANSI(RoleT),
            TCHAR_TO_ANSI(ObjNameT)
        );
    }
    else
    {
        // "%s %s %s"
        FCStringAnsi::Snprintf(
            Desc,
            UE_ARRAY_COUNT(Desc),
            "%s %s %s",
            TCHAR_TO_ANSI(ResNameT),
            TCHAR_TO_ANSI(RoleT),
            TCHAR_TO_ANSI(ObjNameT)
        );
    }

    return Desc;
}
