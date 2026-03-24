/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionEventDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionEventDlg
    - Unreal UUserWidget replacement for legacy MsnEventDlg.
    - Keeps legacy List<> / ListIter<> usage for mission/system traversal.
    - Uses MissionEvent public mutators (SetTime/SetDelay/SetEventParams/etc)
      so we do NOT touch protected members (time, delay, event_param, etc).
*/

#include "MissionEventDlg.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

#include "Blueprint/WidgetTree.h"
#include "Input/Reply.h"

#include "Mission.h"
#include "MissionEvent.h"
#include "MissionElement.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"

//////////////////////////////////////////////////////////////////////////
// Local helpers

static bool ParseLegacyIntArray(const FString& In, int32* OutArray, int32 MaxElems, int32& OutCount)
{
    OutCount = 0;
    if (!OutArray || MaxElems <= 0) return false;

    FString S = In.TrimStartAndEnd();
    if (S.IsEmpty()) return false;

    // "(1,2,3)" style only:
    if (!S.StartsWith(TEXT("(")) || !S.EndsWith(TEXT(")")))
        return false;

    S = S.Mid(1, S.Len() - 2).TrimStartAndEnd();
    if (S.IsEmpty()) return false;

    TArray<FString> Parts;
    S.ParseIntoArray(Parts, TEXT(","), true);

    for (int32 i = 0; i < Parts.Num() && OutCount < MaxElems; ++i)
    {
        const FString Token = Parts[i].TrimStartAndEnd();
        if (Token.IsEmpty()) continue;

        const int32 V = FCString::Atoi(*Token);
        OutArray[OutCount++] = V;
    }

    return OutCount > 0;
}

static int32 FindEventIndexByName(const FString& Selected)
{
    for (int32 i = 0; i < MissionEvent::NUM_EVENTS; ++i)
    {
        if (Selected.Equals(ANSI_TO_TCHAR(MissionEvent::EventName(i)), ESearchCase::IgnoreCase))
            return i;
    }
    return 0;
}

