/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright (c) 1997-2004. All Rights Reserved.

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
#include "GameStructs.h"
#include "ThreadSync.h"

class MusicTrack;

class MusicManager
{
public:
    MusicManager();
    ~MusicManager();

    void                ExecFrame();
    void                ScanTracks();

    MusicMode           CheckMode(MusicMode InMode);
    MusicMode           GetMode() const { return mode; }

    static void         Initialize();
    static void         Close();
    static MusicManager* GetInstance();
    static void         SetMode(MusicMode InMode);
    static const char* GetModeName(MusicMode InMode);
    static bool         IsNoMusic();

protected:
    void                StartThread();
    void                StopThread();

    // Track sequencing within the CURRENT mode playlist:
    void                GetNextTrack(int TrackIndex);

    void                ShuffleTracks();

protected:
    MusicMode           mode;
    MuisicTransition    transition;

    MusicTrack* track;
    MusicTrack* next_track;

    List<Text>          menu_tracks;
    List<Text>          intro_tracks;
    List<Text>          brief_tracks;
    List<Text>          debrief_tracks;
    List<Text>          promote_tracks;
    List<Text>          flight_tracks;
    List<Text>          combat_tracks;
    List<Text>          launch_tracks;
    List<Text>          recovery_tracks;
    List<Text>          victory_tracks;
    List<Text>          defeat_tracks;
    List<Text>          credit_tracks;

    bool                no_music;

    HANDLE              hproc;
    ThreadSync          sync;
};