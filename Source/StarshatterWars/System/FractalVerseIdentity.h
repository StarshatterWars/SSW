#pragma once

#include "CoreMinimal.h"

struct FFractalVerseVerifiedIdentity
{
    FName Principal;
    FString LegacySlot;
};

// Identity proof is verified by EOS, not decoded and trusted by the game.
class FFractalVerseIdentity
{
public:
    static bool IsProofWellFormed(const FString& ProductUserId, const FString& Jwt);
    static bool CopyLocalProof(FString& ProductUserId, FString& Jwt);
    static void VerifyAccount(const FString& ProductUserId,const FString& Jwt,TFunction<void(FFractalVerseVerifiedIdentity)> Completion);
    static void Verify(const FString& ProductUserId, const FString& Jwt, TFunction<void(FName)> Completion);
};