static int32 FindTriggerIndexByName(const FString& Selected)
{
    for (int32 i = 0; i < MissionEvent::NUM_TRIGGERS; ++i)
    {
        if (Selected.Equals(ANSI_TO_TCHAR(MissionEvent::TriggerName(i)), ESearchCase::IgnoreCase))
            return i;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Ctor

UMissionEventDlg::UMissionEventDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

//////////////////////////////////////////////////////////////////////////
// Lifecycle

void UMissionEventDlg::NativeConstruct()
{
    Super::NativeConstruct();

    if (AcceptButton && !AcceptButton->OnClicked.IsBound())
    {
        AcceptButton->OnClicked.AddDynamic(this, &UMissionEventDlg::OnAcceptClicked);
    }

    // Header currently doesn't declare CancelButton; best-effort bind by name:
    if (WidgetTree)
    {
        static const FName CandidateNames[] = {
            TEXT("CancelButton"), TEXT("BtnCancel"), TEXT("ButtonCancel"),
            TEXT("Cancel"), TEXT("Btn_Cancel"), TEXT("Button_Cancel")
        };

        for (const FName& N : CandidateNames)
        {
            if (UWidget* W = WidgetTree->FindWidget(N))
            {
                if (UButton* Btn = Cast<UButton>(W))
                {
                    if (!Btn->OnClicked.IsBound())
                        Btn->OnClicked.AddDynamic(this, &UMissionEventDlg::OnCancelClicked);
                    break;
                }
            }
        }
    }

    if (EventCombo && !EventCombo->OnSelectionChanged.IsBound())
    {
        EventCombo->OnSelectionChanged.AddDynamic(this, &UMissionEventDlg::OnEventSelectionChanged);
    }

    RebuildFromModel();
}

FReply UMissionEventDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Enter)
    {
        OnAcceptClicked();
        return FReply::Handled();
    }

    if (Key == EKeys::Escape)
    {
        OnCancelClicked();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

//////////////////////////////////////////////////////////////////////////
// Public API

void UMissionEventDlg::InitializeDlg(UMenuScreen* InManager)
{
    Manager = InManager;
}

void UMissionEventDlg::SetMission(Mission* InMission)
{
    MissionPtr = InMission;
}

void UMissionEventDlg::SetMissionEvent(MissionEvent* InEvent)
{
    EventPtr = InEvent;
}

void UMissionEventDlg::ShowDlg()
{
    RebuildFromModel();
    SetVisibility(ESlateVisibility::Visible);
}

//////////////////////////////////////////////////////////////////////////
// UI rebuild

void UMissionEventDlg::RebuildFromModel()
{
    if (!MissionPtr || !EventPtr)
        return;

    FillShipList(EventShipCombo, EventPtr->EventShip());
    FillShipList(EventSourceCombo, EventPtr->EventSource());

    if (EventPtr->Event() == MissionEvent::JUMP)
        FillRgnList(EventTargetCombo, EventPtr->EventTarget());
    else
        FillShipList(EventTargetCombo, EventPtr->EventTarget());

    FillShipList(TriggerShipCombo, EventPtr->TriggerShip());
    FillShipList(TriggerTargetCombo, EventPtr->TriggerTarget());

    if (IdLabel)
        IdLabel->SetText(FText::AsNumber(EventPtr->EventID()));

    if (TimeEdit)
        TimeEdit->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), (float)EventPtr->Time())));

    if (DelayEdit)
        DelayEdit->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), (float)EventPtr->Delay())));

    if (ChanceEdit)
        ChanceEdit->SetText(FText::AsNumber(EventPtr->EventChance()));

    if (EventParamEdit)
        EventParamEdit->SetText(FText::AsNumber(EventPtr->EventParam()));

    if (TriggerParamEdit)
        TriggerParamEdit->SetText(FText::FromString(ANSI_TO_TCHAR(EventPtr->TriggerParamStr())));

    if (EventMessageEdit)
        EventMessageEdit->SetText(FText::FromString(ANSI_TO_TCHAR(EventPtr->EventMessage())));

    if (EventSoundEdit)
        EventSoundEdit->SetText(FText::FromString(ANSI_TO_TCHAR(EventPtr->EventSound())));

    if (EventCombo)
    {
        EventCombo->ClearOptions();
        for (int32 i = 0; i < MissionEvent::NUM_EVENTS; ++i)
        {
            EventCombo->AddOption(ANSI_TO_TCHAR(MissionEvent::EventName(i)));
        }

        const int32 Cur = EventPtr->Event();
        if (Cur >= 0 && Cur < MissionEvent::NUM_EVENTS)
        {
            EventCombo->SetSelectedOption(ANSI_TO_TCHAR(MissionEvent::EventName(Cur)));
        }
    }

    if (TriggerCombo)
    {
        TriggerCombo->ClearOptions();
        for (int32 i = 0; i < MissionEvent::NUM_TRIGGERS; ++i)
        {
            TriggerCombo->AddOption(ANSI_TO_TCHAR(MissionEvent::TriggerName(i)));
        }

        const int32 Cur = EventPtr->Trigger();
        if (Cur >= 0 && Cur < MissionEvent::NUM_TRIGGERS)
        {
            TriggerCombo->SetSelectedOption(ANSI_TO_TCHAR(MissionEvent::TriggerName(Cur)));
        }
    }
}

