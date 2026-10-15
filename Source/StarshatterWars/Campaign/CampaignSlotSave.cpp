#include "CampaignSlotSave.h"
#include "Campaign.h"
#include "Combatant.h"
#include "CombatAction.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "Kismet/GameplayStatics.h"

FString UCampaignSlotSave::SlotFor(const FString& CampaignName)
{
    FString Safe = CampaignName;
    Safe.ReplaceInline(TEXT(" "), TEXT("_"));
    Safe.ReplaceInline(TEXT("/"), TEXT("_"));
    return FString::Printf(TEXT("Campaign_%s"), *Safe);
}

bool UCampaignSlotSave::SaveCampaign(Campaign* C)
{
    if (!C) return false;

    UCampaignSlotSave* Save = Cast<UCampaignSlotSave>(
        UGameplayStatics::CreateSaveGameObject(UCampaignSlotSave::StaticClass()));
    if (!Save) return false;

    Save->CampaignName = ANSI_TO_TCHAR(C->GetName());
    Save->Status = (int32)C->GetStatus();
    Save->BaseTime = C->GetStartTime();
    Save->Time = C->GetTime();
    Save->Sitrep = ANSI_TO_TCHAR(C->Situation());
    Save->Orders = ANSI_TO_TCHAR(C->Orders());

    if (CombatGroup* G = C->GetPlayerGroup())
    {
        Save->GroupIFF = (int32)G->GetIFF();
        Save->GroupType = (int32)G->Type();
        Save->GroupId = (int32)G->GetID();
    }
    if (CombatUnit* U = C->GetPlayerUnit())
        Save->UnitName = ANSI_TO_TCHAR(U->Name().data());

    ListIter<Combatant> CI = C->GetCombatants();
    while (++CI)
    {
        Combatant* X = CI.value();
        if (!X) continue;
        FCampaignCombatantSave& Row = Save->Combatants.AddDefaulted_GetRef();
        Row.Name = ANSI_TO_TCHAR(X->Name());
        Row.IFF = (int32)X->GetIFF();
        Row.Score = (int32)X->Score();
    }

    ListIter<CombatAction> AI = C->GetActions();
    while (++AI)
    {
        CombatAction* A = AI.value();
        if (!A) continue;
        FCampaignActionSave& Row = Save->Actions.AddDefaulted_GetRef();
        Row.Id = (int32)A->Identity();
        Row.Status = (int32)A->Status();
        Row.Count = (int32)A->Count();
        Row.After = (int32)A->StartAfter();
    }

    ListIter<CombatEvent> EI = C->GetEvents();
    while (++EI)
    {
        CombatEvent* E = EI.value();
        if (!E) continue;
        FCampaignEventSave& Row = Save->Events.AddDefaulted_GetRef();
        Row.Type = ANSI_TO_TCHAR(E->TypeName());
        Row.Time = (int32)E->Time();
        Row.Team = (int32)E->GetIFF();
        Row.Points = (int32)E->Points();
        Row.Source = ANSI_TO_TCHAR(E->SourceName());
        Row.Region = ANSI_TO_TCHAR(E->Region());
        Row.Title = ANSI_TO_TCHAR(E->Title());
        if (E->Filename()) Row.File = ANSI_TO_TCHAR(E->Filename());
        if (E->ImageFile()) Row.Image = ANSI_TO_TCHAR(E->ImageFile());
        if (E->SceneFile()) Row.Scene = ANSI_TO_TCHAR(E->SceneFile());
        Row.Info = ANSI_TO_TCHAR(E->Information());
    }

    const FString Slot = SlotFor(Save->CampaignName);
    return UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
}

UCampaignSlotSave* UCampaignSlotSave::LoadCampaign(const FString& CampaignName)
{
    const FString Slot = SlotFor(CampaignName);
    if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return nullptr;
    return Cast<UCampaignSlotSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
}

bool UCampaignSlotSave::DeleteCampaign(const FString& CampaignName)
{
    return UGameplayStatics::DeleteGameInSlot(SlotFor(CampaignName), 0);
}

bool UCampaignSlotSave::ApplyTo(Campaign* C) const
{
    if (!C) return false;

    C->SetSituation(TCHAR_TO_ANSI(*Sitrep));
    C->SetOrders(TCHAR_TO_ANSI(*Orders));
    C->SetStartTime(BaseTime);
    C->SetLoadTime(BaseTime + Time);

    ListIter<Combatant> CI = C->GetCombatants();
    while (++CI)
    {
        Combatant* X = CI.value();
        if (!X) continue;
        const FString Name = ANSI_TO_TCHAR(X->GetName());
        for (const FCampaignCombatantSave& Row : Combatants)
            if (Row.Name == Name) { X->SetScore(Row.Score); break; }
    }

    ListIter<CombatAction> AI = C->GetActions();
    while (++AI)
    {
        CombatAction* A = AI.value();
        if (!A) continue;
        for (const FCampaignActionSave& Row : Actions)
            if (Row.Id == (int32)A->Identity()) { A->SetStatus(Row.Status); break; }
    }

    // Events are restored by the existing text loader (CombatEvent needs
    // the campaign pointer and enum constructors). The slot keeps them
    // for reference; the file load rebuilds them.
    C->LockoutEvents(3600);
    C->Start();
    return true;
}
