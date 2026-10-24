#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarshatterAudioSubsystem.h"
#include "StarshatterAudioSettings.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundClass.h"
#include "Engine/GameInstance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioRuntimeSettingsTest,"Starshatter.Audit.F09.AudioRuntimeSettings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioRuntimeSettingsTest::RunTest(const FString& Parameters)
{
    auto* GI=NewObject<UGameInstance>();auto* Audio=NewObject<UStarshatterAudioSubsystem>(GI);
    auto* Settings=NewObject<UStarshatterAudioSettings>();
    Settings->SetMasterVolume(0.4f);Settings->SetMusicVolume(0.2f);Settings->SetEffectsVolume(0.6f);Settings->SetVoiceVolume(0.8f);
    Audio->ApplyRuntimeSettings(*Settings);
    TestEqual(TEXT("Master applies independently"),Audio->GetMasterGain(),0.4f);
    TestEqual(TEXT("Music channel gain"),Audio->GetChannelGain(EStarshatterAudioChannel::Music),0.2f);
    TestEqual(TEXT("Effects channel gain"),Audio->GetChannelGain(EStarshatterAudioChannel::Effects),0.6f);
    TestEqual(TEXT("Voice channel gain"),Audio->GetChannelGain(EStarshatterAudioChannel::Voice),0.8f);
    UAudioComponent* Component=NewObject<UAudioComponent>(GI);USoundClass* Original=NewObject<USoundClass>(GI);
    Component->SoundClassOverride=Original;Component->SetVolumeMultiplier(0.35f);
    Audio->RouteAudioComponent(Component,EStarshatterAudioChannel::Effects);
    USoundClass* Routed=Component->SoundClassOverride;
    TestTrue(TEXT("Category class preserves original class ancestry"),Routed && Routed!=Original && Routed->ParentClass==Original);
    TestEqual(TEXT("Routing preserves authored/fade volume multiplier"),Component->VolumeMultiplier,0.35f);
    Audio->RouteAudioComponent(Component,EStarshatterAudioChannel::Effects);
    TestTrue(TEXT("Repeated routing does not nest or duplicate classes"),Component->SoundClassOverride==Routed);
    Settings->SetEffectsVolume(0);Audio->ApplyRuntimeSettings(*Settings);
    TestEqual(TEXT("Effects mute is exact zero"),Audio->GetChannelGain(EStarshatterAudioChannel::Effects),0.f);
    TestEqual(TEXT("Changing effects preserves voice"),Audio->GetChannelGain(EStarshatterAudioChannel::Voice),0.8f);
    TestEqual(TEXT("Repeated application cannot compound component volume"),Component->VolumeMultiplier,0.35f);
    TestTrue(TEXT("Music files route to music"),SSWAudioRouting::InferChannel(TEXT("/Game/Audio/Music/Track.Track"))==EStarshatterAudioChannel::Music);
    TestTrue(TEXT("Radio files route to voice"),SSWAudioRouting::InferChannel(TEXT("/Game/Audio/Vox/M1.M1"))==EStarshatterAudioChannel::Voice);
    Audio->RouteAudioComponent(Component,EStarshatterAudioChannel::Voice);
    TestTrue(TEXT("Changing channel keeps original ancestor"),Component->SoundClassOverride->ParentClass==Original && Component->SoundClassOverride!=Routed);
    return true;
}
#endif
