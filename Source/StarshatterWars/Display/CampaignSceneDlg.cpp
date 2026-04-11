#include "CampaignSceneDlg.h"

#include "CmpnScreen.h"
#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UCampaignSceneDlg::UCampaignSceneDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCampaignSceneDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeWidgets();

    SetVisibility(ESlateVisibility::Collapsed);
    SetIsEnabled(false);

    ResetSceneState();
}

void UCampaignSceneDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bSceneRunning)
    {
        return;
    }

    const float NowSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());
    AdvanceSceneFromTimer(NowSeconds);
}

UFont* UCampaignSceneDlg::GetRegularLimerickFont() const
{
    static UFont* CachedFont = nullptr;

    if (!CachedFont)
    {
        CachedFont = LoadObject<UFont>(
            nullptr,
            TEXT("/Game/Font/limerick-7_Font.limerick-7_Font"));
    }

    return CachedFont;
}

UFont* UCampaignSceneDlg::GetBoldLimerickFont() const
{
    static UFont* CachedFont = nullptr;

    if (!CachedFont)
    {
        CachedFont = LoadObject<UFont>(
            nullptr,
            TEXT("/Game/Font/Limerick-Serial_Bold_Font.Limerick-Serial_Bold_Font"));
    }

    return CachedFont;
}

FString UCampaignSceneDlg::FixEscapedText(const FString& InText)
{
    FString Out = InText;
    Out.ReplaceInline(TEXT("\\r\\n"), TEXT("\n"));
    Out.ReplaceInline(TEXT("\\n"), TEXT("\n"));
    Out.ReplaceInline(TEXT("\\\""), TEXT("\""));
    return Out.TrimStartAndEnd();
}

int32 UCampaignSceneDlg::ResolveCampaignNumber() const
{
    return CurrentCampaignNumber > 0 ? CurrentCampaignNumber : 2;
}

FString UCampaignSceneDlg::ResolveSceneSoundPath(const FString& SoundToken) const
{
    const FString CleanToken = FixEscapedText(SoundToken);
    if (CleanToken.IsEmpty())
    {
        return FString();
    }

    return FString::Printf(
        TEXT("/Game/Audio/Vox/Scenes/%02d/%s.%s"),
        ResolveCampaignNumber(),
        *CleanToken,
        *CleanToken);
}

USoundBase* UCampaignSceneDlg::ResolveSceneSound(const FString& SoundToken) const
{
    static TMap<FString, TObjectPtr<USoundBase>> SoundCache;

    const FString AssetPath = ResolveSceneSoundPath(SoundToken);
    if (AssetPath.IsEmpty())
    {
        return nullptr;
    }

    if (const TObjectPtr<USoundBase>* Found = SoundCache.Find(AssetPath))
    {
        return Found->Get();
    }

    USoundBase* Sound = LoadObject<USoundBase>(nullptr, *AssetPath);
    if (Sound)
    {
        SoundCache.Add(AssetPath, Sound);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ResolveSceneSound failed: Token=%s Path=%s"),
            *SoundToken,
            *AssetPath);
    }

    return Sound;
}

