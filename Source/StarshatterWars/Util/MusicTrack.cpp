/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MusicTrack.cpp
    AUTHOR:       Carlos Bott


    OVERVIEW
    ========
    MusicTrack class implementation
*/

#include "MusicTrack.h"

#include "AudioConfig.h"
#include "Game.h"
#include "MusicManager.h"
#include "Sound.h"
#include "Starshatter.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterAudio, Log, All);

// +-------------------------------------------------------------------+

static const double FADE_TIME = 1.5;
static const double SILENCE = -5000;

// +-------------------------------------------------------------------+

MusicTrack::MusicTrack(const Text& txt, int m, int n)
    : name(txt)
    , sound(nullptr)
    , state(NONE)
    , mode(m)
    , index(n)
    , fade(0)
    , fade_time(FADE_TIME)
{
    long max_vol = 0;

    if (mode >= MusicManager::FLIGHT)
        max_vol = AudioConfig::GameMusic();
    else
        max_vol = AudioConfig::MenuMusic();

    if (max_vol <= AudioConfig::Silence())
        return;

    name.setSensitive(false);

    if (name.contains(".ogg")) {
        sound = USound::CreateOggStream(name);

        if (!sound) {
            UE_LOG(LogStarshatterAudio, Warning, TEXT("MusicTrack: Failed to create OGG stream for track."));
            return;
        }

        if (name.contains("-loop")) {
            sound->SetFlags(USound::STREAMED |
                USound::OGGVORBIS |
                USound::LOOP |
                USound::LOCKED);
        }
        else {
            sound->SetFlags(USound::STREAMED |
                USound::OGGVORBIS |
                USound::LOCKED);
        }

        sound->SetVolume((long)SILENCE);
    }
}

MusicTrack::~MusicTrack()
{
    if (sound) {
        sound->Stop();
        sound->Release();
        sound = nullptr;
    }
}

// +--------------------------------------------------------------------+

void
MusicTrack::ExecFrame()
{
    bool music_pause = false;

    Starshatter* stars = Starshatter::GetInstance();
    if (stars) {
        music_pause = (stars->GetGameMode() == EGameMode::PLAY) && Game::Paused();
    }

    if (sound && !music_pause) {
        double fvol = 1.0;
        long   volume = 0;

        switch (state) {
        case PLAY:
            if (sound->IsReady())
                sound->Play();
            SetVolume(volume);
            break;

        case FADE_IN:
            if (sound->IsReady())
                sound->Play();

            if (fade > 0) {
                fvol = fade / fade_time;
                volume = (long)(fvol * SILENCE);
                SetVolume(volume);
            }

            if (fade < 0.01)
                state = PLAY;
            break;

        case FADE_OUT:
            if (sound->IsReady())
                sound->Play();

            if (fade > 0) {
                fvol = 1.0 - fade / fade_time;
                volume = (long)(fvol * SILENCE);
                SetVolume(volume);
            }

            if (fade < 0.01)
                state = STOP;
            break;

        case STOP:
            if (sound->IsPlaying()) {
                sound->Stop();
                sound->Release();
                sound = nullptr;
            }
            break;

        default:
            break;
        }

        if (fade > 0)
            fade -= Game::GUITime();

        if (fade < 0)
            fade = 0;
    }
}

// +--------------------------------------------------------------------+

void
MusicTrack::Play()
{
    state = PLAY;
    fade = 0;
}

void
MusicTrack::Stop()
{
    state = STOP;
    fade = 0;
}

void
MusicTrack::FadeIn()
{
    if (state != FADE_IN && state != PLAY) {
        state = FADE_IN;
        fade = fade_time;
    }
}

void
MusicTrack::FadeOut()
{
    if (state != FADE_OUT && state != STOP) {
        state = FADE_OUT;
        fade = fade_time;
    }
}

// +--------------------------------------------------------------------+

int
MusicTrack::IsReady() const
{
    if (sound)
        return sound->IsReady();

    return false;
}

int
MusicTrack::IsPlaying() const
{
    if (sound)
        return sound->IsPlaying();

    return false;
}

int
MusicTrack::IsDone() const
{
    if (sound)
        return sound->IsDone() || sound->LoopCount() >= 5;

    return true;
}

int
MusicTrack::IsLooped() const
{
    if (sound)
        return sound->IsDone() || sound->LoopCount() >= 4;

    return true;
}

// +--------------------------------------------------------------------+

long
MusicTrack::GetVolume() const
{
    if (sound)
        return sound->GetVolume();

    return 0;
}

void
MusicTrack::SetVolume(long v)
{
    if (sound) {
        long max_vol = 0;

        if (mode >= MusicManager::FLIGHT)
            max_vol = AudioConfig::GameMusic();
        else
            max_vol = AudioConfig::MenuMusic();

        if (v > max_vol)
            v = max_vol;

        sound->SetVolume(v);
    }
}

double
MusicTrack::GetTotalTime() const
{
    if (sound)
        return sound->GetTotalTime();

    return 0;
}

double
MusicTrack::GetTimeRemaining() const
{
    if (sound)
        return sound->GetTimeRemaining();

    return 0;
}

double
MusicTrack::GetTimeElapsed() const
{
    if (sound)
        return sound->GetTimeElapsed();

    return 0;
}
