#include "CampaignSceneDlg.h"

#include "CmpnScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

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

    const FLinearColor HalfAlphaWhite(1.f, 1.f, 1.f, 0.5f);
    UFont* BoldLimerick = GetBoldLimerickFont();

    // Use near full-screen width while still allowing wrap:
    const float WrapWidth = 1800.0f;
    const FMargin FullWidthMargin(32.0f, 0.0f, 32.0f, 0.0f);

    // ------------------------------------------------------------
    // HEADER TEXT (mission objective)
    // ------------------------------------------------------------
    HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeaderText"));
    if (HeaderText)
    {
        HeaderText->SetText(FText::GetEmpty());
        HeaderText->SetColorAndOpacity(FSlateColor(HalfAlphaWhite));
        HeaderText->SetAutoWrapText(true);
        HeaderText->SetWrapTextAt(WrapWidth);
        HeaderText->SetJustification(ETextJustify::Left);
        HeaderText->SetMinDesiredWidth(WrapWidth);

        if (BoldLimerick)
        {
            FSlateFontInfo FontInfo;
            FontInfo.FontObject = BoldLimerick;
            FontInfo.Size = 18;
            HeaderText->SetFont(FontInfo);
        }

        UOverlaySlot* HeaderSlot = RuntimeOverlay->AddChildToOverlay(HeaderText);
        if (HeaderSlot)
        {
            HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
            HeaderSlot->SetVerticalAlignment(VAlign_Top);
            HeaderSlot->SetPadding(FMargin(32.0f, 24.0f, 32.0f, 0.0f));
        }
    }

    // ------------------------------------------------------------
    // MESSAGE TITLE
    // ------------------------------------------------------------
    MessageTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageTitleText"));
    if (MessageTitleText)
    {
        MessageTitleText->SetText(FText::GetEmpty());
        MessageTitleText->SetColorAndOpacity(FSlateColor(HalfAlphaWhite));
        MessageTitleText->SetAutoWrapText(true);
        MessageTitleText->SetWrapTextAt(WrapWidth);
        MessageTitleText->SetJustification(ETextJustify::Left);
        MessageTitleText->SetMinDesiredWidth(WrapWidth);

        if (BoldLimerick)
        {
            FSlateFontInfo FontInfo;
            FontInfo.FontObject = BoldLimerick;
            FontInfo.Size = 18;
            MessageTitleText->SetFont(FontInfo);
        }

        UOverlaySlot* TitleSlot = RuntimeOverlay->AddChildToOverlay(MessageTitleText);
        if (TitleSlot)
        {
            TitleSlot->SetHorizontalAlignment(HAlign_Fill);
            TitleSlot->SetVerticalAlignment(VAlign_Top);
            TitleSlot->SetPadding(FMargin(32.0f, 88.0f, 32.0f, 0.0f));
        }
    }

    // ------------------------------------------------------------
    // MESSAGE SUBTITLE
    // ------------------------------------------------------------
    MessageSubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageSubtitleText"));
    if (MessageSubtitleText)
    {
        MessageSubtitleText->SetText(FText::GetEmpty());
        MessageSubtitleText->SetColorAndOpacity(FSlateColor(HalfAlphaWhite));
        MessageSubtitleText->SetAutoWrapText(true);
        MessageSubtitleText->SetWrapTextAt(WrapWidth);
        MessageSubtitleText->SetJustification(ETextJustify::Left);
        MessageSubtitleText->SetMinDesiredWidth(WrapWidth);

        if (BoldLimerick)
        {
            FSlateFontInfo FontInfo;
            FontInfo.FontObject = BoldLimerick;
            FontInfo.Size = 18;
            MessageSubtitleText->SetFont(FontInfo);
        }

        UOverlaySlot* SubtitleSlot = RuntimeOverlay->AddChildToOverlay(MessageSubtitleText);
        if (SubtitleSlot)
        {
            SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
            SubtitleSlot->SetVerticalAlignment(VAlign_Top);
            SubtitleSlot->SetPadding(FMargin(32.0f, 122.0f, 32.0f, 0.0f));
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

        HeaderText->SetText(FText::FromString(Header));
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

    for (const FS_MissionEvent& Event : SortedEvents)
    {
        if (Event.EventType != MISSIONEVENT_TYPE::DISPLAY)
        {
            continue;
        }

        const FString Line = Event.EventMessage.TrimStartAndEnd();
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

    if (LatestTime <= 0.0)
    {
        return 10.0f;
    }

    return static_cast<float>(LatestTime);
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

    if (MessageTitleText)
    {
        MessageTitleText->SetText(FText::GetEmpty());
    }

    if (MessageSubtitleText)
    {
        MessageSubtitleText->SetText(FText::GetEmpty());
    }

    Show();

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] BeginSceneByName: Scene=%s Duration=%.2f Start=%.2f"),
        *ActiveSceneName,
        SceneDurationSeconds,
        SceneStartRealSeconds);
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

        ExecuteEventBlockAtTime(BlockTime);
        ++NextDisplayBlockIndex;
    }
}

void UCampaignSceneDlg::ExecuteEventBlockAtTime(double BlockTime)
{
    const TArray<FString>* Lines = DisplayBlocks.Find(BlockTime);
    if (!Lines || Lines->Num() == 0)
    {
        return;
    }

    const FString TitleLine = (*Lines)[0];
    const FString SubtitleLines = BuildBodyTextFromDisplayBlock(*Lines);

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