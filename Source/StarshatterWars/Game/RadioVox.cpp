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
#include "ThreadSync.h"

// Minimal Unreal logging support:
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterWarsRadioVox, Log, All);

// +====================================================================+
//
// RADIO VOX CONTROLLER:
//
// NOTE: Original implementation used Win32 threads (CreateThread/Sleep/etc).
// This file retains that structure for now, but removes MemDebug.h.
// If/when you migrate to UE threading (FRunnable/AsyncTask/FTSTicker),
// replace these Windows APIs accordingly.
//

DWORD WINAPI VoxUpdateProc(LPVOID link);

class RadioVoxController
{
public:
	enum { MAX_QUEUE = 5 };

	RadioVoxController();
	~RadioVoxController();

	bool  Add(RadioVox* vox);
	void  Update();
	DWORD UpdateThread();

	bool           shutdown;
	HANDLE         hthread;
	List<RadioVox> queue;
	ThreadSync     sync;
};

static RadioVoxController* controller = 0;

// +--------------------------------------------------------------------+

RadioVoxController::RadioVoxController()
	: hthread(0), shutdown(false)
{
	DWORD thread_id = 0;
	hthread = CreateThread(0, 4096, VoxUpdateProc, (LPVOID)this, 0, &thread_id);

	if (!hthread) {
		UE_LOG(LogStarshatterWarsRadioVox, Error, TEXT("RadioVoxController: CreateThread failed."));
	}
}

// +--------------------------------------------------------------------+

RadioVoxController::~RadioVoxController()
{
	shutdown = true;

	if (hthread) {
		WaitForSingleObject(hthread, 500);
		CloseHandle(hthread);
		hthread = 0;
	}

	queue.destroy();
}

// +--------------------------------------------------------------------+

DWORD WINAPI VoxUpdateProc(LPVOID link)
{
	RadioVoxController* controller_local = (RadioVoxController*)link;

	if (controller_local)
		return controller_local->UpdateThread();

	return (DWORD)E_POINTER;
}

// +--------------------------------------------------------------------+

DWORD
RadioVoxController::UpdateThread()
{
	while (!shutdown) {
		Update();
		Sleep(50);
	}

	return 0;
}

// +--------------------------------------------------------------------+

void
RadioVoxController::Update()
{
	AutoThreadSync a(sync);

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

	AutoThreadSync a(sync);

	if (queue.size() < MAX_QUEUE) {
		queue.append(vox);
		return true;
	}

	UE_LOG(LogStarshatterWarsRadioVox, Warning, TEXT("RadioVoxController: queue full (MAX_QUEUE=%d)."), MAX_QUEUE);
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
	sounds.destroy();
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

			return true;
		}

		UE_LOG(LogStarshatterWarsRadioVox, Verbose, TEXT("RadioVox: missing optional phrase '%hs/%hs'."), datapath, filename);
	}

	return false;
}

// +--------------------------------------------------------------------+

bool
RadioVox::Start()
{
	if (controller)
		return controller->Add(this);

	UE_LOG(LogStarshatterWarsRadioVox, Warning, TEXT("RadioVox::Start called before RadioVox::Initialize()."));
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
