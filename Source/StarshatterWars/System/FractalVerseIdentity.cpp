#include "FractalVerseIdentity.h"
#include "FractalVersePilotMigration.h"
#include "Async/Async.h"
#include "OnlineSubsystem.h"
#include "IOnlineSubsystemEOS.h"
#include "IEOSSDKManager.h"
#include "eos_connect.h"
#include "eos_sdk.h"
#include "EOSSettings.h"

namespace
{
IEOSPlatformHandlePtr IdentityPlatform()
{
    auto* Subsystem=IOnlineSubsystem::Get(TEXT("EOS"));
    if (!Subsystem || Subsystem->GetSubsystemName()!=FName(TEXT("EOS"))) return nullptr;
    return static_cast<IOnlineSubsystemEOS*>(Subsystem)->GetEOSPlatformHandle();
}
struct FIdentityVerification
{
    IEOSPlatformHandlePtr Platform;
    TFunction<void(FFractalVerseVerifiedIdentity)> Completion;
    FString Product, Sandbox, Deployment, ExpectedUser;
    TUniquePtr<FTCHARToUTF8> TokenText;
    EOS_Connect_IdToken Token{};
};
void EOS_CALL IdentityVerified(const EOS_Connect_VerifyIdTokenCallbackInfo* Data)
{
    TUniquePtr<FIdentityVerification> Request(static_cast<FIdentityVerification*>(Data->ClientData));
    FFractalVerseVerifiedIdentity Identity;
    if (Data->ResultCode==EOS_EResult::EOS_Success && Data->bIsAccountInfoPresent &&
        Data->AccountId && Data->AccountId[0] && Data->ProductId && Data->SandboxId && Data->DeploymentId &&
        Request->Product==UTF8_TO_TCHAR(Data->ProductId) && Request->Sandbox==UTF8_TO_TCHAR(Data->SandboxId) &&
        Request->Deployment==UTF8_TO_TCHAR(Data->DeploymentId))
    {
        char User[EOS_PRODUCTUSERID_MAX_LENGTH+1]{}; int32 Length=UE_ARRAY_COUNT(User);
        if (EOS_ProductUserId_ToString(Data->ProductUserId,User,&Length)==EOS_EResult::EOS_Success &&
            Request->ExpectedUser.Equals(UTF8_TO_TCHAR(User),ESearchCase::IgnoreCase))
        {
            Identity.Principal=FName(*(FString(TEXT("EOS:"))+FString(UTF8_TO_TCHAR(User)).ToLower()));
            if (Data->AccountIdType==EOS_EExternalAccountType::EOS_EAT_EPIC)
                Identity.LegacySlot=FFractalVersePilotMigration::EpicAccountSlot(UTF8_TO_TCHAR(Data->AccountId),UTF8_TO_TCHAR(User));
        }
    }
    auto Completion=MoveTemp(Request->Completion);
    AsyncTask(ENamedThreads::GameThread,[Completion=MoveTemp(Completion),Identity]() mutable { Completion(Identity); });
}
}

bool FFractalVerseIdentity::IsProofWellFormed(const FString& User, const FString& Jwt)
{
    if (User.Len()!=32 || Jwt.Len()<32 || Jwt.Len()>16384) return false;
    for (TCHAR C:User) if (!FChar::IsHexDigit(C)) return false;
    int32 Dots=0, SegmentLength=0;
    for (TCHAR C:Jwt)
    {
        if (C==TEXT('.')) { if (!SegmentLength) return false; ++Dots; SegmentLength=0; }
        else { if (!((C>=TEXT('A')&&C<=TEXT('Z')) || (C>=TEXT('a')&&C<=TEXT('z')) ||
                      (C>=TEXT('0')&&C<=TEXT('9')) || C==TEXT('_') || C==TEXT('-'))) return false; ++SegmentLength; }
    }
    return Dots==2 && SegmentLength>0;
}