void UCampaignSceneDlg::BuildRuntimeWidgets()
{
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: WidgetTree is null"));
        return;
    }

    if (!RuntimeHost)
    {
        RuntimeHost = Cast<UBorder>(GetWidgetFromName(TEXT("RuntimeHost")));
    }

    if (!RuntimeHost)
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: RuntimeHost not found"));
        return;
    }

    RuntimeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RuntimeOverlay"));
    if (!RuntimeOverlay)
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: failed to create RuntimeOverlay"));
        return;
    }

    RuntimeHost->SetContent(RuntimeOverlay);

    FLinearColor SceneGold = MissionUIStyle::RowSelected;
    SceneGold.A = 0.85f;

    const float WrapWidth = 1800.0f;
    const FSlateFontInfo HeaderFont = MissionUIStyle::GetLimerickFont(18);
    const FSlateFontInfo TitleFont = MissionUIStyle::GetLimerickFont(18);
    const FSlateFontInfo SubtitleFont = MissionUIStyle::GetLimerickFont(18);
    const FSlateFontInfo CaptionFont = MissionUIStyle::GetLimerickFont(18);

    HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeaderText"));
    if (HeaderText)
    {
        HeaderText->SetText(FText::GetEmpty());
        HeaderText->SetColorAndOpacity(FSlateColor(SceneGold));
        HeaderText->SetAutoWrapText(true);
        HeaderText->SetWrapTextAt(WrapWidth);
        HeaderText->SetJustification(ETextJustify::Left);
        HeaderText->SetMinDesiredWidth(WrapWidth);
        HeaderText->SetFont(HeaderFont);
        HeaderText->SetShadowOffset(FVector2D(1.0f, 1.0f));
        HeaderText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));

        UOverlaySlot* HeaderSlot = RuntimeOverlay->AddChildToOverlay(HeaderText);
        if (HeaderSlot)
        {
            HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
            HeaderSlot->SetVerticalAlignment(VAlign_Top);
            HeaderSlot->SetPadding(FMargin(32.0f, 24.0f, 32.0f, 0.0f));
        }
    }

    MessageTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageTitleText"));
    if (MessageTitleText)
    {
        MessageTitleText->SetText(FText::GetEmpty());
        MessageTitleText->SetColorAndOpacity(FSlateColor(SceneGold));
        MessageTitleText->SetAutoWrapText(true);
        MessageTitleText->SetWrapTextAt(WrapWidth);
        MessageTitleText->SetJustification(ETextJustify::Left);
        MessageTitleText->SetMinDesiredWidth(WrapWidth);
        MessageTitleText->SetFont(TitleFont);
        MessageTitleText->SetShadowOffset(FVector2D(1.0f, 1.0f));
        MessageTitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));

        UOverlaySlot* TitleSlot = RuntimeOverlay->AddChildToOverlay(MessageTitleText);
        if (TitleSlot)
        {
            TitleSlot->SetHorizontalAlignment(HAlign_Fill);
            TitleSlot->SetVerticalAlignment(VAlign_Top);
            TitleSlot->SetPadding(FMargin(32.0f, 88.0f, 32.0f, 0.0f));
        }
    }

    MessageSubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageSubtitleText"));
    if (MessageSubtitleText)
    {
        MessageSubtitleText->SetText(FText::GetEmpty());
        MessageSubtitleText->SetColorAndOpacity(FSlateColor(SceneGold));
        MessageSubtitleText->SetAutoWrapText(true);
        MessageSubtitleText->SetWrapTextAt(WrapWidth);
        MessageSubtitleText->SetJustification(ETextJustify::Left);
        MessageSubtitleText->SetMinDesiredWidth(WrapWidth);
        MessageSubtitleText->SetFont(SubtitleFont);
        MessageSubtitleText->SetShadowOffset(FVector2D(1.0f, 1.0f));
        MessageSubtitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));

        UOverlaySlot* SubtitleSlot = RuntimeOverlay->AddChildToOverlay(MessageSubtitleText);
        if (SubtitleSlot)
        {
            SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
            SubtitleSlot->SetVerticalAlignment(VAlign_Top);
            SubtitleSlot->SetPadding(FMargin(32.0f, 122.0f, 32.0f, 0.0f));
        }
    }

    CaptionTextBottom = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CaptionTextBottom"));
    if (CaptionTextBottom)
    {
        CaptionTextBottom->SetText(FText::GetEmpty());
        CaptionTextBottom->SetColorAndOpacity(FSlateColor(SceneGold));
        CaptionTextBottom->SetAutoWrapText(true);
        CaptionTextBottom->SetWrapTextAt(WrapWidth);
        CaptionTextBottom->SetJustification(ETextJustify::Center);
        CaptionTextBottom->SetMinDesiredWidth(WrapWidth);
        CaptionTextBottom->SetFont(CaptionFont);
        CaptionTextBottom->SetShadowOffset(FVector2D(1.0f, 1.0f));
        CaptionTextBottom->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));

        UOverlaySlot* CaptionSlot = RuntimeOverlay->AddChildToOverlay(CaptionTextBottom);
        if (CaptionSlot)
        {
            CaptionSlot->SetHorizontalAlignment(HAlign_Fill);
            CaptionSlot->SetVerticalAlignment(VAlign_Bottom);
            CaptionSlot->SetPadding(FMargin(100.0f, 0.0f, 100.0f, 60.0f));
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] Runtime widgets created"));
}

