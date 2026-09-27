/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         VideoFactory.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Video Factory class
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+
// Forward Declarations (keep header light):

class Video;
class VideoSettings;
class USoundCard;

// +--------------------------------------------------------------------+

/*
    NOTE:
    - HWND is not supported in Unreal.
    - This factory now stores an opaque native window handle instead.
    - Platform-specific resolution (Slate, viewport, etc.) happens elsewhere.
*/

class VideoFactory
{
public:
    explicit VideoFactory(void* native_window);
    virtual ~VideoFactory();

    virtual Video*      CreateVideo(VideoSettings* vs);
    virtual void        DestroyVideo(Video* video);

private:
    void* native_window;

    Video* video;
};

