/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright (c) 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MusicManager.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Music Manager class to manage selection, setup, and playback
    of background music tracks for both menu and game modes
*/

#include "MusicManager.h"
#include "MusicTrack.h"

#include "Random.h"
#include "DataLoader.h"
#include "FormatUtil.h"
#include "Sound.h"

#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Misc/AssertionMacros.h"
#include "Logging/LogMacros.h"
#include "GameStructs.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterMusic, Log, All);

static MusicManager* music_manager = nullptr;

// +-------------------------------------------------------------------+

MusicManager::MusicManager()
    : mode(MusicMode::NONE)
    , transition(MuisicTransition::CROSS_FADE)
    , track(nullptr)
    , next_track(nullptr)
    , no_music(true)
    , hproc(0)
{
    music_manager = this;

    ScanTracks();

    if (!no_music)
        StartThread();
}

MusicManager::~MusicManager()
{
    StopThread();

    delete track;
    delete next_track;

    menu_tracks.destroy();
    intro_tracks.destroy();
    brief_tracks.destroy();
    debrief_tracks.destroy();
    promote_tracks.destroy();
    flight_tracks.destroy();
    combat_tracks.destroy();
    launch_tracks.destroy();
    recovery_tracks.destroy();
    victory_tracks.destroy();
    defeat_tracks.destroy();
    credit_tracks.destroy();

    if (this == music_manager)
        music_manager = nullptr;
}

// +--------------------------------------------------------------------+

void MusicManager::Initialize()
{
    if (music_manager)
        delete music_manager;

    music_manager = new MusicManager();
}

void MusicManager::Close()
{
    delete music_manager;
    music_manager = nullptr;
}

MusicManager* MusicManager::GetInstance()
{
    return music_manager;
}

// +-------------------------------------------------------------------+

void MusicManager::ExecFrame()
{
    if (no_music)
        return;

    AutoThreadSync a(sync);

    if (next_track && !track) {
        track = next_track;
        next_track = nullptr;
    }

    if (track) {
        if (track->IsDone()) {
            if (mode != MusicMode::NONE && mode != MusicMode::SHUTDOWN && next_track == nullptr) {
                GetNextTrack(track->GetIndex() + 1);
            }

            delete track;
            track = next_track;
            next_track = nullptr;
        }

        else if (track->IsLooped()) {
            if (mode != MusicMode::NONE && mode != MusicMode::SHUTDOWN && next_track == nullptr) {
                GetNextTrack(track->GetIndex() + 1);
            }

            track->FadeOut();
            track->ExecFrame();
        }

        else {
            track->ExecFrame();
        }
    }

    if (next_track) {
        if (next_track->IsDone()) {
            delete next_track;
            next_track = nullptr;
        }

        else if (next_track->IsLooped()) {
            next_track->FadeOut();
            next_track->ExecFrame();
        }

        else {
            next_track->ExecFrame();
        }
    }
}

// +-------------------------------------------------------------------+

void MusicManager::ScanTracks()
{
    DataLoader* loader = DataLoader::GetLoader();

    bool old_file_system = loader->IsFileSystemEnabled();
    loader->UseFileSystem(true);
    loader->SetDataPath("Music/");

    List<Text> files;
    loader->ListFiles("*.ogg", files, true);

    if (files.size() == 0) {
        loader->UseFileSystem(old_file_system);
        no_music = true;
        return;
    }

    no_music = false;

    ListIter<Text> iter = files;
    while (++iter) {
        Text* name = iter.value();
        Text* file = new Text("Music/");

        name->setSensitive(false);
        file->append(*name);

        if (name->indexOf("Menu") == 0) {
            menu_tracks.append(file);
        }
        else if (name->indexOf("Intro") == 0) {
            intro_tracks.append(file);
        }
        else if (name->indexOf("Brief") == 0) {
            brief_tracks.append(file);
        }
        else if (name->indexOf("Debrief") == 0) {
            debrief_tracks.append(file);
        }
        else if (name->indexOf("Promot") == 0) {
            promote_tracks.append(file);
        }
        else if (name->indexOf("Flight") == 0) {
            flight_tracks.append(file);
        }
        else if (name->indexOf("Combat") == 0) {
            combat_tracks.append(file);
        }
        else if (name->indexOf("Launch") == 0) {
            launch_tracks.append(file);
        }
        else if (name->indexOf("Recovery") == 0) {
            recovery_tracks.append(file);
        }
        else if (name->indexOf("Victory") == 0) {
            victory_tracks.append(file);
        }
        else if (name->indexOf("Defeat") == 0) {
            defeat_tracks.append(file);
        }
        else if (name->indexOf("Credit") == 0) {
            credit_tracks.append(file);
        }
        else {
            menu_tracks.append(file);
        }

        delete iter.removeItem();
    }

    loader->UseFileSystem(old_file_system);

    menu_tracks.sort();
    intro_tracks.sort();
    brief_tracks.sort();
    debrief_tracks.sort();
    promote_tracks.sort();
    flight_tracks.sort();
    combat_tracks.sort();
    launch_tracks.sort();
    recovery_tracks.sort();
    victory_tracks.sort();
    defeat_tracks.sort();
    credit_tracks.sort();
}