void UCampaignSceneDlg::ResetSceneState()
{
    ActiveMissionData = FS_CampaignMission();
    SortedEvents.Empty();

    DisplayBlocks.Empty();
    SortedDisplayTimes.Empty();
    NextDisplayBlockIndex = 0;
    NextMessageEventIndex = 0;

    ActiveSceneName.Empty();
    SceneStartRealSeconds = 0.0f;
    SceneDurationSeconds = 0.0f;
    bSceneRunning = false;

    if (HeaderText)
    {
        HeaderText->SetText(FText::GetEmpty());
    }

    if (MessageTitleText)
    {
        MessageTitleText->SetText(FText::GetEmpty());
    }

    if (MessageSubtitleText)
    {
        MessageSubtitleText->SetText(FText::GetEmpty());
    }

    if (CaptionTextBottom)
    {
        CaptionTextBottom->SetText(FText::GetEmpty());
    }
}

void UCampaignSceneDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);

    UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] SHOW"));
}

void UCampaignSceneDlg::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);
    SetIsEnabled(false);

    UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] HIDE"));
}

void UCampaignSceneDlg::LoadSceneFromMissionData(const FS_CampaignMission& MissionData)
{
    ResetSceneState();

    ActiveMissionData = MissionData;

    if (HeaderText)
    {
        const FString Header =
            !MissionData.Objective.IsEmpty() ? MissionData.Objective :
            !MissionData.MissionName.IsEmpty() ? MissionData.MissionName :
            !MissionData.Scene.IsEmpty() ? MissionData.Scene :
            TEXT("MISSION BRIEFING");

        HeaderText->SetText(FText::FromString(FixEscapedText(Header)));
    }

    BuildSortedEventQueue();

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] LoadSceneFromMissionData: Mission=%s Scene=%s Objective=%s Events=%d"),
        *MissionData.MissionName,
        *MissionData.Scene,
        *MissionData.Objective,
        MissionData.Event.Num());
}

void UCampaignSceneDlg::BuildSortedEventQueue()
{
    SortedEvents = ActiveMissionData.Event;

    SortedEvents.Sort([](const FS_MissionEvent& A, const FS_MissionEvent& B)
        {
            return A.EventTime < B.EventTime;
        });

    DisplayBlocks.Empty();
    SortedDisplayTimes.Empty();

    for (const FS_MissionEvent& Event : SortedEvents)
    {
        if (Event.EventType != MISSIONEVENT_TYPE::DISPLAY)
        {
            continue;
        }

        const FString Line = FixEscapedText(Event.EventMessage);
        if (Line.IsEmpty())
        {
            continue;
        }

        DisplayBlocks.FindOrAdd(Event.EventTime).Add(Line);
    }

    DisplayBlocks.GetKeys(SortedDisplayTimes);
    SortedDisplayTimes.Sort();

    for (const double BlockTime : SortedDisplayTimes)
    {
        const TArray<FString>* Lines = DisplayBlocks.Find(BlockTime);

        FString Joined;
        if (Lines)
        {
            for (int32 i = 0; i < Lines->Num(); ++i)
            {
                if (i > 0)
                {
                    Joined += TEXT(" | ");
                }
                Joined += (*Lines)[i];
            }
        }

        UE_LOG(LogTemp, Log,
            TEXT("[SceneDlg] Display block time=%.2f lines=%d [%s]"),
            BlockTime,
            Lines ? Lines->Num() : 0,
            *Joined);
    }
}

FString UCampaignSceneDlg::BuildBodyTextFromDisplayBlock(const TArray<FString>& Lines) const
{
    FString Out;

    if (Lines.Num() <= 1)
    {
        return Out;
    }

    for (int32 i = 1; i < Lines.Num(); ++i)
    {
        if (!Out.IsEmpty())
        {
            Out += TEXT("\n");
        }

        Out += Lines[i];
    }

    return Out;
}

float UCampaignSceneDlg::ResolveSceneDurationSeconds() const
{
    double LatestTime = 0.0;

    for (const FS_MissionEvent& Event : SortedEvents)
    {
        LatestTime = FMath::Max(LatestTime, Event.EventTime);
    }

    const float FinalHoldSeconds = 4.0f;
    return static_cast<float>(LatestTime) + FinalHoldSeconds;
}

