#pragma once
#include "FractalVerseState.h"

// Runtime control lease; only trusted server provisioning may assign an object.
// No raw ship pointers or client clocks survive here. Sequence numbers never wrap.
class FFractalVerseFlightGate
{
public:
    bool Bind(FName Principal, uint32 ObjectId)
    {
        if (Principal.IsNone() || !ObjectId) return false;
        for (const auto& Pair : Entries)
            if (Pair.Key != Principal && Pair.Value.ObjectId == ObjectId) return false;
        if (const auto* Current = Entries.Find(Principal)) return Current->ObjectId == ObjectId;
        FEntry Entry; Entry.ObjectId=ObjectId; Entries.Add(Principal,Entry); return true;
    }
    uint32 Object(FName Principal) const
    { const auto* Entry=Entries.Find(Principal); return Entry?Entry->ObjectId:0; }
    bool Submit(FName Principal, const FFractalVerseFlightInput& Input, double Now)
    {
        auto* Entry=Entries.Find(Principal);
        if (!Entry || !FMath::IsFinite(Now) || Now<0 || Input.Sequence<=Entry->Input.Sequence ||
            Input.Rotation.ContainsNaN() || Input.Translation.ContainsNaN() || !FMath::IsFinite(Input.Throttle) ||
            Input.Rotation.GetAbsMax()>1 || Input.Translation.GetAbsMax()>1 || Input.Throttle<0 || Input.Throttle>100 ||
            (Entry->Last>=0 && (Now<Entry->Last || Now-Entry->Last<.025))) return false;
        Entry->Input=Input; Entry->Last=Now; return true;
    }
    FFractalVerseFlightInput Read(FName Principal,double Now) const
    {
        const auto* Entry=Entries.Find(Principal);
        if (!Entry || Entry->Last<0 || !FMath::IsFinite(Now) || Now<Entry->Last || Now-Entry->Last>.25)
            return FFractalVerseFlightInput();
        return Entry->Input;
    }
    void Revoke(FName Principal) { Entries.Remove(Principal); }
    void Reset() { Entries.Empty(); }
private:
    struct FEntry { uint32 ObjectId=0; double Last=-1; FFractalVerseFlightInput Input; };
    TMap<FName,FEntry> Entries;
};
