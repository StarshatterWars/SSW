/*  Project Starshatter 4.5
    Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CombatEvent.cpp
    AUTHOR:       John DiCamillo

    UNREAL PORT:
    - Preserves original logic and flow.
    - Uses FString for name lookups (case-insensitive), no _stricmp.
    - Player substitutions ($NAME / $RANK) route through UStarshatterPlayerSubsystem.
    - DataLoader usage preserved (until you finish full GameData -> Subsystem migration).
    - Bitmap loading intentionally removed (legacy Bitmap disabled); keep image_file for later UTexture2D path.
*/

#include "CombatEvent.h"
#include "CombatGroup.h"
#include "Campaign.h"
#include "ShipDesign.h"
#include "Ship.h"
#include "Term.h"
#include "ParseUtil.h"
#include "FormatUtil.h"
#include "DataLoader.h"

#include "StarshatterPlayerSubsystem.h"

#include "Math/UnrealMathUtility.h"

// ----------------------------------------------------------------------

static FString ToFStringSafe(const char* In)
{
    return In ? FString(UTF8_TO_TCHAR(In)) : FString();
}

static bool EqualsI(const FString& A, const TCHAR* B)
{
    return A.Equals(B, ESearchCase::IgnoreCase);
}

// ----------------------------------------------------------------------

CombatEvent::CombatEvent(Campaign* c, int typ, int tim, int tem, ECombatEventSource src, const char* rgn)
    : campaign(c)
    , type(typ)
    , time(tim)
    , team(tem)
    , source(src)
    , visited(false)
    , loc(FVector::ZeroVector)
    , points(FVector::ZeroVector)
    , region(rgn)
{
}

// ----------------------------------------------------------------------

FString CombatEvent::TypeName(ECombatEventType Type)
{
    switch (Type)
    {
    case ECombatEventType::ATTACK:         return TEXT("ATTACK");
    case ECombatEventType::DEFEND:         return TEXT("DEFEND");
    case ECombatEventType::MOVE_TO:        return TEXT("MOVE_TO");
    case ECombatEventType::CAPTURE:        return TEXT("CAPTURE");
    case ECombatEventType::STRATEGY:       return TEXT("STRATEGY");
    case ECombatEventType::STORY:          return TEXT("STORY");
    case ECombatEventType::CAMPAIGN_START: return TEXT("CAMPAIGN_START");
    case ECombatEventType::CAMPAIGN_END:   return TEXT("CAMPAIGN_END");
    case ECombatEventType::CAMPAIGN_FAIL:  return TEXT("CAMPAIGN_FAIL");
    default:                               return TEXT("Unknown");
    }
}

// ----------------------------------------------------------------------

FString CombatEvent::SourceName(ECombatEventSource Source)
{
    switch (Source)
    {
    case ECombatEventSource::FORCOM:  return TEXT("FORCOM");
    case ECombatEventSource::TACNET:  return TEXT("TACNET");
    case ECombatEventSource::INTEL:   return TEXT("SECURE");
    case ECombatEventSource::MAIL:    return TEXT("Mail");
    case ECombatEventSource::NEWS:    return TEXT("News");
    default:                          return TEXT("Unknown");
    }
}

ECombatEventSource CombatEvent::SourceFromName(const FString& Name)
{
    const FString N = Name.TrimStartAndEnd();

    if (N.IsEmpty())
        return ECombatEventSource::NONE;   // <-- define this in your enum

    if (N.Equals(TEXT("FORCOM"), ESearchCase::IgnoreCase))
        return ECombatEventSource::FORCOM;

    if (N.Equals(TEXT("TACNET"), ESearchCase::IgnoreCase))
        return ECombatEventSource::TACNET;

    if (N.Equals(TEXT("SECURE"), ESearchCase::IgnoreCase))
        return ECombatEventSource::INTEL;

    if (N.Equals(TEXT("Mail"), ESearchCase::IgnoreCase))
        return ECombatEventSource::MAIL;

    if (N.Equals(TEXT("News"), ESearchCase::IgnoreCase))
        return ECombatEventSource::NEWS;

    return ECombatEventSource::NONE;
}

// ----------------------------------------------------------------------
ECombatEventType CombatEvent::GetTypeFromName(const FString& Name)
{
    const FString N = Name.TrimStartAndEnd();

    if (N.IsEmpty())
        return ECombatEventType::NONE; // add Unknown to the enum

    if (N.Equals(TEXT("ATTACK"), ESearchCase::IgnoreCase))
        return ECombatEventType::ATTACK;

    if (N.Equals(TEXT("DEFEND"), ESearchCase::IgnoreCase))
        return ECombatEventType::DEFEND;

    if (N.Equals(TEXT("MOVE_TO"), ESearchCase::IgnoreCase))
        return ECombatEventType::MOVE_TO;

    if (N.Equals(TEXT("CAPTURE"), ESearchCase::IgnoreCase))
        return ECombatEventType::CAPTURE;

    if (N.Equals(TEXT("STRATEGY"), ESearchCase::IgnoreCase))
        return ECombatEventType::STRATEGY;

    if (N.Equals(TEXT("STORY"), ESearchCase::IgnoreCase))
        return ECombatEventType::STORY;

    if (N.Equals(TEXT("CAMPAIGN_START"), ESearchCase::IgnoreCase))
        return ECombatEventType::CAMPAIGN_START;

    if (N.Equals(TEXT("CAMPAIGN_END"), ESearchCase::IgnoreCase))
        return ECombatEventType::CAMPAIGN_END;

    if (N.Equals(TEXT("CAMPAIGN_FAIL"), ESearchCase::IgnoreCase))
        return ECombatEventType::CAMPAIGN_FAIL;

    return ECombatEventType::NONE;
}

// ----------------------------------------------------------------------

void CombatEvent::Load()
{
    DataLoader* loader = DataLoader::GetLoader();

    if (!campaign || !loader)
        return;

    loader->SetDataPath(campaign->Path());

    if (file.length() > 0)
    {
        const char* filename = file.data();
        BYTE* block = 0;

        loader->LoadBuffer(filename, block, true);
        info = (const char*)block;
        loader->ReleaseBuffer(block);

        if (info.contains('$'))
        {
            CombatGroup* group = campaign->GetPlayerGroup();

            // Player substitutions migrated to PlayerSubsystem:
            const FString PlayerName = UStarshatterPlayerSubsystem::GetPlayerNameSafe(/*WorldContext*/ nullptr);
            const int32  RankId = UStarshatterPlayerSubsystem::GetRankIdSafe(/*WorldContext*/ nullptr, /*Default*/ 0);
            const FString RankName = UStarshatterPlayerSubsystem::GetRankNameSafe(/*WorldContext*/ nullptr, RankId);

            if (!PlayerName.IsEmpty())
                info = FormatTextReplace(info, "$NAME", TCHAR_TO_UTF8(*PlayerName));

            if (!RankName.IsEmpty())
                info = FormatTextReplace(info, "$RANK", TCHAR_TO_UTF8(*RankName));

            if (group)
            {
                // Group description is legacy const char*; preserve
                info = FormatTextReplace(info, "$GROUP", group->GetDescription());
            }

            char timestr[32] = { 0 };
            FormatDayTime(timestr, campaign->GetTime(), true);
            info = FormatTextReplace(info, "$TIME", timestr);
        }
    }

    // Bitmap loading removed/commented to match template header (Bitmap disabled):
    // if (type < CAMPAIGN_END && image_file.length() > 0) {
    //     loader->LoadBitmap(image_file, image);
    // }

    loader->SetDataPath("");
}
