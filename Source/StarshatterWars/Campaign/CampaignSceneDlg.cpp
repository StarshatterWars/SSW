#include "CampaignSceneDlg.h"

#include "CmpnScreen.h"
#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

#include "SystemSceneBuilder.h"
#include "EngineUtils.h"
#include "Engine/World.h"

#include "Engine/Font.h"
#include "Engine/Texture2D.h"
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
    SetIsFocusable(true);
    SetKeyboardFocus();

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
    UpdatePanelFade(NowSeconds);
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

    ScenePanelImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ScenePanelImage"));
    if (ScenePanelImage)
    {
        ScenePanelImage->SetOpacity(0.0f);

        UOverlaySlot* ImageSlot = RuntimeOverlay->AddChildToOverlay(ScenePanelImage);
        if (ImageSlot)
        {
            ImageSlot->SetHorizontalAlignment(HAlign_Fill);
            ImageSlot->SetVerticalAlignment(VAlign_Fill);
            ImageSlot->SetPadding(FMargin(0.0f));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: failed to create ScenePanelImage"));
    }

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
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: failed to create HeaderText"));
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
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: failed to create MessageTitleText"));
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
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: failed to create MessageSubtitleText"));
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
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] BuildRuntimeWidgets: failed to create CaptionTextBottom"));
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

USoundBase* UCampaignSceneDlg::ResolveSceneSound(const FString& SoundToken) const
{
    const FString SoundPath = ResolveSceneSoundPath(SoundToken);
    if (SoundPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneDlg] ResolveSceneSound: no path resolved for token '%s'"),
            *SoundToken);
        return nullptr;
    }

    USoundBase* Sound = LoadObject<USoundBase>(nullptr, *SoundPath);
    if (!Sound)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneDlg] ResolveSceneSound: failed to load '%s'"),
            *SoundPath);
    }

    return Sound;
}

int32 UCampaignSceneDlg::ResolveCampaignNumber() const
{
    return CurrentCampaignNumber > 0 ? CurrentCampaignNumber : 1;
}

FString UCampaignSceneDlg::ResolveSceneSoundPath(const FString& SoundToken) const
{
    FString Token = SoundToken.TrimStartAndEnd();
    if (Token.IsEmpty())
    {
        return FString();
    }

    // Full quoted Unreal object reference:
    // /Script/Engine.SoundWave'/Game/Audio/Vox/Scenes/02/Briefing_06.Briefing_06'
    const int32 FirstQuote = Token.Find(TEXT("'"));
    const int32 LastQuote = Token.Find(TEXT("'"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);

    if (FirstQuote != INDEX_NONE && LastQuote != INDEX_NONE && LastQuote > FirstQuote)
    {
        const FString InnerPath = Token.Mid(FirstQuote + 1, LastQuote - FirstQuote - 1).TrimStartAndEnd();
        if (InnerPath.StartsWith(TEXT("/Game/")))
        {
            return InnerPath;
        }
    }

    // Already a direct object path:
    if (Token.StartsWith(TEXT("/Game/")))
    {
        return Token;
    }

    // Bare token fallback:
    const int32 CampaignNum = ResolveCampaignNumber();

    TArray<FString> CandidatePaths;
    CandidatePaths.Add(FString::Printf(
        TEXT("/Game/Audio/Vox/Scenes/%02d/%s.%s"),
        CampaignNum,
        *Token,
        *Token));

    CandidatePaths.Add(FString::Printf(
        TEXT("/Game/Audio/Campaigns/%02d/%s.%s"),
        CampaignNum,
        *Token,
        *Token));

    CandidatePaths.Add(FString::Printf(
        TEXT("/Game/Audio/%s.%s"),
        *Token,
        *Token));

    CandidatePaths.Add(FString::Printf(
        TEXT("/Game/Sounds/%s.%s"),
        *Token,
        *Token));

    for (const FString& Path : CandidatePaths)
    {
        if (LoadObject<USoundBase>(nullptr, *Path))
        {
            return Path;
        }
    }

    return FString();
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

    UTexture2D* TestTexture =
        ResolveScenePanelTexture(TEXT("/Script/Engine.Texture2D'/Game/UI/Campaigns/02/News.News'"));
    ApplyPanelTexture(TestTexture);

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

void UCampaignSceneDlg::ExecuteDisplayEvent(const FS_MissionEvent& Event)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ExecuteDisplayEvent: Time=%.2f Image='%s' Target='%s' Message='%s' Fade=(%.2f, %.2f, %.2f)"),
        Event.EventTime,
        *Event.EventImage,
        *Event.EventTarget,
        *Event.EventMessage,
        Event.EventFade.X,
        Event.EventFade.Y,
        Event.EventFade.Z);

    // DISPLAY panel token can arrive in EventImage or EventTarget.
    FString PanelToken = Event.EventImage.TrimStartAndEnd();
    if (PanelToken.IsEmpty())
    {
        PanelToken = Event.EventTarget.TrimStartAndEnd();
    }

    if (!PanelToken.IsEmpty())
    {
        UTexture2D* PanelTexture = ResolveScenePanelTexture(PanelToken);
        ApplyPanelTexture(PanelTexture);

        PanelStartTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
        PanelFadeInTime = FMath::Max(0.0f, Event.EventFade.X);
        PanelHoldTime = FMath::Max(0.0f, Event.EventFade.Y);
        PanelFadeOutTime = FMath::Max(0.0f, Event.EventFade.Z);
        bPanelActive = (PanelTexture != nullptr);

        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] Panel state started from token '%s': FadeIn=%.2f Hold=%.2f FadeOut=%.2f"),
            *PanelToken,
            PanelFadeInTime,
            PanelHoldTime,
            PanelFadeOutTime);
    }

    const FString Message = FixEscapedText(Event.EventMessage);
    if (!Message.IsEmpty())
    {
        if (MessageTitleText)
        {
            MessageTitleText->SetText(FText::FromString(Message));
        }
    }
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
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] MESSAGE SOUND @ %.2f '%s'"),
            Event.EventTime,
            *Event.EventSound);

        USoundBase* Sound = ResolveSceneSound(Event.EventSound);
        if (Sound)
        {
            UGameplayStatics::PlaySound2D(this, Sound);

            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] PLAYED SOUND '%s'"),
                *GetNameSafe(Sound));
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("[SceneDlg] FAILED TO RESOLVE SOUND '%s'"),
                *Event.EventSound);
        }
    }
}

