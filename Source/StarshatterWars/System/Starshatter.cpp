#include "Campaign.h"
#include "CampaignSaveGame.h"
#include "PlayerData.h"
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

    PlayerData* Player = PlayerData::GetCurrentPlayer();
    if (!Player)
    {
        UE_LOG(LogStarshatter, Warning, TEXT("[Starshatter] No current player"));
        return;
    }

    List<Campaign>& Campaigns = Campaign::GetAllCampaigns();
    Campaign* Chosen = nullptr;

    Text Resume = CampaignSaveGame::GetResumeFile();
    if (Resume.length())
    {
        CampaignSaveGame Save;
        Save.Load(Resume.data());
        Chosen = Save.GetCampaign();
        UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] Resuming save"));
    }
    else if (Player->Trained() < 255 && Campaigns.size() > 0)
    {
        Chosen = Campaigns[0];
        if (Chosen) Chosen->Load();
        UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] Starting training campaign"));
    }
    else if (Campaigns.size() > 1)
    {
        Chosen = Campaigns[1];
        if (Chosen) Chosen->Load();
        UE_LOG(LogStarshatter, Log, TEXT("[Starshatter] Starting dynamic campaign"));
    }

    if (Chosen)
        Campaign::SelectCampaign(Chosen->Name());

    SetGameMode(CLOD_MODE);
}

// +--------------------------------------------------------------------+

bool Starshatter::UseFileSystem()
{
    return true;
}