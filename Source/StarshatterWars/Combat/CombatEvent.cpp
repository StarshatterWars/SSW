// ============================================================================
// CombatEvent.cpp
// ============================================================================

#include "CombatEvent.h"
#include "Campaign.h"
#include "Engine/Texture2D.h"

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
CombatEvent::CombatEvent(Campaign* c,
    ECombatEventType typ,
    int tim,
    int tem,
    ECombatEventSource src,
    const char* rgn)
    : campaign(c)
    , type(typ)
    , time(tim)
    , team(tem)
    , source(src)
    , visited(false)
    , loc(FVector::ZeroVector)
    , points(FVector::ZeroVector)
    , region((rgn&&* rgn) ? rgn : "")
    , image(nullptr)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[CombatEvent] ctor: campaign=%p type=%s time=%d team=%d src=%d region='%s'"),
        campaign,
        *GetTypeName(type),
        time,
        team,
        (int)source,
        (rgn && *rgn) ? ANSI_TO_TCHAR(rgn) : TEXT("<null-or-empty>"));
}

// ----------------------------------------------------------------------------
// Load
// ----------------------------------------------------------------------------
void CombatEvent::Load()
{
    UE_LOG(LogTemp, Log,
        TEXT("[CombatEvent] Load: region='%s' type=%s source=%d"),
        ANSI_TO_TCHAR(region),
        *GetTypeName(type),
        (int)source);
}

// ----------------------------------------------------------------------------
// Name Helpers
// ----------------------------------------------------------------------------
FString CombatEvent::GetEventSourceName() const
{
    return GetSourceName(source);
}

FString CombatEvent::GetEventTypeName() const
{
    return GetTypeName(type);
}

// ----------------------------------------------------------------------------
// Type Mapping
// ----------------------------------------------------------------------------
FString CombatEvent::GetTypeName(ECombatEventType Type)
{
    switch (Type)
    {
    case ECombatEventType::NONE:            return TEXT("NONE");
    case ECombatEventType::ATTACK:          return TEXT("ATTACK");
    case ECombatEventType::DEFEND:          return TEXT("DEFEND");
    case ECombatEventType::MOVE_TO:         return TEXT("MOVE_TO");
    case ECombatEventType::CAPTURE:         return TEXT("CAPTURE");
    case ECombatEventType::STRATEGY:        return TEXT("STRATEGY");
    case ECombatEventType::CAMPAIGN_START:  return TEXT("CAMPAIGN_START");
    case ECombatEventType::STORY:           return TEXT("STORY");
    case ECombatEventType::CAMPAIGN_END:    return TEXT("CAMPAIGN_END");
    case ECombatEventType::CAMPAIGN_FAIL:   return TEXT("CAMPAIGN_FAIL");
    default:                                return TEXT("UNKNOWN");
    }
}

ECombatEventType CombatEvent::GetTypeFromName(const FString& Name)
{
    if (Name.Equals(TEXT("ATTACK"), ESearchCase::IgnoreCase)) return ECombatEventType::ATTACK;
    if (Name.Equals(TEXT("DEFEND"), ESearchCase::IgnoreCase)) return ECombatEventType::DEFEND;
    if (Name.Equals(TEXT("MOVE_TO"), ESearchCase::IgnoreCase)) return ECombatEventType::MOVE_TO;
    if (Name.Equals(TEXT("CAPTURE"), ESearchCase::IgnoreCase)) return ECombatEventType::CAPTURE;
    if (Name.Equals(TEXT("STRATEGY"), ESearchCase::IgnoreCase)) return ECombatEventType::STRATEGY;
    if (Name.Equals(TEXT("CAMPAIGN_START"), ESearchCase::IgnoreCase)) return ECombatEventType::CAMPAIGN_START;
    if (Name.Equals(TEXT("STORY"), ESearchCase::IgnoreCase)) return ECombatEventType::STORY;
    if (Name.Equals(TEXT("CAMPAIGN_END"), ESearchCase::IgnoreCase)) return ECombatEventType::CAMPAIGN_END;
    if (Name.Equals(TEXT("CAMPAIGN_FAIL"), ESearchCase::IgnoreCase)) return ECombatEventType::CAMPAIGN_FAIL;

    return ECombatEventType::NONE;
}

// ----------------------------------------------------------------------------
// Source Mapping
// ----------------------------------------------------------------------------
FString CombatEvent::GetSourceName(ECombatEventSource Source)
{
    switch (Source)
    {
    case ECombatEventSource::NONE:     return TEXT("NONE");
    case ECombatEventSource::FORCOM:   return TEXT("FORCOM");
    case ECombatEventSource::TACNET:   return TEXT("TACNET");
    case ECombatEventSource::INTEL:    return TEXT("INTEL");
    case ECombatEventSource::MAIL:     return TEXT("MAIL");
    case ECombatEventSource::NEWS:     return TEXT("NEWS");
    default:                           return TEXT("UNKNOWN");
    }
}

ECombatEventSource CombatEvent::GetSourceFromName(const FString& Name)
{
    if (Name.Equals(TEXT("FORCOM"), ESearchCase::IgnoreCase)) return ECombatEventSource::FORCOM;
    if (Name.Equals(TEXT("TACNET"), ESearchCase::IgnoreCase)) return ECombatEventSource::TACNET;
    if (Name.Equals(TEXT("INTEL"), ESearchCase::IgnoreCase)) return ECombatEventSource::INTEL;
    if (Name.Equals(TEXT("MAIL"), ESearchCase::IgnoreCase)) return ECombatEventSource::MAIL;
    if (Name.Equals(TEXT("NEWS"), ESearchCase::IgnoreCase)) return ECombatEventSource::NEWS;

    return ECombatEventSource::NONE;
}