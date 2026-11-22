#pragma once

#include "FractalVerseState.h"

struct FFractalVersePilotCandidate;

// Trusted server context, never supplied by an RPC payload.
struct FFractalVerseAccess
{
    FName Principal;
    bool bDocked = false;
    TSet<FName> AccessibleInventories;
};

class FFractalVerseAuthority
{
public:
    FFractalVerseAuthority() { World.Universe = FGuid::NewGuid(); }
    const FFractalVerseWorld& Read() const { return World; }
    FFractalVerseStatus Status(bool bPersistenceReady) const;
    bool SynchronizeClock(int64 ServerUtc);
    // Called only after identity verification on the server. No profile or balance payload.
    bool EnsurePilot(FName VerifiedPrincipal, FString& Reason);
    bool ImportPilot(FName VerifiedPrincipal,const FFractalVersePilotCandidate&,FString& Reason);
    bool PilotView(FName VerifiedPrincipal, FFractalVersePilotView& Out) const;
    bool Transfer(const FFractalVerseAccess&, const FFractalVerseTransfer&, FString& Reason);
    bool Restore(const FFractalVerseWorld&, FString& Reason);
    // Only server simulation code may advance the authoritative world.
    bool Advance(double Seconds, TFunctionRef<void(FFractalVerseWorld&, double)> Simulation);
private:
    friend class UFractalVerseSubsystem;
    FFractalVerseWorld World;
};

class FFractalVerseCheckpointStore
{
public:
    static bool Save(const FString& Directory, const FFractalVerseWorld&, FString& Reason);
    static bool Load(const FString& Directory, FFractalVerseWorld&, FString& Reason);
};