void UCampaignSceneDlg::BeginSceneByName(const FString& InSceneName, float InDurationSeconds)
{
    ActiveSceneName = InSceneName;
    SceneStartRealSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());

    if (InDurationSeconds > 0.0f)
    {
        SceneDurationSeconds = InDurationSeconds;
    }
    else
    {
        SceneDurationSeconds = ResolveSceneDurationSeconds();
    }

    bSceneRunning = true;
    NextDisplayBlockIndex = 0;
    NextMessageEventIndex = 0;

    if (MessageTitleText)
    {
        MessageTitleText->SetText(FText::GetEmpty());
    }

    if (MessageSubtitleText)
    {
        MessageSubtitleText->SetText(FText::GetEmpty());
    }

    if (CaptionTextBottom)
    {
        CaptionTextBottom->SetText(FText::GetEmpty());
    }

    Show();

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] BeginSceneByName: Scene=%s Duration=%.2f Start=%.2f"),
        *ActiveSceneName,
        SceneDurationSeconds,
        SceneStartRealSeconds);
}

void UCampaignSceneDlg::ExecuteDisplayBlockAtTime(double BlockTime)
{
    const TArray<FString>* Lines = DisplayBlocks.Find(BlockTime);
    if (!Lines || Lines->Num() == 0)
    {
        return;
    }

    const FString TitleLine = FixEscapedText((*Lines)[0]);
    const FString SubtitleLines = FixEscapedText(BuildBodyTextFromDisplayBlock(*Lines));

    if (MessageTitleText)
    {
        MessageTitleText->SetText(FText::FromString(TitleLine));
    }

    if (MessageSubtitleText)
    {
        MessageSubtitleText->SetText(FText::FromString(SubtitleLines));
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] DISPLAY BLOCK @ %.2f Title='%s' Subtitle='%s'"),
        BlockTime,
        *TitleLine,
        *SubtitleLines);
}

void UCampaignSceneDlg::ExecuteMessageEvent(const FS_MissionEvent& Event)
{
    const FString Caption = FixEscapedText(Event.EventCaption);

    if (!Caption.IsEmpty() && CaptionTextBottom)
    {
        CaptionTextBottom->SetText(FText::FromString(Caption));

        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] CAPTION @ %.2f '%s'"),
            Event.EventTime,
            *Caption);
    }

    if (!Event.EventSound.IsEmpty())
    {
        if (USoundBase* SceneSound = ResolveSceneSound(Event.EventSound))
        {
            UGameplayStatics::PlaySound2D(this, SceneSound);

            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] MESSAGE SOUND @ %.2f '%s'"),
                Event.EventTime,
                *Event.EventSound);
        }
    }
}

void UCampaignSceneDlg::ProcessPendingEvents(float ElapsedSeconds)
{
    while (NextDisplayBlockIndex < SortedDisplayTimes.Num())
    {
        const double BlockTime = SortedDisplayTimes[NextDisplayBlockIndex];

        if (ElapsedSeconds + KINDA_SMALL_NUMBER < BlockTime)
        {
            break;
        }

        ExecuteDisplayBlockAtTime(BlockTime);
        ++NextDisplayBlockIndex;
    }

    while (NextMessageEventIndex < SortedEvents.Num())
    {
        const FS_MissionEvent& Event = SortedEvents[NextMessageEventIndex];

        if (ElapsedSeconds + KINDA_SMALL_NUMBER < Event.EventTime)
        {
            break;
        }

        if (Event.EventType == MISSIONEVENT_TYPE::MESSAGE)
        {
            ExecuteMessageEvent(Event);
        }

        ++NextMessageEventIndex;
    }
}

void UCampaignSceneDlg::AdvanceSceneFromTimer(float NowSeconds)
{
    if (!bSceneRunning)
    {
        return;
    }

    const float Elapsed = NowSeconds - SceneStartRealSeconds;

    ProcessPendingEvents(Elapsed);

    if (Elapsed >= SceneDurationSeconds)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] COMPLETE -> Returning to CmdDlg"));

        bSceneRunning = false;
        Hide();

        if (Manager)
        {
            Manager->HideCmpSceneDlg();
            Manager->ShowCmdDlg();
        }
    }
}