/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         Wave.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+

struct WAVE_HEADER
{
    DWORD RIFF;
    DWORD file_len;
    DWORD WAVE;
};

struct WAVE_FMT
{
    DWORD FMT;
    DWORD chunk_size;
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
};

struct WAVE_FACT
{
    DWORD FACT;
    DWORD chunk_size;
};

struct WAVE_DATA
{
    DWORD DATA;
    DWORD chunk_size;
};

// +--------------------------------------------------------------------+