bool FFractalVerseIdentity::CopyLocalProof(FString& User, FString& Jwt)
{
    User.Empty(); Jwt.Empty();
    const auto Platform=IdentityPlatform(); if (!Platform) return false;
    const auto Connect=EOS_Platform_GetConnectInterface(*Platform);
    // This prototype supports the launcher's single signed-in local user only.
    if (EOS_Connect_GetLoggedInUsersCount(Connect)!=1) return false;
    EOS_Connect_CopyIdTokenOptions Options{}; Options.ApiVersion=EOS_CONNECT_COPYIDTOKEN_API_LATEST;
    Options.LocalUserId=EOS_Connect_GetLoggedInUserByIndex(Connect,0);
    EOS_Connect_IdToken* Token=nullptr;
    if (EOS_Connect_CopyIdToken(Connect,&Options,&Token)!=EOS_EResult::EOS_Success || !Token) return false;
    char Buffer[EOS_PRODUCTUSERID_MAX_LENGTH+1]{}; int32 Length=UE_ARRAY_COUNT(Buffer);
    const bool Valid=Token->JsonWebToken && EOS_ProductUserId_ToString(Token->ProductUserId,Buffer,&Length)==EOS_EResult::EOS_Success;
    if (Valid) { User=UTF8_TO_TCHAR(Buffer); Jwt=UTF8_TO_TCHAR(Token->JsonWebToken); }
    EOS_Connect_IdToken_Release(Token);
    return Valid && IsProofWellFormed(User,Jwt);
}

void FFractalVerseIdentity::VerifyAccount(const FString& User, const FString& Jwt, TFunction<void(FFractalVerseVerifiedIdentity)> Completion)
{
    if (!IsProofWellFormed(User,Jwt)) { Completion(FFractalVerseVerifiedIdentity()); return; }
    const auto Platform=IdentityPlatform();
    if (!Platform) { Completion(FFractalVerseVerifiedIdentity()); return; }
    FString Product,Sandbox,Deployment;
    const auto& Settings=UEOSSettings::GetSettings();
    if (Settings.bUseNamedPlatformConfig)
    {
        auto* Manager=IEOSSDKManager::Get();
        const auto* Config=Manager ? Manager->GetPlatformConfig(Settings.PlatformConfigName) : nullptr;
        if (Config) { Product=Config->ProductId; Sandbox=Config->SandboxId; Deployment=Config->DeploymentId; }
    }
    else
    {
        FEOSArtifactSettings Artifact;
        if (UEOSSettings::GetSelectedArtifactSettings(Artifact))
        { Product=Artifact.ProductId; Sandbox=Artifact.SandboxId; Deployment=Artifact.DeploymentId; }
    }
    if (Product.IsEmpty() || Sandbox.IsEmpty() || Deployment.IsEmpty()) { Completion(FFractalVerseVerifiedIdentity()); return; }
    auto* Request=new FIdentityVerification;
    Request->Platform=Platform; Request->Completion=MoveTemp(Completion);
    Request->Product=Product; Request->Sandbox=Sandbox; Request->Deployment=Deployment;
    Request->ExpectedUser=User; Request->TokenText=MakeUnique<FTCHARToUTF8>(*Jwt);
    Request->Token.ApiVersion=EOS_CONNECT_IDTOKEN_API_LATEST;
    Request->Token.ProductUserId=EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*User));
    Request->Token.JsonWebToken=Request->TokenText->Get();
    EOS_Connect_VerifyIdTokenOptions Options{}; Options.ApiVersion=EOS_CONNECT_VERIFYIDTOKEN_API_LATEST; Options.IdToken=&Request->Token;
    EOS_Connect_VerifyIdToken(EOS_Platform_GetConnectInterface(*Platform),&Options,Request,&IdentityVerified);
}

void FFractalVerseIdentity::Verify(const FString& User,const FString& Jwt,TFunction<void(FName)> Completion)
{
    VerifyAccount(User,Jwt,[Completion=MoveTemp(Completion)](FFractalVerseVerifiedIdentity Identity) mutable { Completion(Identity.Principal); });
}