void UCampaignSceneDlg::ProcessPendingEvents(float ElapsedSeconds)
{
    while (NextMessageEventIndex < SortedEvents.Num())
    {
        const FS_MissionEvent& Event = SortedEvents[NextMessageEventIndex];

        if (ElapsedSeconds + KINDA_SMALL_NUMBER < Event.EventTime)
        {
            break;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ProcessPendingEvents: firing EventIndex=%d Type=%d Time=%.2f Target='%s' Message='%s' Caption='%s'"),
            NextMessageEventIndex,
            (int32)Event.EventType,
            Event.EventTime,
            *Event.EventTarget,
            *Event.EventMessage,
            *Event.EventCaption);

        if (Event.EventType == MISSIONEVENT_TYPE::DISPLAY)
        {
            ExecuteDisplayEvent(Event);
        }
        else if (Event.EventType == MISSIONEVENT_TYPE::MESSAGE)
        {
            ExecuteMessageEvent(Event);
        }
        else if (Event.EventType == MISSIONEVENT_TYPE::CAMERA)
        {
            ExecuteCameraEvent(Event);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] ProcessPendingEvents: unhandled event type %d"),
                (int32)Event.EventType);
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
        FinishCutscene();
    }
}

FString UCampaignSceneDlg::ResolveScenePanelPath(const FString& ImageToken) const
{
    FString Token = ImageToken.TrimStartAndEnd();
    if (Token.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] ResolveScenePanelPath: empty token"));
        return FString();
    }

    const int32 FirstQuote = Token.Find(TEXT("'"));
    const int32 LastQuote = Token.Find(TEXT("'"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);

    if (FirstQuote != INDEX_NONE && LastQuote != INDEX_NONE && LastQuote > FirstQuote)
    {
        const FString InnerPath = Token.Mid(FirstQuote + 1, LastQuote - FirstQuote - 1).TrimStartAndEnd();
        if (InnerPath.StartsWith(TEXT("/Game/")))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] ResolveScenePanelPath: quoted -> %s"),
                *InnerPath);
            return InnerPath;
        }
    }

    if (Token.StartsWith(TEXT("/Game/")))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ResolveScenePanelPath: direct -> %s"),
            *Token);
        return Token;
    }

    const FString CampaignPath = FString::Printf(TEXT("/Game/UI/Campaigns/02/%s.%s"), *Token, *Token);
    if (LoadObject<UTexture2D>(nullptr, *CampaignPath))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ResolveScenePanelPath: token '%s' -> '%s'"),
            *Token, *CampaignPath);
        return CampaignPath;
    }

    UE_LOG(LogTemp, Error,
        TEXT("[SceneDlg] ResolveScenePanelPath: failed for '%s'"),
        *Token);

    return FString();
}

