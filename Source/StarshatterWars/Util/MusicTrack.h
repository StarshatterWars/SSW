/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MusicTrack.h
    AUTHOR:       Carlos Bott


    OVERVIEW
    ========
    MusicTrack class
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "Text.h"

// +-------------------------------------------------------------------+

class USound;

// +-------------------------------------------------------------------+

class MusicTrack
{
public:
    enum STATE { NONE, FADE_IN, PLAY, FADE_OUT, STOP };

    MusicTrack(const Text& name, int mode = 0, int index = 0);
    virtual ~MusicTrack();

    // Operations:
    virtual void      ExecFrame();

    virtual void      Play();
    virtual void      Stop();
    virtual void      FadeIn();
    virtual void      FadeOut();

    // accessors / mutators
    const Text& Name()      const { return name; }
    USound* GetSound()  const { return sound; }
    int               GetState()  const { return state; }
    int               GetMode()   const { return mode; }
    int               GetIndex()  const { return index; }

    int               IsReady()   const;
    int               IsPlaying() const;
    int               IsDone()    const;
    int               IsLooped()  const;

    virtual long      GetVolume() const;
    virtual void      SetVolume(long v);

    virtual double    GetTotalTime()     const;
    virtual double    GetTimeRemaining() const;
    virtual double    GetTimeElapsed()   const;

    virtual double    GetFadeTime()      const { return fade_time; }
    virtual void      SetFadeTime(double t) { fade_time = t; }

protected:
    Text              name;
    USound* sound;
    int               state;
    int               mode;
    int               index;
    double            fade;
    double            fade_time;
};
