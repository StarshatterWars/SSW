
#include "RadioVox.h"
#include "AudioRouting.h"
#include "RadioView.h"
#include "AudioConfig.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Containers/Ticker.h"
#include "UObject/WeakObjectPtr.h"

namespace
{
    TWeakObjectPtr<UObject> RadioWorldContext;
    FString PhraseKey(const FString& Phrase)
    {
        FString Key;
        for (TCHAR C : Phrase.ToLower()) Key.AppendChar(FChar::IsAlnum(C) ? C : TCHAR('_'));
        return Key;
    }
    const TMap<FString,FString>& PhrasePaths()
    {
        // Literal pointers are constant-initialized data, not thousands of
        // FString constructors expanded into this function by the compiler.
        struct FPhrasePathEntry
        {
            const TCHAR* Key;
            const TCHAR* Path;
        };
        static const FPhrasePathEntry Entries[] = {
#include "RadioAudioAssets.inl"
        };
        static const TMap<FString, FString> Paths = []()
        {
            TMap<FString, FString> Result;
            Result.Reserve(UE_ARRAY_COUNT(Entries));
            for (const FPhrasePathEntry& Entry : Entries)
                Result.Add(FString(Entry.Key), FString(Entry.Path));
            return Result;
        }();
        return Paths;
    }
}

class RadioVoxController
{
public:
    List<RadioVox> Queue;
    FTSTicker::FDelegateHandle TickHandle;
    RadioVoxController()
    {
        TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float)
        {
            UWorld* World = RadioWorldContext.IsValid() ? RadioWorldContext->GetWorld() : nullptr;
            if (!World || World->IsPaused()) return true;
            if (Queue.size() && !Queue.first()->Update()) delete Queue.removeIndex(0);
            return true;
        }), 0.02f);
    }
    ~RadioVoxController() { FTSTicker::GetCoreTicker().RemoveTicker(TickHandle); Queue.destroy(); }
    bool Add(RadioVox* Vox)
    {
        if (!Vox || Vox->PhraseAssets.IsEmpty() || Queue.size() >= 16) return false;
        Queue.append(Vox); return true;
    }
};
static RadioVoxController* RadioAudioController = nullptr;

void RadioVox::Initialize(UObject* WorldContext)
{
    if (WorldContext) RadioWorldContext = WorldContext;
    if (!RadioAudioController) RadioAudioController = new RadioVoxController;
}
void RadioVox::Close()
{
    delete RadioAudioController; RadioAudioController = nullptr; RadioWorldContext.Reset();
}
void RadioVox::DiscardMessages() { if (RadioAudioController) RadioAudioController->Queue.destroy(); }
RadioVox::RadioVox(int n, const char* p, const char* m) : path(p), message(m), index(0), channel(n) {}
RadioVox::~RadioVox()
{
    if (PlayingAudio.IsValid()) { PlayingAudio->Stop(); PlayingAudio->DestroyComponent(); }
    PlayingAudio.Reset(); PhraseAssets.Empty();
}
bool RadioVox::AddPhrase(const char* Key)
{
    if (!Key || !*Key || AudioConfig::VoxVolume() <= AudioConfig::Silence()) return false;
    FString Name = PhraseKey(ANSI_TO_TCHAR(Key));
    // Legacy command text and the imported recording occasionally differ.
    if (Name == TEXT("bracket")) Name = TEXT("bracket_target");
    const FString Lookup = FString(ANSI_TO_TCHAR(path.data())) + TEXT("|") + Name;
    const FString* AssetPath = PhrasePaths().Find(Lookup);
    if (!AssetPath) return false; // Never substitute an unrelated acknowledgement.
    USoundBase* Sound = LoadObject<USoundBase>(nullptr, **AssetPath);
    if (!Sound) return false;
    PhraseAssets.Emplace(Sound);
    return true;
}
bool RadioVox::Start() { return RadioAudioController && RadioWorldContext.IsValid() && RadioAudioController->Add(this); }
bool RadioVox::Update()
{
    if (message.length()) { RadioView::Message(message.data()); message = ""; }
    if (PlayingAudio.IsValid())
    {
        if (PlayingAudio->IsPlaying()) return true;
        PlayingAudio->DestroyComponent(); PlayingAudio.Reset();
    }
    while (index < PhraseAssets.Num())
    {
        USoundBase* Sound = PhraseAssets[index++].Get();
        if (!Sound || !RadioWorldContext.IsValid()) continue;
        const float Volume = FMath::Pow(10.0f, float(AudioConfig::VoxVolume()) / 2000.0f);
        UAudioComponent* Audio = UGameplayStatics::CreateSound2D(RadioWorldContext.Get(), Sound,
            Volume, 1.0f, 0.0f, nullptr, false, false);
        if (!Audio) continue;
        SSWAudioRouting::Route(Audio,EStarshatterAudioChannel::Voice);
        Audio->Play();
        Audio->bIsUISound = false;
        PlayingAudio.Reset(Audio);
        return true;
    }
    return false;
}
