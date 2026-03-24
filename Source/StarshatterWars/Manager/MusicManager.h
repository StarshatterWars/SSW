/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MusicManager.h
    AUTHOR:       Carlos Bott


    OVERVIEW
    ========
    Music Manager class to manage selection, setup, and playback
    of background music tracks for both menu and game modes
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "Text.h"
#include "ThreadSync.h"

// +-------------------------------------------------------------------+

class MusicTrack;

// +-------------------------------------------------------------------+

class MusicManager
{
public:
    enum MODES
    {
        NONE,

        // menu modes:
        MENU,
        INTRO,
        BRIEFING,
        DEBRIEFING,
        PROMOTION,
        VICTORY,
        DEFEAT,
        CREDITS,

        // in game modes:
        FLIGHT,
        COMBAT,
        LAUNCH,
        RECOVERY,

        // special modes:
        SHUTDOWN
    };

    enum TRANSITIONS
    {
        CUT,
        FADE_OUT,
        FADE_IN,
        FADE_BOTH,
        CROSS_FADE
    };

    MusicManager();
    ~MusicManager();

    // Operations:
    void              ExecFrame();
    void              ScanTracks();

    int               CheckMode(int inMode);
    int               GetMode() const { return mode; }

    static void       Initialize();
    static void       Close();
    static MusicManager* GetInstance();
    static void       SetMode(int inMode);
    static const char* GetModeName(int inMode);
    static bool       IsNoMusic();

protected:
    void              StartThread();
    void              StopThread();
    void              GetNextTrack(int index);
    void              ShuffleTracks();

protected:
    int               mode;
    int               transition;

    MusicTrack* track;
    MusicTrack* next_track;

    List<Text>        menu_tracks;
    List<Text>        intro_tracks;
    List<Text>        brief_tracks;
    List<Text>        debrief_tracks;
    List<Text>        promote_tracks;
    List<Text>        flight_tracks;
    List<Text>        combat_tracks;
    List<Text>        launch_tracks;
    List<Text>        recovery_tracks;
    List<Text>        victory_tracks;
    List<Text>        defeat_tracks;
    List<Text>        credit_tracks;

    bool              no_music;

    HANDLE            hproc;
    ThreadSync        sync;
};