void UMissionEventDlg::FillShipList(UComboBoxString* Combo, const char* SelectedAnsi)
{
    if (!Combo) return;

    Combo->ClearOptions();
    Combo->AddOption(TEXT(""));

    if (!MissionPtr) return;

    int index = 1;
    int selected_index = 0;

    // Maintain legacy List<> usage:
    List<MissionElement>& list = MissionPtr->GetElements();

    for (int i = 0; i < list.size(); i++)
    {
        MissionElement* elem = list[i];
        if (!elem) continue;

        if (elem->IsSquadron())
            continue;

        const FString BaseName = ANSI_TO_TCHAR(elem->Name().data());

        if (elem->Count() == 1)
        {
            Combo->AddOption(BaseName);

            if (SelectedAnsi && BaseName.Equals(ANSI_TO_TCHAR(SelectedAnsi), ESearchCase::IgnoreCase))
                selected_index = index;

            index++;
        }
        else
        {
            for (int n = 0; n < elem->Count(); n++)
            {
                const FString ShipName = FString::Printf(TEXT("%s %d"), *BaseName, n + 1);
                Combo->AddOption(ShipName);

                if (SelectedAnsi && ShipName.Equals(ANSI_TO_TCHAR(SelectedAnsi), ESearchCase::IgnoreCase))
                    selected_index = index;

                index++;
            }
        }
    }

    if (selected_index >= 0 && selected_index < Combo->GetOptionCount())
    {
        Combo->SetSelectedOption(Combo->GetOptionAtIndex(selected_index));
    }
}

void UMissionEventDlg::FillRgnList(UComboBoxString* Combo, const char* SelectedAnsi)
{
    if (!Combo) return;

    Combo->ClearOptions();
    if (!MissionPtr) return;

    int selected_index = 0;
    int i = 0;

    // Maintain legacy ListIter<> usage:
    ListIter<StarSystem> iter = MissionPtr->GetSystemList();
    while (++iter)
    {
        StarSystem* sys = iter.value();
        if (!sys) continue;

        ListIter<OrbitalRegion> iter2 = sys->AllRegions();
        while (++iter2)
        {
            OrbitalRegion* region = iter2.value();
            if (!region) continue;

            const FString RegionName = ANSI_TO_TCHAR(region->Name());

            if (SelectedAnsi && RegionName.Equals(ANSI_TO_TCHAR(SelectedAnsi), ESearchCase::IgnoreCase))
                selected_index = i;

            Combo->AddOption(RegionName);
            i++;
        }
    }

    if (selected_index >= 0 && selected_index < Combo->GetOptionCount())
    {
        Combo->SetSelectedOption(Combo->GetOptionAtIndex(selected_index));
    }
}

void UMissionEventDlg::RefreshTargetListForSelectedEvent()
{
    if (!MissionPtr || !EventPtr || !EventCombo)
        return;

    const int32 SelectedEventIndex = FindEventIndexByName(EventCombo->GetSelectedOption());

    if (SelectedEventIndex == MissionEvent::JUMP)
        FillRgnList(EventTargetCombo, EventPtr->EventTarget());
    else
        FillShipList(EventTargetCombo, EventPtr->EventTarget());
}

//////////////////////////////////////////////////////////////////////////
// Parsing helpers (declared static in header)

bool UMissionEventDlg::ParseFloatBox(UEditableTextBox* Box, float& OutValue)
{
    if (!Box) return false;
    const FString S = Box->GetText().ToString().TrimStartAndEnd();
    if (S.IsEmpty()) return false;
    OutValue = FCString::Atof(*S);
    return true;
}

bool UMissionEventDlg::ParseIntBox(UEditableTextBox* Box, int32& OutValue)
{
    if (!Box) return false;
    const FString S = Box->GetText().ToString().TrimStartAndEnd();
    if (S.IsEmpty()) return false;
    OutValue = FCString::Atoi(*S);
    return true;
}

//////////////////////////////////////////////////////////////////////////
// UMG callbacks

void UMissionEventDlg::OnEventSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    RefreshTargetListForSelectedEvent();
}

