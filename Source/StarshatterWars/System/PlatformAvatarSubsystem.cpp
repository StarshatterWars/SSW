#include "PlatformAvatarSubsystem.h"
#include "Engine/Texture2D.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#if SSW_STEAM_AVATARS
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END
#endif

void UPlatformAvatarSubsystem::SetAccount(FName Provider, const FString& AccountId)
{
    if (AccountProvider == Provider && Account == AccountId) return;
    AccountProvider = Provider;
    Account = AccountId;
    Brush.SetResourceObject(nullptr);
    Texture = nullptr;
    Attempts = 0;
}

void UPlatformAvatarSubsystem::Refresh()
{
#if SSW_STEAM_AVATARS
    if (AccountProvider != TEXT("STEAM") || Account.IsEmpty()) return;
    auto* Online = IOnlineSubsystem::Get(TEXT("STEAM"));
    auto Identity = Online ? Online->GetIdentityInterface() : nullptr;
    auto Id = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
    if (!Id.IsValid() || Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn || Id->ToString() != Account)
    {
        Brush.SetResourceObject(nullptr);
        Texture = nullptr;
        return;
    }
    if (Texture || Attempts >= 30) return;
    ++Attempts;
    // OnlineSubsystem owns Steam initialization and callback dispatch. Never start it here.
    auto* Friends = SteamFriends();
    auto* Utils = SteamUtils();
    auto* User = SteamUser();
    if (!Friends || !Utils || !User || FString::Printf(TEXT("%llu"), User->GetSteamID().ConvertToUint64()) != Account) return;
    const int Handle = Friends->GetLargeFriendAvatar(User->GetSteamID());
    if (Handle < 0) return; // Pending download; the visible panel retries, at most once a second.
    if (Handle == 0) { Attempts = 30; return; }
    uint32 Width = 0, Height = 0;
    if (!Utils->GetImageSize(Handle, &Width, &Height) || Width == 0 || Height == 0 || Width > 512 || Height > 512) return;
    TArray<uint8> Pixels;
    Pixels.SetNumUninitialized(Width * Height * 4);
    if (!Utils->GetImageRGBA(Handle, Pixels.GetData(), Pixels.Num())) return;
    Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
    if (!Texture) return;
    Texture->SRGB = true;
    auto& Mip = Texture->GetPlatformData()->Mips[0];
    void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num());
    Mip.BulkData.Unlock();
    Texture->UpdateResource();
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = FVector2D(64,64);
    Brush.DrawAs = ESlateBrushDrawType::Image;
#endif
}

FText UPlatformAvatarSubsystem::GetStatus() const
{
    if (Texture) return INVTEXT("Steam account avatar");
    if (AccountProvider == TEXT("EOS")) return INVTEXT("Epic account — avatar unavailable through EOS");
    if (AccountProvider == TEXT("STEAM")) return INVTEXT("Steam account — avatar unavailable or loading");
    return INVTEXT("Offline pilot");
}