UTexture2D* UCampaignSceneDlg::ResolveScenePanelTexture(const FString& ImageToken) const
{
    const FString TexturePath = ResolveScenePanelPath(ImageToken);
    if (TexturePath.IsEmpty())
    {
        return nullptr;
    }

    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
    if (!Texture)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneDlg] ResolveScenePanelTexture: failed to load '%s'"),
            *TexturePath);
        return nullptr;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[SceneDlg] ResolveScenePanelTexture: loaded '%s'"),
        *TexturePath);

    return Texture;
}

void UCampaignSceneDlg::ApplyPanelTexture(UTexture2D* Texture)
{
    if (!ScenePanelImage)
    {
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] ApplyPanelTexture: ScenePanelImage is null"));
        return;
    }

    if (!Texture)
    {
        ScenePanelImage->SetBrush(FSlateBrush());
        ScenePanelImage->SetOpacity(0.0f);
        UE_LOG(LogTemp, Error, TEXT("[SceneDlg] ApplyPanelTexture: Texture is null"));
        return;
    }

    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = FVector2D(1920.0f, 1080.0f);

    ScenePanelImage->SetBrush(Brush);
    ScenePanelImage->SetOpacity(1.0f);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ApplyPanelTexture: applied %s"),
        *GetNameSafe(Texture));
}

void UCampaignSceneDlg::ClearPanelTexture()
{
    if (!ScenePanelImage)
    {
        return;
    }

    ScenePanelImage->SetBrush(FSlateBrush());
    ScenePanelImage->SetOpacity(0.0f);

    UE_LOG(LogTemp, Log, TEXT("[SceneDlg] ClearPanelTexture"));
}

void UCampaignSceneDlg::UpdatePanelFade(float NowSeconds)
{
    if (!bPanelActive || !ScenePanelImage)
    {
        return;
    }

    const float Elapsed = NowSeconds - PanelStartTime;

    const float FadeInEnd = PanelFadeInTime;
    const float HoldEnd = FadeInEnd + PanelHoldTime;
    const float FadeOutEnd = HoldEnd + PanelFadeOutTime;

    float NewOpacity = 0.0f;

    if (Elapsed <= FadeInEnd)
    {
        if (PanelFadeInTime <= KINDA_SMALL_NUMBER)
        {
            NewOpacity = 1.0f;
        }
        else
        {
            NewOpacity = FMath::Clamp(Elapsed / PanelFadeInTime, 0.0f, 1.0f);
        }
    }
    else if (Elapsed <= HoldEnd)
    {
        NewOpacity = 1.0f;
    }
    else if (Elapsed <= FadeOutEnd)
    {
        if (PanelFadeOutTime <= KINDA_SMALL_NUMBER)
        {
            NewOpacity = 0.0f;
        }
        else
        {
            const float T = (Elapsed - HoldEnd) / PanelFadeOutTime;
            NewOpacity = 1.0f - FMath::Clamp(T, 0.0f, 1.0f);
        }
    }
    else
    {
        NewOpacity = 0.0f;
        bPanelActive = false;
        ClearPanelTexture();
    }

    ScenePanelImage->SetOpacity(NewOpacity);
}

ASystemSceneBuilder* UCampaignSceneDlg::ResolveSystemSceneBuilder() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ResolveSystemSceneBuilder: World is null"));
        return nullptr;
    }

    for (TActorIterator<ASystemSceneBuilder> It(World); It; ++It)
    {
        ASystemSceneBuilder* Builder = *It;
        if (Builder)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] ResolveSystemSceneBuilder: found builder '%s'"),
                *Builder->GetName());

            return Builder;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ResolveSystemSceneBuilder: no builder found"));

    return nullptr;
}