void UMissionEventDlg::OnAcceptClicked()
{
    if (!MissionPtr || !EventPtr)
        return;

    // --- time/delay ---
    if (TimeEdit)
    {
        float T = 0.f;
        if (ParseFloatBox(TimeEdit, T))
            EventPtr->SetTime((double)T);
    }

    if (DelayEdit)
    {
        float T = 0.f;
        if (ParseFloatBox(DelayEdit, T))
            EventPtr->SetDelay((double)T);
    }

    // --- event params (int OR "(...)" array) ---
    if (EventParamEdit)
    {
        const FString S = EventParamEdit->GetText().ToString().TrimStartAndEnd();

        int32 Params[10] = { 0 };
        int32 N = 0;

        if (!S.IsEmpty() && (FChar::IsDigit(S[0]) || S[0] == TEXT('-') || S[0] == TEXT('+')))
        {
            Params[0] = FCString::Atoi(*S);
            N = 1;
        }
        else
        {
            ParseLegacyIntArray(S, Params, 10, N);
        }

        if (N > 0)
            EventPtr->SetEventParams(Params, N);
        else
            EventPtr->ClearEventParams();
    }

    // --- chance ---
    if (ChanceEdit)
    {
        int32 V = 0;
        if (ParseIntBox(ChanceEdit, V))
            EventPtr->SetEventChance(V);
        else
            EventPtr->SetEventChance(0);
    }

    // --- message / sound ---
    if (EventMessageEdit)
    {
        const FString Msg = EventMessageEdit->GetText().ToString();
        EventPtr->SetEventMessage(TCHAR_TO_ANSI(*Msg));
    }

    if (EventSoundEdit)
    {
        const FString Snd = EventSoundEdit->GetText().ToString();
        EventPtr->SetEventSound(TCHAR_TO_ANSI(*Snd));
    }

    // --- trigger params (int OR "(...)" array) ---
    if (TriggerParamEdit)
    {
        const FString S = TriggerParamEdit->GetText().ToString().TrimStartAndEnd();

        int32 Params[10] = { 0 };
        int32 N = 0;

        if (!S.IsEmpty() && (FChar::IsDigit(S[0]) || S[0] == TEXT('-') || S[0] == TEXT('+')))
        {
            Params[0] = FCString::Atoi(*S);
            N = 1;
        }
        else
        {
            ParseLegacyIntArray(S, Params, 10, N);
        }

        if (N > 0)
            EventPtr->SetTriggerParams(Params, N);
        else
            EventPtr->ClearTriggerParams();
    }

    // --- event selection ---
    if (EventCombo)
    {
        const int32 Evt = FindEventIndexByName(EventCombo->GetSelectedOption());
        EventPtr->SetEvent(Evt);
    }

    // --- ship/source/target ---
    if (EventShipCombo)
        EventPtr->SetEventShip(TCHAR_TO_ANSI(*EventShipCombo->GetSelectedOption()));

    if (EventSourceCombo)
        EventPtr->SetEventSource(TCHAR_TO_ANSI(*EventSourceCombo->GetSelectedOption()));

    if (EventTargetCombo)
        EventPtr->SetEventTarget(TCHAR_TO_ANSI(*EventTargetCombo->GetSelectedOption()));

    // --- trigger selection ---
    if (TriggerCombo)
    {
        const int32 Trg = FindTriggerIndexByName(TriggerCombo->GetSelectedOption());
        EventPtr->SetTrigger(Trg);
    }

    // --- trigger ship/target ---
    if (TriggerShipCombo)
        EventPtr->SetTriggerShip(TCHAR_TO_ANSI(*TriggerShipCombo->GetSelectedOption()));

    if (TriggerTargetCombo)
        EventPtr->SetTriggerTarget(TCHAR_TO_ANSI(*TriggerTargetCombo->GetSelectedOption()));

    // Legacy: return to mission edit dlg
    if (Manager)
    {
        Manager->ShowMissionEditorDlg();
    }
}

void UMissionEventDlg::OnCancelClicked()
{
    if (Manager)
    {
        Manager->ShowMissionEditorDlg();
    }
}
