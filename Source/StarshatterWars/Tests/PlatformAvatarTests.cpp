#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "PlatformAvatarSubsystem.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlatformAvatarCacheTest, "Starshatter.UI.PlatformAvatar.AccountIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlatformAvatarCacheTest::RunTest(const FString&)
{
    auto* Instance = NewObject<UGameInstance>();
    auto* Cache = NewObject<UPlatformAvatarSubsystem>(Instance);
    TestNull(TEXT("Offline has no platform image"), Cache->GetBrush());
    Cache->SetAccount(TEXT("EOS"), TEXT("epic-a"));
    Cache->Refresh();
    TestNull(TEXT("EOS does not fabricate a platform avatar"), Cache->GetBrush());
    TestTrue(TEXT("Epic fallback explains availability"), Cache->GetStatus().ToString().Contains(TEXT("EOS")));

    // Seed a downloaded texture without depending on Steam or a network account.
    auto* Property = FindFProperty<FObjectPropertyBase>(Cache->GetClass(), TEXT("Texture"));
    if (!TestNotNull(TEXT("Avatar texture is GC tracked"), Property)) return false;
    Property->SetObjectPropertyValue_InContainer(Cache, NewObject<UTexture2D>());
    Cache->SetAccount(TEXT("EOS"), TEXT("epic-a"));
    TestNotNull(TEXT("Same account retains its cached image"), Cache->GetBrush());
    Cache->SetAccount(TEXT("EOS"), TEXT("epic-b"));
    TestNull(TEXT("Account switch clears previous image"), Cache->GetBrush());
    Property->SetObjectPropertyValue_InContainer(Cache, NewObject<UTexture2D>());
    Cache->SetAccount(TEXT("STEAM"), TEXT("steam-a"));
    TestNull(TEXT("Provider switch also clears previous image"), Cache->GetBrush());
    Cache->SetAccount(NAME_None, FString());
    TestEqual(TEXT("Offline status restored"), Cache->GetStatus().ToString(), FString(TEXT("Offline pilot")));
    return true;
}
#endif
