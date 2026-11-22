#pragma once

#include "CoreMinimal.h"
#include "LocalGameStateSave.h"
#include "PlayerProgressSaveTransaction.h"
#include "UObject/StrongObjectPtr.h"

struct FFractalVersePilotCandidate
{
    FS_PlayerGameInfo Pilot;
    TStrongObjectPtr<ULocalUniverseState> World;
    FString BaseSlot;
    FString ProfileSlot;
    FString BundleDigest;
    FString CareerJson;
    int64 Credits = 0;
    int32 SourceSaveVersion = 0;
    bool bRecoveredPrevious = false;
    // Exact source bytes retained for an immutable, verified backup before promotion.
    TMap<FString,TArray<uint8>> Sources;
};

class FFractalVersePilotMigration
{
public:
    // Call with identities returned by EOS verification, never client claims.
    static FString EpicAccountSlot(const FString& EpicAccountId,const FString& ProductUserId);
    // Reads only. Pending local save transactions must be recovered by the old game first.
    static bool Read(IPlayerProgressSlotIO& TrustedSource,const FString& VerifiedBaseSlot,FFractalVersePilotCandidate& Out,FString& Reason);
    static bool ReadDirectory(const FString& TrustedServerSourceDirectory,const FString& VerifiedBaseSlot,FFractalVersePilotCandidate& Out,FString& Reason);
    static bool Revalidate(const FFractalVersePilotCandidate&,FFractalVersePilotCandidate& Canonical,FString& Reason);
    // Stages an immutable bundle; does not mutate or replace a live server pilot.
    static bool Stage(const FString& ServerDirectory,FName VerifiedPrincipal,const FFractalVersePilotCandidate&,FString& BundlePath,FString& Reason);
};
