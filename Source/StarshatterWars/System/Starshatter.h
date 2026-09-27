/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Starshatter.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC
*/

#pragma once

#include "CoreMinimal.h"
#include "Types.h"
#include "Game.h"
#include "KeyMap.h"
#include "Text.h"
#include "GameStructs.h"

/*
    Project Starshatter Wars

    ROLE (NOW):
    ===========
    - Legacy support shell
    - Holds remaining data + subsystems not yet migrated
    - No longer owns:
        - Game loop
        - Game state
        - World / Sim
        - Input
        - UI
        - GameMode
*/

class Campaign;
class Ship;
class Mission;
class DataLoader;
class MusicManager;
class SystemFont;

// +--------------------------------------------------------------------+

class Starshatter : public Game
{
public:
    Starshatter();
    virtual ~Starshatter();

    virtual bool Init();          // still used for base init
    virtual bool ChangeVideo();
    virtual void Exit();

    virtual bool OnHelp() { return false; }

    static Starshatter* GetInstance() { return instance; }

    int GetScreenWidth();
    int GetScreenHeight();

    // --- Graphics options ---
    int LensFlare() { return lens_flare; }
    int Corona() { return corona; }
    int Nebula() { return nebula; }
    int Dust() { return dust; }

    // --- Key config ---
    KeyMap& GetKeyMap() { return keycfg; }

    // --- Load state ---
    int GetLoadProgress() { return load_progress; }
    const char* GetLoadActivity() { return load_activity; }

    void InvalidateTextureCache();

    // --- Chat ---
    int GetChatMode() const { return chat_mode; }
    void SetChatMode(int c);
    const char* GetChatText() const { return chat_text.data(); }

    // --- Cutscene ---
    void ExecCutscene(const char* msn_file, const char* path);
    void BeginCutscene();
    void EndCutscene();
    bool InCutscene() const { return cutscene > 0; }
    Mission* GetCutsceneMission() const;
    const char* GetSubtitles() const;
    void EndMission();

    // --- Game flow ---
    void StartOrResumeGame();

    static bool UseFileSystem();

protected:
    static Starshatter* instance;

    // --- Legacy subsystems (to be migrated later) ---
    DataLoader* loader = nullptr;
    MusicManager* music_dir = nullptr;

    // --- Fonts (can move later) ---
    SystemFont* HUDfont = nullptr;
    SystemFont* GUIfont = nullptr;
    SystemFont* GUI_small_font = nullptr;
    SystemFont* terminal = nullptr;
    SystemFont* verdana = nullptr;
    SystemFont* title_font = nullptr;
    SystemFont* limerick18 = nullptr;
    SystemFont* limerick12 = nullptr;
    SystemFont* ocrb = nullptr;

    // --- Rendering / camera config ---
    double field_of_view = 0.0;
    double orig_fov = 0.0;

    // --- Key mapping (temporary) ---
    static int keymap[256];
    static int keyalt[256];
    KeyMap keycfg;

    // --- Misc state ---
    int test_mode = 0;
    int req_change_video = 0;
    int video_changed = 0;

    int lens_flare = 0;
    int corona = 0;
    int nebula = 0;
    int dust = 0;

    int load_step = 0;
    int load_progress = 0;
    Text load_activity;

    int catalog_index = 0;

    int cutscene = 0;
    int lobby_mode = 0;
    int chat_mode = 0;
    Text chat_text;
};