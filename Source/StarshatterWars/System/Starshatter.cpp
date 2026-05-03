#include "Starshatter.h"

#include "DataLoader.h"
#include "MusicManager.h"
#include "Mission.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatter, Log, All);

Starshatter* Starshatter::instance = nullptr;

// +--------------------------------------------------------------------+

Starshatter::Starshatter()
{
    instance = this;

    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] Constructor"));
}

// +--------------------------------------------------------------------+

Starshatter::~Starshatter()
{
    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] Destructor"));

    instance = nullptr;
}

// +--------------------------------------------------------------------+

bool Starshatter::Init()
{
    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] Init"));

    // Keep minimal legacy initialization only if still required
    loader = DataLoader::GetLoader();

    return true;
}

// +--------------------------------------------------------------------+

bool Starshatter::ChangeVideo()
{
    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] ChangeVideo (stub)"));

    return true;
}

// +--------------------------------------------------------------------+

void Starshatter::Exit()
{
    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] Exit"));

    // Unreal handles shutdown now
}

// +--------------------------------------------------------------------+

int Starshatter::GetScreenWidth()
{
    return 1920; // Temporary fallback (Unreal owns resolution)
}

// +--------------------------------------------------------------------+

int Starshatter::GetScreenHeight()
{
    return 1080; // Temporary fallback
}

// +--------------------------------------------------------------------+

void Starshatter::InvalidateTextureCache()
{
    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] InvalidateTextureCache (stub)"));
}

// +--------------------------------------------------------------------+

void Starshatter::SetChatMode(int c)
{
    chat_mode = c;
}

// +--------------------------------------------------------------------+

void Starshatter::ExecCutscene(const char* msn_file, const char* path)
{
    UE_LOG(LogStarshatter, Log,
        TEXT("[Starshatter] ExecCutscene file=%s path=%s"),
        ANSI_TO_TCHAR(msn_file),
        ANSI_TO_TCHAR(path));

    // Stub for now — runtime subsystem / mission system will own this later
}

// +--------------------------------------------------------------------+

void Starshatter::BeginCutscene()
{
    ++cutscene;

    UE_LOG(LogStarshatter, Log,
        TEXT("[Starshatter] BeginCutscene (%d)"),
        cutscene);
}

// +--------------------------------------------------------------------+

void Starshatter::EndCutscene()
{
    if (cutscene > 0)
    {
        --cutscene;
    }

    UE_LOG(LogStarshatter, Log,
        TEXT("[Starshatter] EndCutscene (%d)"),
        cutscene);
}

// +--------------------------------------------------------------------+

Mission* Starshatter::GetCutsceneMission() const
{
    return nullptr; // Stub
}

// +--------------------------------------------------------------------+

const char* Starshatter::GetSubtitles() const
{
    return ""; // Stub
}

// +--------------------------------------------------------------------+

void Starshatter::EndMission()
{
    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] EndMission"));
}

// +--------------------------------------------------------------------+

void Starshatter::StartOrResumeGame()
{
    UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] StartOrResumeGame"));
}

// +--------------------------------------------------------------------+

bool Starshatter::UseFileSystem()
{
    return true;
}