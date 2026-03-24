/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    System
	FILE:         Game.cpp
	AUTHOR:       Carlos Bott
*/


#include "Game.h"
#include "Screen.h"
#include "Video.h"
#include "Keyboard.h"
#include "StarshatterGameDataSubsystem.h"

Game* game = 0;

bool     Game::active = false;
bool     Game::paused = false;
bool     Game::server = false;
bool     Game::show_mouse = false;
DWORD    Game::base_game_time = 0;
DWORD    Game::real_time = 0;
DWORD    Game::game_time = 0;
DWORD    Game::time_comp = 1;
DWORD    Game::frame_number = 0;

const int    VIDEO_FPS = 30;
const double MAX_FRAME_TIME_VIDEO = 1.0 / (double)VIDEO_FPS;
const double MAX_FRAME_TIME_NORMAL = 1.0 / 5.0;
const double MIN_FRAME_TIME_NORMAL = 1.0 / 60.0;

double   Game::max_frame_length = MAX_FRAME_TIME_NORMAL;
double   Game::min_frame_length = MIN_FRAME_TIME_NORMAL;

char     Game::panicbuf[256];

static LARGE_INTEGER  perf_freq;
static LARGE_INTEGER  perf_cnt1;
static LARGE_INTEGER  perf_cnt2;

FString Game::VersionInfo = "6.0";
FString Game::AppName = "Starshatter Wars";
FString Game::TitleText = "Starshatter Wars";

int    Game::DefaultTrackUpdate = 500;      // legacy default (tune later)
int    Game::DefaultTrackLength = 20;     // frames or samples (tune)
double Game::DefaultTrackAge = 10.0;   // seconds (tune)
double Game::SensorThreshold = 0.25;    // legacy default; set to whatever you used before

const int MAX_KEY_BUF = 512;
static int vkbuf[MAX_KEY_BUF];
static int vkshiftbuf[MAX_KEY_BUF];
static int vkins = 0;
static int vkext = 0;


Game::Game()
{
}

Game::~Game()
{
}

bool Game::Init()
{
	return false;
}

int Game::Run()
{
	return 0;
}

void Game::Exit()
{
}

void Game::Activate(bool f)
{
}

void Game::Pause(bool f)
{
}

DWORD GetRealTime()
{
	return Game::RealTime();
}

DWORD Game::RealTime()
{
	return real_time;
}

DWORD Game::GameTime()
{
	return game_time;
}

DWORD Game::TimeCompression()
{
	return time_comp;
}

void Game::SetTimeCompression(DWORD comp)
{
	if (comp > 0 && comp <= 100)
		time_comp = comp;
}

DWORD Game::Frame()
{
	return frame_number;
}

FString Game::GetMonth(int month)
{
	switch (month) {
		case 1: 
			return "Jan";
			break;
		case 2:
			return "Feb";
			break;
		case 3:
			return "Mar";
			break;
		case 4:
			return "Apr";
			break;
		case 5:
			return "May";
			break;
		case 6:
			return "Jun";
			break;
		case 7:
			return "Jul";
			break;
		case 8:
			return "Aug";
			break;
		case 9:
			return "Sep";
			break;
		case 10:
			return "Oct";
			break;
		case 11:
			return "Nov";
			break;
		case 12:
			return "Dec";
			break;
		default:
			return "";
			break;
	}
}

void Game::ResetGameTime()
{
	game_time = 0;
}

void Game::SkipGameTime(double seconds)
{
	if (seconds > 0)
		game_time += (DWORD)(seconds * 1000);
}

double Game::FrameRate()
{
	return 0.0;
}

double Game::FrameTime()
{
	return 0.0;
}

double Game::GUITime()
{
	return 0.0;
}

void
Game::SetMaxTexSize(int n)
{
	if (game && n >= 64 && n <= 4096)
		game->max_tex_size = n;
}

int
Game::MaxTexSize()
{
	if (game && game->video) {
		int max_vid_size = game->video->MaxTexSize();
		return max_vid_size < game->max_tex_size ?
			max_vid_size : game->max_tex_size;
	}
	else if (Video::GetInstance()) {
		return Video::GetInstance()->MaxTexSize();
	}

	return 256;
}

int
Game::MaxTexAspect()
{
	if (game && game->video) {
		return game->video->MaxTexAspect();
	}
	else if (Video::GetInstance()) {
		return Video::GetInstance()->MaxTexAspect();
	}

	return 1;
}

bool
Game::DisplayModeSupported(int w, int h, int bpp)
{
	return game && game->video && game->video->IsModeSupported(w, h, bpp);
}

Game* Game::GetInstance()
{
	return nullptr;
}

FColor Game::GetScreenColor()
{
	return FColor();
}

void Game::SetScreenColor(FColor c)
{
}

int Game::GetScreenWidth()
{
	return 0;
}

int Game::GetScreenHeight()
{
	return 0;
}

bool Game::IsWindowed()
{
	return false;
}

bool Game::GameLoop()
{
	return false;
}

void Game::UpdateWorld()
{
}

void Game::GameState()
{
}

void Game::UpdateScreen()
{
}

void Game::CollectStats()
{
}

bool Game::InitContent()
{
	return false;
}

bool Game::InitGame()
{
	return false;
}

bool Game::ToggleFullscreen()
{
	return false;
}

bool Game::AdjustWindowForChange()
{
	return false;
}

void Game::ShowStats()
{
}

void Game::Panic(const char* msg)
{
}

Text
Game::GetText(const char* key)
{
	if (game && game->content && game->content->IsContentBundleLoaded())
		return game->content->GetContentBundleText(key);

	return key;
}

FString
Game::GetGameVersion()
{
	return "5.1.87 EX";
}

Video*
Game::GetVideo()
{
	if (game)
		return game->video;

	return 0;
}
int
Game::GammaLevel()
{
	if (game)
		return game->gamma;

	return 0;
}

void
Game::SetGammaLevel(int g)
{
	if (game) {
		game->gamma = g;

		if (game->video)
			game->video->SetGammaLevel(g);
	}
}

void
FlushKeys()
{
	Keyboard::FlushKeys();
	vkins = vkext = 0;
}

void
BufferKey(int vkey)
{
	if (vkey < 1) return;

	int shift = 0;

	if (GetAsyncKeyState(VK_SHIFT))
		shift |= 1;

	if (GetAsyncKeyState(VK_CONTROL))
		shift |= 2;

	if (GetAsyncKeyState(VK_MENU))
		shift |= 4;

	vkbuf[vkins] = vkey;
	vkshiftbuf[vkins++] = shift;

	if (vkins >= MAX_KEY_BUF)
		vkins = 0;

	if (vkins == vkext) {
		vkext++;
		if (vkext >= MAX_KEY_BUF)
			vkext = 0;
	}
}

int
GetKey()
{
	if (vkins == vkext) return 0;

	int result = vkbuf[vkext++];
	if (vkext >= MAX_KEY_BUF)
		vkext = 0;

	return result;
}

int
GetKeyPlus(int& key, int& shift)
{
	if (vkins == vkext) return 0;

	key = vkbuf[vkext];
	shift = vkshiftbuf[vkext++];

	if (vkext >= MAX_KEY_BUF)
		vkext = 0;

	return key;
}

bool
Game::ResetVideo()
{
	return true;
}

bool
Game::ResizeVideo()
{
	return true;
}

