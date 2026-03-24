/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         AudioConfig.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Audio Configuration class
*/

#pragma once

#include "CoreMinimal.h"

// Forward declarations (legacy engine types):
class DataLoader;

class AudioConfig
{
public:
    AudioConfig();
    ~AudioConfig();

    // Singleton lifecycle:
    static void          Initialize();
    static void          Close();
    static AudioConfig* GetInstance();

    // ------------------------------------------------------------------
    // Modern internal getters (attenuation values):
    static int MenuMusic();
    static int GameMusic();
    static int EfxVolume();
    static int GuiVolume();
    static int WrnVolume();
    static int VoxVolume();
    static int Silence();

    // ------------------------------------------------------------------
    // Legacy API compatibility (DO NOT REMOVE — used by UI code):
    static int GetMenuMusic() { return MenuMusic(); }
    static int GetGameMusic() { return GameMusic(); }
    static int GetEfxVolume() { return EfxVolume(); }
    static int GetGuiVolume() { return GuiVolume(); }
    static int GetWrnVolume() { return WrnVolume(); }
    static int GetVoxVolume() { return VoxVolume(); }

    // ------------------------------------------------------------------
    // State:
    static void SetTraining(bool t);

    // ------------------------------------------------------------------
    // Setters (0–100 scale):
    void SetMenuMusic(int v);
    void SetGameMusic(int v);
    void SetEfxVolume(int v);
    void SetGuiVolume(int v);
    void SetWrnVolume(int v);
    void SetVoxVolume(int v);

    // ------------------------------------------------------------------
    // Persistence:
    void Load();
    void Save();

private:
    int  menu_music;
    int  game_music;
    int  efx_volume;
    int  gui_volume;
    int  wrn_volume;
    int  vox_volume;
    bool training;
};
