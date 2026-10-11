/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioVox.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC

	OVERVIEW
	========
	View class for Radio Communications HUD Overlay
*/

#include "RadioVox.h"
#include "RadioView.h"
#include "AudioConfig.h"

#include "DataLoader.h"
#include "Game.h"
#include "Sound.h"
#include "Containers/Ticker.h"

// Minimal Unreal logging support:
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogTempRadioVox, Log, All);

// +====================================================================+
//
// RADIO VOX CONTROLLER:
//
// UObjects and audio components must be accessed on the game thread.
class RadioVoxController
{
public:
    enum { MAX_QUEUE = 5 };
    RadioVoxController();
    ~RadioVoxController();
    bool Add(RadioVox* vox);
    void Update();
    List<RadioVox> queue;
    FTSTicker::FDelegateHandle TickHandle;
};
static RadioVoxController* controller = nullptr;

RadioVoxController::RadioVoxController()
{
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this](float) { Update(); return true; }), 0.05f);
}
RadioVoxController::~RadioVoxController()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    queue.destroy();
}

void
RadioVoxController::Update()
{

	if (queue.size()) {
		RadioVox* vox = queue.first();

		if (!vox->Update())
			delete queue.removeIndex(0);
	}
}

bool
RadioVoxController::Add(RadioVox* vox)
{
	if (!vox || vox->sounds.isEmpty())
		return false;


	if (queue.size() < MAX_QUEUE) {
		queue.append(vox);
		return true;
	}

	UE_LOG(LogTempRadioVox, Warning, TEXT("RadioVoxController: queue full (MAX_QUEUE=%d)."), MAX_QUEUE);
	return false;
}

// +====================================================================+
//
// RADIO VOX MESSAGE:
//

void
RadioVox::Initialize()
{
	if (!controller) {
		controller = new RadioVoxController;
	}
}

void
RadioVox::Close()
{
	delete controller;
	controller = 0;
}

// +--------------------------------------------------------------------+

RadioVox::RadioVox(int n, const char* p, const char* m)
	: path(p), message(m), index(0), channel(n)
{
}

RadioVox::~RadioVox()
{
    for (int i = 0; i < sounds.size(); ++i)
        if (USound* Sound = sounds[i]) Sound->Release();
    sounds.clear(); // USound is a UObject: never delete it through List::destroy.
    SoundReferences.Empty();
}

void RadioVox::DiscardMessages()
{
    if (controller) controller->queue.destroy();
}

// +--------------------------------------------------------------------+

bool
RadioVox::AddPhrase(const char* key)
{
	if (AudioConfig::VoxVolume() <= AudioConfig::Silence())
		return false;

	DataLoader* loader = DataLoader::GetLoader();
	if (!loader)
		return false;

	if (key && *key) {
		char datapath[256];
		char filename[256];

		sprintf_s(datapath, "Vox/%s/", path.data());
		sprintf_s(filename, "%s.wav", key);

		bool   use_fs = loader->IsFileSystemEnabled();
		USound* sound = 0;

		loader->UseFileSystem(true);
		loader->SetDataPath(datapath);
		loader->LoadSound(filename, sound, USound::LOCALIZED, true); // optional sound
		loader->SetDataPath("");
		loader->UseFileSystem(use_fs);

		if (sound) {
			sound->SetVolume(AudioConfig::VoxVolume());
			sound->SetFlags(USound::LOCALIZED | USound::LOCKED);
			sound->SetFilename(filename);
			sounds.append(sound);
            SoundReferences.Emplace(sound);

			return true;
		}

		UE_LOG(LogTempRadioVox, Verbose, TEXT("RadioVox: missing optional phrase '%hs/%hs'."), datapath, filename);
	}

	return false;
}

// +--------------------------------------------------------------------+

bool
RadioVox::Start()
{
	if (controller)
		return controller->Add(this);

	UE_LOG(LogTempRadioVox, Warning, TEXT("RadioVox::Start called before RadioVox::Initialize()."));
	return false;
}

bool
RadioVox::Update()
{
	if (message.length()) {
		RadioView::Message(message);
		message = "";
	}

	bool active = false;

	while (!active && index < sounds.size()) {
		USound* s = sounds[index];

		if (s->IsReady()) {
			if (channel & 1)
				s->SetPan(channel * -3000);
			else
				s->SetPan(channel * 3000);

			s->Play();
			active = true;
		}
		else if (s->IsPlaying()) {
			s->Update();
			active = true;
		}
		else {
			index++;
		}
	}

	return active;
}