// +-------------------------------------------------------------------+

const char* MusicManager::GetModeName(MusicMode InMode)
{
    switch (InMode) {
    case MusicMode::NONE:        return "NONE";
    case MusicMode::MENU:        return "MENU";
    case MusicMode::INTRO:       return "INTRO";
    case MusicMode::BRIEFING:    return "BRIEFING";
    case MusicMode::DEBRIEFING:  return "DEBRIEFING";
    case MusicMode::PROMOTION:   return "PROMOTION";
    case MusicMode::FLIGHT:      return "FLIGHT";
    case MusicMode::COMBAT:      return "COMBAT";
    case MusicMode::LAUNCH:      return "LAUNCH";
    case MusicMode::RECOVERY:    return "RECOVERY";
    case MusicMode::VICTORY:     return "VICTORY";
    case MusicMode::DEFEAT:      return "DEFEAT";
    case MusicMode::CREDITS:     return "CREDITS";
    case MusicMode::SHUTDOWN:    return "SHUTDOWN";
    default:                     break;
    }

    return "UNKNOWN?";
}

// +-------------------------------------------------------------------+

void MusicManager::SetMode(MusicMode InMode)
{
    if (!music_manager || music_manager->no_music)
        return;

    AutoThreadSync a(music_manager->sync);

    // stay in intro mode until it is complete:
    if (InMode == MusicMode::MENU &&
        (music_manager->GetMode() == MusicMode::NONE || music_manager->GetMode() == MusicMode::INTRO))
    {
        InMode = MusicMode::INTRO;
    }

    InMode = music_manager->CheckMode(InMode);

    if (InMode != music_manager->mode) {
        UE_LOG(LogStarshatterMusic, Log, TEXT("MusicManager::SetMode() old: %hs  new: %hs"),
            GetModeName(music_manager->mode),
            GetModeName(InMode));

        music_manager->mode = InMode;

        MusicTrack* t = music_manager->track;
        if (t && t->GetState() && !t->IsDone()) {
            if (InMode == MusicMode::NONE || InMode == MusicMode::SHUTDOWN)
                t->SetFadeTime(0.5);

            t->FadeOut();
        }

        t = music_manager->next_track;
        if (t && t->GetState() && !t->IsDone()) {
            if (InMode == MusicMode::NONE || InMode == MusicMode::SHUTDOWN)
                t->SetFadeTime(0.5);

            t->FadeOut();

            delete music_manager->track;
            music_manager->track = t;
            music_manager->next_track = nullptr;
        }

        music_manager->ShuffleTracks();
        music_manager->GetNextTrack(0);

        if (music_manager->next_track)
            music_manager->next_track->FadeIn();
    }
}

MusicMode MusicManager::CheckMode(MusicMode ReqMode)
{
    if (ReqMode == MusicMode::RECOVERY && recovery_tracks.size() == 0)
        ReqMode = MusicMode::LAUNCH;

    if (ReqMode == MusicMode::LAUNCH && launch_tracks.size() == 0)
        ReqMode = MusicMode::FLIGHT;

    if (ReqMode == MusicMode::COMBAT && combat_tracks.size() == 0)
        ReqMode = MusicMode::FLIGHT;

    if (ReqMode == MusicMode::FLIGHT && flight_tracks.size() == 0)
        ReqMode = MusicMode::NONE;

    if (ReqMode == MusicMode::PROMOTION && promote_tracks.size() == 0)
        ReqMode = MusicMode::VICTORY;

    if (ReqMode == MusicMode::DEBRIEFING && debrief_tracks.size() == 0)
        ReqMode = MusicMode::BRIEFING;

    if (ReqMode == MusicMode::BRIEFING && brief_tracks.size() == 0)
        ReqMode = MusicMode::MENU;

    if (ReqMode == MusicMode::INTRO && intro_tracks.size() == 0)
        ReqMode = MusicMode::MENU;

    if (ReqMode == MusicMode::VICTORY && victory_tracks.size() == 0)
        ReqMode = MusicMode::MENU;

    if (ReqMode == MusicMode::DEFEAT && defeat_tracks.size() == 0)
        ReqMode = MusicMode::MENU;

    if (ReqMode == MusicMode::CREDITS && credit_tracks.size() == 0)
        ReqMode = MusicMode::MENU;

    if (ReqMode == MusicMode::MENU && menu_tracks.size() == 0)
        ReqMode = MusicMode::NONE;

    return ReqMode;
}

// +-------------------------------------------------------------------+

bool MusicManager::IsNoMusic()
{
    if (music_manager)
        return music_manager->no_music;

    return true;
}

// +-------------------------------------------------------------------+

