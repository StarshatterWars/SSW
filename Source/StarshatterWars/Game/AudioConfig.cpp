/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         AudioConfig.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR: John DiCamillo
    ORIGINAL STUDIO: Destroyer Studios LLC

    OVERVIEW
    ========
    Audio Configuration class
*/

#include "AudioConfig.h"

#include "DataLoader.h"
#include "ParseUtil.h"
#include "UIButton.h"
#include "Game.h"

#include <stdio.h>

// Unreal logging:
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterWarsAudioConfig, Log, All);

// ---------------------------------------------------------------------

static AudioConfig* audio_config = nullptr;

// ---------------------------------------------------------------------

AudioConfig::AudioConfig()
    : menu_music(90)
    , game_music(90)
    , efx_volume(90)
    , gui_volume(90)
    , wrn_volume(90)
    , vox_volume(90)
    , training(false)
{
    if (!audio_config)
        audio_config = this;
}

AudioConfig::~AudioConfig()
{
    if (audio_config == this)
        audio_config = nullptr;
}

// ---------------------------------------------------------------------

void AudioConfig::Initialize()
{
    audio_config = new AudioConfig;
    if (audio_config)
        audio_config->Load();
}

void AudioConfig::Close()
{
    delete audio_config;
    audio_config = nullptr;
}

AudioConfig* AudioConfig::GetInstance()
{
    return audio_config;
}

// ---------------------------------------------------------------------
// Volume getters (attenuation units):

int AudioConfig::MenuMusic()
{
    if (audio_config)
        return -50 * (100 - audio_config->menu_music);

    return 0;
}

int AudioConfig::GameMusic()
{
    int vol = 0;

    if (audio_config) {
        vol = -50 * (100 - audio_config->game_music);
        if (audio_config->training)
            vol -= 2000;
    }

    return vol;
}

int AudioConfig::EfxVolume()
{
    int vol = 0;

    if (audio_config) {
        vol = -50 * (100 - audio_config->efx_volume);
        if (audio_config->training)
            vol -= 2000;
    }

    return vol;
}

int AudioConfig::GuiVolume()
{
    if (audio_config)
        return -50 * (100 - audio_config->gui_volume);

    return 0;
}

int AudioConfig::WrnVolume()
{
    int vol = 0;

    if (audio_config) {
        vol = -50 * (100 - audio_config->wrn_volume);
        if (audio_config->training)
            vol -= 2000;
    }

    return vol;
}

int AudioConfig::VoxVolume()
{
    int vol = 0;

    if (audio_config) {
        vol = -50 * (100 - audio_config->vox_volume);
        if (audio_config->training && vol < -750)
            vol = -750;
    }

    return vol;
}

int AudioConfig::Silence()
{
    return -5000;
}

// ---------------------------------------------------------------------

void AudioConfig::SetTraining(bool t)
{
    if (audio_config)
        audio_config->training = t;
}

// ---------------------------------------------------------------------
// Setters (0–100 scale):

void AudioConfig::SetMenuMusic(int v)
{
    menu_music = FMath::Clamp(v, 0, 100);
}

void AudioConfig::SetGameMusic(int v)
{
    game_music = FMath::Clamp(v, 0, 100);
}

void AudioConfig::SetEfxVolume(int v)
{
    efx_volume = FMath::Clamp(v, 0, 100);
}

void AudioConfig::SetGuiVolume(int v)
{
    gui_volume = FMath::Clamp(v, 0, 100);
    UIButton::SetVolume(-50 * (100 - gui_volume));
}

void AudioConfig::SetWrnVolume(int v)
{
    wrn_volume = FMath::Clamp(v, 0, 100);
    UIButton::SetVolume(-50 * (100 - wrn_volume));
}

void AudioConfig::SetVoxVolume(int v)
{
    vox_volume = FMath::Clamp(v, 0, 100);
}

// ---------------------------------------------------------------------
// Load config from audio.cfg (legacy format):

void AudioConfig::Load()
{
    DataLoader* loader = DataLoader::GetLoader();
    Text old_path = loader->GetDataPath();
    loader->SetDataPath(0);

    BYTE* block = nullptr;
    int   blocklen = 0;
    const char* ConfigFilename = "audio.cfg";

    FILE* f = nullptr;
    fopen_s(&f, ConfigFilename, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);
        blocklen = ftell(f);
        fseek(f, 0, SEEK_SET);

        block = new BYTE[blocklen + 1];
        block[blocklen] = 0;

        fread(block, blocklen, 1, f);
        fclose(f);
    }

    if (blocklen == 0) {
        loader->SetDataPath(old_path);
        return;
    }

    Parser parser(new BlockReader((const char*)block, blocklen));
    Term* term = parser.ParseTerm();

    if (!term) {
        UE_LOG(LogStarshatterWarsAudioConfig, Error,
            TEXT("AudioConfig: could not parse '%hs'."), ConfigFilename);
        delete[] block;
        loader->SetDataPath(old_path);
        return;
    }

    TermText* file_type = term->isText();
    if (!file_type || file_type->value() != "AUDIO") {
        UE_LOG(LogStarshatterWarsAudioConfig, Warning,
            TEXT("AudioConfig: invalid '%hs' file."), ConfigFilename);
        delete[] block;
        loader->SetDataPath(old_path);
        return;
    }

    do {
        delete term;
        term = parser.ParseTerm();

        if (!term)
            break;

        TermDef* def = term->isDef();
        if (!def)
            continue;

        int v = 0;
        const Text& name = def->name()->value();

        if (name == "menu_music") { GetDefNumber(v, def, ConfigFilename); menu_music = FMath::Clamp(v, 0, 100); }
        else if (name == "game_music") { GetDefNumber(v, def, ConfigFilename); game_music = FMath::Clamp(v, 0, 100); }
        else if (name == "efx_volume") { GetDefNumber(v, def, ConfigFilename); efx_volume = FMath::Clamp(v, 0, 100); }
        else if (name == "gui_volume") { GetDefNumber(v, def, ConfigFilename); gui_volume = FMath::Clamp(v, 0, 100); }
        else if (name == "wrn_volume") { GetDefNumber(v, def, ConfigFilename); wrn_volume = FMath::Clamp(v, 0, 100); }
        else if (name == "vox_volume") { GetDefNumber(v, def, ConfigFilename); vox_volume = FMath::Clamp(v, 0, 100); }

    } while (term);

    delete[] block;
    loader->SetDataPath(old_path);
}

// ---------------------------------------------------------------------

void AudioConfig::Save()
{
    FILE* f = nullptr;
    fopen_s(&f, "audio.cfg", "w");

    if (f) {
        fprintf(f, "AUDIO\n\n");
        fprintf(f, "menu_music: %3d\n", menu_music);
        fprintf(f, "game_music: %3d\n\n", game_music);
        fprintf(f, "efx_volume: %3d\n", efx_volume);
        fprintf(f, "gui_volume: %3d\n", gui_volume);
        fprintf(f, "wrn_volume: %3d\n", wrn_volume);
        fprintf(f, "vox_volume: %3d\n", vox_volume);
        fclose(f);
    }
}
