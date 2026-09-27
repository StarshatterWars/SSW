/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         VideoFac.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Video and Polygon Renderer Factory class
*/

#include "VideoFactory.h"

// Legacy backend implementations (still plain C++):
//#include "VideoDX9.h"
//#include "SoundD3D.h"

// Unreal (for logging only):
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogVideoFactory, Log, All);

// +--------------------------------------------------------------------+

VideoFactory::VideoFactory(void* native_window)
    : native_window(native_window)
    , video(nullptr)
{
}

VideoFactory::~VideoFactory()
{
    // Defensive cleanup
    /*if (video) {
        delete video;
        video = nullptr;
    }

    if (audio) {
        delete audio;
        audio = nullptr;
    }*/
}

// +--------------------------------------------------------------------+

Video*
VideoFactory::CreateVideo(VideoSettings* vs)
{
    /*if (!video) {
        video = new VideoDX9(native_window, vs);

        if (!video) {
            UE_LOG(LogVideoFactory, Error, TEXT("Failed to create VideoDX9"));
            delete video;
            video = nullptr;
        }
        else {
            UE_LOG(LogVideoFactory, Log, TEXT("VideoDX9 created successfully"));
        }
    }*/

    return nullptr;
}

// +--------------------------------------------------------------------+

void
VideoFactory::DestroyVideo(Video* v)
{
    /*if (v && v == video) {
        UE_LOG(LogVideoFactory, Log, TEXT("Destroying Video instance"));
        delete video;
        video = nullptr;
    }*/
}

// +--------------------------------------------------------------------+