void MusicManager::GetNextTrack(int TrackIndex)
{
    List<Text>* tracks = nullptr;

    switch (mode) {
    case MusicMode::MENU:        tracks = &menu_tracks;     break;
    case MusicMode::INTRO:       tracks = &intro_tracks;    break;
    case MusicMode::BRIEFING:    tracks = &brief_tracks;    break;
    case MusicMode::DEBRIEFING:  tracks = &debrief_tracks;  break;
    case MusicMode::PROMOTION:   tracks = &promote_tracks;  break;
    case MusicMode::FLIGHT:      tracks = &flight_tracks;   break;
    case MusicMode::COMBAT:      tracks = &combat_tracks;   break;
    case MusicMode::LAUNCH:      tracks = &launch_tracks;   break;
    case MusicMode::RECOVERY:    tracks = &recovery_tracks; break;
    case MusicMode::VICTORY:     tracks = &victory_tracks;  break;
    case MusicMode::DEFEAT:      tracks = &defeat_tracks;   break;
    case MusicMode::CREDITS:     tracks = &credit_tracks;   break;
    default:                     tracks = nullptr;           break;
    }

    if (tracks && tracks->size()) {
        if (next_track)
            delete next_track;

        if (TrackIndex < 0 || TrackIndex >= tracks->size()) {
            TrackIndex = 0;

            if (mode == MusicMode::INTRO) {
                mode = MusicMode::MENU;
                ShuffleTracks();
                tracks = &menu_tracks;

                UE_LOG(LogStarshatterMusic, Log, TEXT("MusicManager: INTRO mode complete, switching to MENU"));

                if (!tracks || !tracks->size())
                    return;
            }
        }

        next_track = new MusicTrack(*tracks->at(TrackIndex), mode, TrackIndex);
        next_track->FadeIn();
    }
    else if (next_track) {
        next_track->FadeOut();
    }
}

// +-------------------------------------------------------------------+

void MusicManager::ShuffleTracks()
{
    List<Text>* tracks = nullptr;

    switch (mode) {
    case MusicMode::MENU:        tracks = &menu_tracks;     break;
    case MusicMode::INTRO:       tracks = &intro_tracks;    break;
    case MusicMode::BRIEFING:    tracks = &brief_tracks;    break;
    case MusicMode::DEBRIEFING:  tracks = &debrief_tracks;  break;
    case MusicMode::PROMOTION:   tracks = &promote_tracks;  break;
    case MusicMode::FLIGHT:      tracks = &flight_tracks;   break;
    case MusicMode::COMBAT:      tracks = &combat_tracks;   break;
    case MusicMode::LAUNCH:      tracks = &launch_tracks;   break;
    case MusicMode::RECOVERY:    tracks = &recovery_tracks; break;
    case MusicMode::VICTORY:     tracks = &victory_tracks;  break;
    case MusicMode::DEFEAT:      tracks = &defeat_tracks;   break;
    case MusicMode::CREDITS:     tracks = &credit_tracks;   break;
    default:                     tracks = nullptr;           break;
    }

    if (tracks && tracks->size() > 1) {
        tracks->sort();

        Text* t = tracks->at(0);

        if (!isdigit((*t)[0]))
            tracks->shuffle();
    }
}

// +--------------------------------------------------------------------+

#if PLATFORM_WINDOWS
static DWORD WINAPI MusicManagerThreadProc(LPVOID link);
#endif

void MusicManager::StartThread()
{
#if PLATFORM_WINDOWS
    if (hproc != 0) {
        DWORD result = 0;
        ::GetExitCodeThread(hproc, &result);

        if (result != STILL_ACTIVE) {
            ::CloseHandle(hproc);
            hproc = 0;
        }
        else {
            return;
        }
    }

    if (hproc == 0) {
        DWORD thread_id = 0;
        hproc = ::CreateThread(nullptr, 4096, MusicManagerThreadProc, (LPVOID)this, 0, &thread_id);

        if (hproc == 0) {
            static int report = 10;
            if (report > 0) {
                UE_LOG(LogStarshatterMusic, Warning,
                    TEXT("WARNING: MusicManager failed to create thread (err=%08x)"),
                    (uint32)::GetLastError());
                report--;

                if (report == 0) {
                    UE_LOG(LogStarshatterMusic, Warning,
                        TEXT("Further warnings of this type will be suppressed."));
                }
            }
        }
    }
#else
    UE_LOG(LogStarshatterMusic, Warning, TEXT("MusicManager::StartThread() is only supported on Windows in this build."));
#endif
}

void MusicManager::StopThread()
{
#if PLATFORM_WINDOWS
    if (hproc != 0) {
        SetMode(MusicMode::SHUTDOWN);
        ::WaitForSingleObject(hproc, 1500);
        ::CloseHandle(hproc);
        hproc = 0;
    }
#endif
}

#if PLATFORM_WINDOWS
static DWORD WINAPI MusicManagerThreadProc(LPVOID link)
{
    MusicManager* mgr = (MusicManager*)link;

    if (mgr) {
        while (mgr->GetMode() != MusicMode::SHUTDOWN) {
            mgr->ExecFrame();
            ::Sleep(100);
        }

        return 0;
    }

    return (DWORD)E_POINTER;
}
#endif