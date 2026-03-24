/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionEvent.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR: John DiCamillo
    ORIGINAL STUDIO: Destroyer Studios LLC

    OVERVIEW
    ========
    Events for mission scripting
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "Text.h"
#include "Geometry.h"
#include "Bitmap.h"

// Minimal Unreal include for FVector only:
#include "Math/Vector.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class Mission;
class MissionTemplate;
class MissionElement;
class MissionLoad;

class MissionEditorDlg;
class MissionEventDlg;      // legacy dialog
class UMissionEventDlg;     // Unreal dialog

class Ship;
class SimSystem;
class SimElement;
class ShipDesign;
class WeaponDesign;
class StarSystem;
class Instruction;
class USound;

// +--------------------------------------------------------------------+

class MissionEvent
{
    friend class Mission;
    friend class MissionTemplate;
    friend class MissionEditorDlg;

    // Legacy UI (if still present):
    friend class MissionEventDlg;

    // Unreal UI:
    friend class UMissionEventDlg;

public:
    static const char* TYPENAME() { return "MissionEvent"; }

    enum EVENT_TYPE {
        MESSAGE,
        OBJECTIVE,
        INSTRUCTION,
        IFF,
        DAMAGE,
        JUMP,
        HOLD,
        SKIP,
        END_MISSION,

        BEGIN_SCENE,
        CAMERA,
        VOLUME,
        DISPLAY,
        FIRE_WEAPON,
        END_SCENE,

        NUM_EVENTS
    };

    enum EVENT_STATUS {
        PENDING, ACTIVE, COMPLETE, SKIPPED
    };

    enum EVENT_TRIGGER {
        TRIGGER_TIME, TRIGGER_DAMAGE, TRIGGER_DESTROYED,
        TRIGGER_JUMP, TRIGGER_LAUNCH, TRIGGER_DOCK,
        TRIGGER_NAVPT, TRIGGER_EVENT, TRIGGER_SKIPPED,
        TRIGGER_TARGET, TRIGGER_SHIPS_LEFT, TRIGGER_DETECT,
        TRIGGER_RANGE, TRIGGER_EVENT_ALL, TRIGGER_EVENT_ANY,
        NUM_TRIGGERS
    };

    MissionEvent();
    ~MissionEvent();

    // operations:
    void                 ExecFrame(double seconds);
    void                 Activate();

    virtual bool         CheckTrigger();
    virtual void         Execute(bool silent = false);
    virtual void         Skip();

    // accessors:
    int                  EventID()         const { return id; }
    INSTRUCTION_STATUS   GetStatus()       const { return status; }
    bool                 IsPending()       const { return status == INSTRUCTION_STATUS::PENDING; }
    bool                 IsActive()        const { return status == INSTRUCTION_STATUS::ACTIVE; }
    bool                 IsComplete()      const { return status == INSTRUCTION_STATUS::COMPLETE; }
    bool                 IsSkipped()       const { return status == INSTRUCTION_STATUS::SKIPPED; }

    double               Time()            const { return time; }
    double               Delay()           const { return delay; }

    int                  Event()           const { return event; }
    const char* EventName()       const;

    Text                 EventShip()       const { return event_ship; }
    Text                 EventSource()     const { return event_source; }
    Text                 EventTarget()     const { return event_target; }
    Text                 EventMessage()    const { return event_message; }
    Text                 EventSound()      const { return event_sound; }

    int                  EventParam(int index = 0) const;
    int                  NumEventParams()          const;

    int                  EventChance()     const { return event_chance; }
    FVector              EventPoint()      const { return event_point; }
    Rect                 EventRect()       const { return event_rect; }

    int                  Trigger()         const { return trigger; }
    const char* TriggerName()     const;

    Text                 TriggerShip()     const { return trigger_ship; }
    Text                 TriggerTarget()   const { return trigger_target; }

    Text                 TriggerParamStr()           const;
    int                  TriggerParam(int index = 0) const;
    int                  NumTriggerParams()          const;

    static const char* EventName(int n);
    static int           EventForName(const char* n);
    static const char* TriggerName(int n);
    static int           TriggerForName(const char* n);

    int                  id = 0;

    // ---------------------------------------------------------------------
    // Editor / UI mutators (Unreal-safe, no protected access)
    // ---------------------------------------------------------------------

    void SetTime(double InTime) { time = InTime; }
    void SetDelay(double InDelay) { delay = InDelay; }

    void SetEvent(int InEvent) { event = InEvent; }
    void SetTrigger(int InTrigger) { trigger = InTrigger; }

    void SetEventChance(int InChance) { event_chance = InChance; }

    void SetEventShip(const char* In) { event_ship = In; }
    void SetEventSource(const char* In) { event_source = In; }
    void SetEventTarget(const char* In) { event_target = In; }
    void SetEventMessage(const char* In) { event_message = In; }
    void SetEventSound(const char* In) { event_sound = In; }

    void ClearEventParams()
    {
        event_nparams = 0;
        for (int i = 0; i < 10; ++i)
            event_param[i] = 0;
    }

    void SetEventParams(const int* In, int N)
    {
        ClearEventParams();
        if (!In || N <= 0) return;
        if (N > 10) N = 10;

        for (int i = 0; i < N; ++i)
            event_param[i] = In[i];

        event_nparams = N;
    }

    void ClearTriggerParams()
    {
        trigger_nparams = 0;
        for (int i = 0; i < 10; ++i)
            trigger_param[i] = 0;
    }

    void SetTriggerParams(const int* In, int N)
    {
        ClearTriggerParams();
        if (!In || N <= 0) return;
        if (N > 10) N = 10;

        for (int i = 0; i < N; ++i)
            trigger_param[i] = In[i];

        trigger_nparams = N;
    }

    void SetTriggerShip(const char* In) { trigger_ship = In; }
    void SetTriggerTarget(const char* In) { trigger_target = In; }

protected:

    INSTRUCTION_STATUS   status = INSTRUCTION_STATUS::PENDING;
    double               time = 0.0;
    double               delay = 0.0;

    int                  event = MESSAGE;
    Text                 event_ship;
    Text                 event_source;
    Text                 event_target;
    Text                 event_message;
    Text                 event_sound;
    int                  event_param[10] = { 0 };
    int                  event_nparams = 0;
    int                  event_chance = 100;
    FVector              event_point = FVector::ZeroVector;
    Rect                 event_rect;

    int                  trigger = TRIGGER_TIME;
    Text                 trigger_ship;
    Text                 trigger_target;
    int                  trigger_param[10] = { 0 };
    int                  trigger_nparams = 0;

    // Starshatter core rendering assets (UNCHANGED)
    Bitmap* image = nullptr;
    USound* sound = nullptr;
};
