#pragma once

#include "FractalVerseState.h"

struct FFractalVersePilotCandidate;
class FFractalVersePilotPromotion
{
public:
    static FGuid ScopedId(FName Principal,FGuid Legacy);
    // Builds a candidate only. Caller must checkpoint it before acknowledging a login.
    static bool Build(const FFractalVerseWorld& Current,FName VerifiedPrincipal,const FFractalVersePilotCandidate&,FFractalVerseWorld& Out,FString& Reason);
};

class FFractalVerseAuthority;
class FFractalVersePilotCommit
{
public:
    static bool Commit(FFractalVerseAuthority&,FName Principal,const FFractalVersePilotCandidate&,
        TFunctionRef<bool()> Save,FString& Reason);
};