void UCampaignSceneDlg::ExecuteCameraEvent(const FS_MissionEvent& Event)
{
    FString EventParamText = TEXT("[");
    const int32 ParamCount = FMath::Clamp(Event.EventNParams, 0, Event.EventParam.Num());

    for (int32 Index = 0; Index < ParamCount; ++Index)
    {
        if (Index > 0)
        {
            EventParamText += TEXT(", ");
        }

        EventParamText += FString::FromInt(Event.EventParam[Index]);
    }

    EventParamText += TEXT("]");

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ExecuteCameraEvent: Time=%.2f Target='%s' Point=%s EventParam=%s EventNParams=%d"),
        Event.EventTime,
        *Event.EventTarget,
        *Event.EventPoint.ToString(),
        *EventParamText,
        Event.EventNParams);

    ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();
    if (!Builder)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneDlg] ExecuteCameraEvent: no SystemSceneBuilder found"));
        return;
    }

    if (!Event.EventTarget.IsEmpty())
    {
        FString RegionName;
        if (ResolveElementRegionForTarget(Event.EventTarget, RegionName))
        {
            const bool bFocusedRegion = Builder->FocusCameraOnBodyByName(
                RegionName,
                Event.EventPoint,
                0.0f);

            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] ExecuteCameraEvent: Region focus Target='%s' Region='%s' result=%s"),
                *Event.EventTarget,
                *RegionName,
                bFocusedRegion ? TEXT("true") : TEXT("false"));

            if (bFocusedRegion)
            {
                return;
            }
        }

        const bool bFocused = Builder->FocusCameraOnBodyByName(
            Event.EventTarget,
            Event.EventPoint,
            0.0f);

        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ExecuteCameraEvent: Direct target focus '%s' result=%s"),
            *Event.EventTarget,
            bFocused ? TEXT("true") : TEXT("false"));

        return;
    }

    if (!Event.EventPoint.IsNearlyZero())
    {
        const bool bApplied = Builder->ApplyCameraViewVector(
            Event.EventPoint,
            0.0f);

        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ExecuteCameraEvent: Apply view vector result=%s"),
            bApplied ? TEXT("true") : TEXT("false"));

        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ExecuteCameraEvent: no target and no usable point"));
}

void UCampaignSceneDlg::DebugCameraEventTarget(const FS_MissionEvent& Event)
{
    FString EventParamText = TEXT("[");
    const int32 ParamCount = FMath::Clamp(Event.EventNParams, 0, Event.EventParam.Num());

    for (int32 Index = 0; Index < ParamCount; ++Index)
    {
        if (Index > 0)
        {
            EventParamText += TEXT(", ");
        }

        EventParamText += FString::FromInt(Event.EventParam[Index]);
    }

    EventParamText += TEXT("]");

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] DebugCameraEventTarget: Time=%.2f Target='%s' Point=%s EventParam=%s EventNParams=%d"),
        Event.EventTime,
        *Event.EventTarget,
        *Event.EventPoint.ToString(),
        *EventParamText,
        Event.EventNParams);

    ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();
    if (!Builder)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneDlg] DebugCameraEventTarget: no SystemSceneBuilder found"));
        return;
    }

    if (Event.EventTarget.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] DebugCameraEventTarget: no target"));
        return;
    }

    const bool bFocused = Builder->DebugFocusCameraOnBodyByName(
        Event.EventTarget,
        Event.EventPoint);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] DebugCameraEventTarget: focus result for '%s' = %s"),
        *Event.EventTarget,
        bFocused ? TEXT("true") : TEXT("false"));
}

FReply UCampaignSceneDlg::NativeOnKeyDown(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::SpaceBar)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] SPACE pressed ? SkipCutscene"));
        SkipCutscene();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCampaignSceneDlg::SkipCutscene()
{
    UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] SkipCutscene triggered"));
    FinishCutscene();
}

void UCampaignSceneDlg::FinishCutscene()
{
    UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] Cutscene finished"));
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
bool UCampaignSceneDlg::ResolveElementRegionForTarget(const FString& TargetName, FString& OutRegionName) const
{
    OutRegionName.Empty();

    const FString SearchName = TargetName.TrimStartAndEnd();
    if (SearchName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ResolveElementRegionForTarget: empty target"));
        return false;
    }

    const FS_MissionElement* FirstDesignMatch = nullptr;

    for (const FS_MissionElement& Elem : ActiveMissionData.Element)
    {
        const FString ElemName = Elem.Name.TrimStartAndEnd();
        const FString ElemDesign = Elem.Design.TrimStartAndEnd();
        const FString ElemRegion = Elem.RegionName.TrimStartAndEnd();

        if (ElemRegion.IsEmpty())
        {
            continue;
        }

        if (ElemName.Equals(SearchName, ESearchCase::IgnoreCase))
        {
            OutRegionName = ElemRegion;

            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] ResolveElementRegionForTarget: exact name match Target='%s' -> Region='%s'"),
                *SearchName,
                *OutRegionName);

            return true;
        }

        if (!FirstDesignMatch &&
            !ElemDesign.IsEmpty() &&
            ElemDesign.Equals(SearchName, ESearchCase::IgnoreCase))
        {
            FirstDesignMatch = &Elem;
        }
    }

    if (FirstDesignMatch)
    {
        OutRegionName = FirstDesignMatch->RegionName.TrimStartAndEnd();

        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ResolveElementRegionForTarget: design match Target='%s' -> Region='%s'"),
            *SearchName,
            *OutRegionName);

        return !OutRegionName.IsEmpty();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ResolveElementRegionForTarget: no region found for Target='%s'"),
        *SearchName);

    return false;
}