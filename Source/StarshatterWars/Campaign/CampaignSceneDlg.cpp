#include "CampaignSceneDlg.h"
#include "Engine/Engine.h"
#include "Components/AudioComponent.h"

#include "CmpnScreen.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

#include "SystemSceneBuilder.h"
#include "SystemLevelScriptActor.h"
#include "CampaignSceneActor.h"
#include "SSWCameraManager.h"
#include "SSWLegacyCameraViewActor.h"
#include "SSWRuntimeSubsystem.h"
#include "CameraManager.h"  // Legacy CameraDirector port; owns cutscene camera behavior
#include "Sim.h"
#include "Orbital.h"
#include "ShipActor.h"
#include "Ship.h"
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

    UE_LOG(LogTemp, Error, TEXT("[SSW BUILD PROBE 20261018] CUTSCENE WIDGET CONSTRUCTED Widget=%s"), *GetNameSafe(this));


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

    static int32 CutsceneTraceTickCount = 0;
    if (++CutsceneTraceTickCount % 120 == 0)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CUTSCENE TRACE] Tick Widget=%s Running=%d Events=%d Next=%d"),
            *GetNameSafe(this), bSceneRunning ? 1 : 0,
            SortedEvents.Num(), NextMessageEventIndex);
    }

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
    StopCutsceneAudio();
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
    bCutsceneTransitionStarted = false;

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
    EnableCutsceneExitInput();
    SetKeyboardFocus();
    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);

    UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] SHOW"));
}

void UCampaignSceneDlg::Hide()
{
    StopCutsceneAudio();
    DisableCutsceneExitInput();
    SetVisibility(ESlateVisibility::Collapsed);
    SetIsEnabled(false);

    UE_LOG(LogTemp, Warning, TEXT("[SceneDlg] HIDE"));
}

void UCampaignSceneDlg::LoadSceneFromMissionData(const FS_CampaignMission& MissionData)
{
    UE_LOG(LogTemp, Error, TEXT("[SSW BUILD PROBE 20261018] CUTSCENE LOAD Scene=%s Events=%d"),
        *MissionData.Scene, MissionData.Event.Num());

    UE_LOG(LogTemp, Error,
        TEXT("[CUTSCENE TRACE] LoadSceneFromMissionData Widget=%s Scene='%s' Events=%d"),
        *GetNameSafe(this), *MissionData.Scene, MissionData.Event.Num());
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

    // Planet preflight before any cutscene camera event. Keep ships separate.
    if (UWorld* World = GetWorld())
    {
        ASystemLevelScriptActor* SystemLevel = nullptr;
        for (TActorIterator<ASystemLevelScriptActor> It(World); It; ++It)
        {
            if (IsValid(*It)) { SystemLevel = *It; break; }
        }
        if (!SystemLevel)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[PlanetPreflight] Persistent SystemLevelScriptActor not found; planets not initialized"));
        }
        else if (!SystemLevel->EnsureCurrentSystemBuilt())
        {
            UE_LOG(LogTemp, Error,
                TEXT("[PlanetPreflight] Planet system was not built; inspect runtime registry and satellites"));
        }
    }

    // Build through the same authoritative runtime presentation path as gameplay:
    // Mission -> Sim -> Ship -> USSWRuntimeSubsystem -> AShipActor.
    if (UWorld* World = GetWorld())
    {
        ACampaignSceneActor* SceneActor = nullptr;

        for (TActorIterator<ACampaignSceneActor> It(World); It; ++It)
        {
            if (IsValid(*It))
            {
                SceneActor = *It;
                break;
            }
        }

        if (SceneActor)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] Initializing cutscene through runtime mission presentation"));

            SceneActor->BuildSceneActorsFromMission(MissionData);
            SceneActor->DumpSceneActors();
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("[SceneDlg] No persistent CampaignSceneActor found for runtime cutscene presentation"));
        }
    }

    // The owning campaign screen starts the timeline after mission data loads.
    // Do not start here: that creates two BeginSceneByName calls, resetting events.
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

    if (Token.StartsWith(TEXT("/Game/")))
    {
        return Token;
    }

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

bool UCampaignSceneDlg::PrepareSceneCameraBeforeReveal()
{
    if (bSceneRunning || SortedEvents.IsEmpty())
        return false;

    // Legacy-first: the first DataTable camera event chooses the presentation
    // endpoint. Do not spawn ASSWCameraManager merely for preflight.
    // Never start the event clock here.

    for (const FS_MissionEvent& Event : SortedEvents)
    {
        if (Event.EventTime > 0.1)
            break;
        if (Event.EventType == MISSIONEVENT_TYPE::CAMERA &&
            Event.EventParam.Num() > 0 && Event.EventParam[0] == 3)
        {
            ExecuteCameraEvent(Event);
            return true;
        }
    }
    UE_LOG(LogTemp, Error, TEXT("[ScenePreload] Initial planetary camera event missing"));
    return false;
}

void UCampaignSceneDlg::BeginSceneByName(const FString& InSceneName, float InDurationSeconds)
{
    UE_LOG(LogTemp, Error,
        TEXT("[CUTSCENE TRACE] BeginSceneByName Widget=%s Scene='%s' Events=%d"),
        *GetNameSafe(this), *InSceneName, SortedEvents.Num());
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

    // Phase 5: do not activate or spawn the compatibility camera here.
    // The first DataTable camera event chooses either the passive legacy
    // presentation actor or the existing scene-only compatibility camera.

    // Establish the first planetary shot immediately. Merely activating the
    // camera leaves it at its level-authored position (often inside the sun).
    // The same event still runs normally at its legacy timestamp.
    for (const FS_MissionEvent& CameraEvent : SortedEvents)
    {
        if (CameraEvent.EventTime > 0.1)
            break;
        if (CameraEvent.EventType == MISSIONEVENT_TYPE::CAMERA &&
            CameraEvent.EventParam.Num() > 0 &&
            CameraEvent.EventParam[0] == 3)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[PlanetCamera] Initial shot target='%s' legacy=%s"),
                *CameraEvent.EventTarget, *CameraEvent.EventPoint.ToString());
            ExecuteCameraEvent(CameraEvent);
            break;
        }
    }

    // Do NOT re-activate ASSWCameraManager here: doing so steals the view
    // target from ASSWLegacyCameraViewActor immediately after initial setup.
    // The runtime's one ExecFrame() + SyncLegacyCameraView() drives the view.

    // Populate time-zero image, title, and display events BEFORE the owning
    // campaign screen removes its loading cover. The first NativeTick is too
    // late and previously exposed a blank frame.
    ProcessPendingEvents(0.0f);
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

        // The opening DISPLAY image is the first visible cutscene frame.
        // Its authored fade-in previously started at opacity zero, exposing
        // a black screen directly after dismissing the loading overlay.
        // Start this one opening image fully visible; leave all later image
        // fades and every narration/camera event time unchanged.
        if (bPanelActive && Event.EventTime <= KINDA_SMALL_NUMBER)
        {
            PanelStartTime -= PanelFadeInTime;
            if (ScenePanelImage) ScenePanelImage->SetOpacity(1.0f);
        }

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
            CutsceneAudioComponents.RemoveAll([](const TWeakObjectPtr<UAudioComponent>& Entry)
            { return !Entry.IsValid(); });
            // Auto-destroy after natural completion; retain a weak handle for scene cleanup.
            if (UAudioComponent* Audio = UGameplayStatics::SpawnSound2D(
                this, Sound, 1.0f, 1.0f, 0.0f, nullptr, false, true))
            {
                CutsceneAudioComponents.Add(Audio);
            }

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

// Phase 7: resolve the player from the imported mission element, not its design.
// Example: Name=Falcon, Design=CameraPod, Player=1.  No ship is spawned here.
static Ship* SSWResolveAuthoredPlayerShip(
    const FS_CampaignMission& MissionData, Sim* LiveSim, UWorld* World)
{
    if (!LiveSim)
        return nullptr;

    const FS_MissionElement* PlayerElement = nullptr;
    int32 PlayerCount = 0;
    for (const FS_MissionElement& Element : MissionData.Element)
    {
        if (Element.Player <= 0)
            continue;
        ++PlayerCount;
        if (!PlayerElement)
            PlayerElement = &Element;
    }

    if (PlayerCount != 1 || !PlayerElement)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[LegacyCamera][PLAYER_BIND] Expected one Player>0 element, found %d. Scene='%s'"),
            PlayerCount, *MissionData.Scene);
        return nullptr;
    }

    const FString AuthoredName = PlayerElement->Name.TrimStartAndEnd();
    if (AuthoredName.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[LegacyCamera][PLAYER_BIND] Authored player has an empty Name; design='%s'"),
            *PlayerElement->Design);
        return nullptr;
    }

    Ship* Result = nullptr;
    const auto NameAnsi = StringCast<ANSICHAR>(*AuthoredName);
    Result = LiveSim->FindShip(NameAnsi.Get());

    // Sim's name lookup may not index a mission element alias.  Match the
    // bound legacy instance through the existing UE *presentation* actor.
    if (!Result && World)
    {
        for (TActorIterator<AShipActor> It(World); It; ++It)
        {
            AShipActor* Actor = *It;
            Ship* BoundShip = IsValid(Actor) ? Actor->GetRuntimeShip() : nullptr;
            if (BoundShip &&
                AuthoredName.Equals(ANSI_TO_TCHAR(BoundShip->GetName()),
                    ESearchCase::IgnoreCase))
            {
                Result = BoundShip;
                break;
            }
        }
    }

    if (!Result)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[LegacyCamera][PLAYER_BIND] NOT_SPAWNED Element='%s' Design='%s' Scene='%s'"),
            *AuthoredName, *PlayerElement->Design, *MissionData.Scene);
        return nullptr;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[LegacyCamera][PLAYER_BIND] RESOLVED Element='%s' Design='%s' LiveShip='%s'"),
        *AuthoredName, *PlayerElement->Design,
        ANSI_TO_TCHAR(Result->GetName()));
    return Result;
}

void UCampaignSceneDlg::ExecuteCameraEvent(const FS_MissionEvent& Event)
{
    UE_LOG(LogTemp, Error,
        TEXT("[CUTSCENE TRACE] ExecuteCameraEvent Widget=%s Target='%s' Params=%d"),
        *GetNameSafe(this), *Event.EventTarget, Event.EventParam.Num());
    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] Camera Event: Time=%.2f Target='%s' Point=%s Offset=%s Rotator=%s Params=%d"),
        (double)Event.EventTime,
        *Event.EventTarget,
        *Event.EventPoint.ToString(),
        *Event.EventOffset.ToString(),
        *Event.EventRotator.ToString(),
        Event.EventParam.Num());

    // ----------------------------------------------------
    // DataTable CAMERA -> original Starshatter CameraDirector semantics.
    // Do not create the UE compatibility camera for a legacy-resolved event.
    // This runs BEFORE the Unreal presentation compatibility path.
    // DataTable import is already authoritative; no .def parsing here.
    // ----------------------------------------------------
    const int32 LegacyCommand =
        Event.EventParam.IsValidIndex(0) ? Event.EventParam[0] : 0;

    if (LegacyCommand >= 1 && LegacyCommand <= 6)
    {
        Sim* LegacySim = Sim::GetSim();
        CameraManager* LegacyDirector = CameraManager::GetInstance();
        // The imported mission Player field takes precedence over Sim's
        // potentially unassigned player pointer during preloaded cutscenes.
        Ship* PlayerShip = SSWResolveAuthoredPlayerShip(
            ActiveMissionData, LegacySim, GetWorld());

        // Refuse to silently substitute an arbitrary combat ship if the
        // mission's specifically-authored player has not spawned yet.
        if (LegacySim && LegacyDirector && PlayerShip)
        {
            // Phase 9: the imported Cutscene DataTable for 01-News-Start incorrectly
            // names "Trellis Docks" for the Farcaster shot at 60.50s. The
            // authoritative .def names "Trellis-Solus". Correct only this
            // authored news shot; do not alias other dock shots or edit assets.
            FString TargetName = Event.EventTarget.TrimStartAndEnd();
            const bool bFarcasterNewsShot =
                LegacyCommand == 3 &&
                FMath::IsNearlyEqual(Event.EventTime, 60.50, 0.02) &&
                (TargetName.Equals(TEXT("Trellis Docks"), ESearchCase::IgnoreCase) ||
                 TargetName.Equals(TEXT("Trellis-Solus"), ESearchCase::IgnoreCase));
            if (bFarcasterNewsShot && TargetName.Equals(TEXT("Trellis Docks"), ESearchCase::IgnoreCase))
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[Phase9FarcasterTargetRepair] Time=%.2f DataTable='%s' AuthoredDEF='Trellis-Solus'"),
                    Event.EventTime, *TargetName);
                TargetName = TEXT("Trellis-Solus");
            }
            const bool bHasTarget = !TargetName.IsEmpty();
            const bool bBodyPrefix = TargetName.StartsWith(TEXT("body:"), ESearchCase::IgnoreCase);

            Ship* TargetShip = nullptr;
            Orbital* TargetBody = nullptr;
            if (bHasTarget)
            {
                if (!bBodyPrefix)
                {
                    const auto TargetUtf8 = StringCast<ANSICHAR>(*TargetName);
                    TargetShip = LegacySim->FindShip(TargetUtf8.Get());

                    // Phase 6: a live UE ship actor is presentation-only, but
                    // can provide the authoritative legacy Ship* when Sim's
                    // name index does not contain this particular alias.
                    // Never substitute the actor transform for legacy state.
                    if (!TargetShip)
                    {
                        if (UWorld* CameraWorld = GetWorld())
                        {
                            for (TActorIterator<AShipActor> It(CameraWorld); It; ++It)
                            {
                                AShipActor* Candidate = *It;
                                Ship* BoundShip = IsValid(Candidate)
                                    ? Candidate->GetRuntimeShip() : nullptr;
                                if (!BoundShip)
                                    continue;

                                const FString BoundName = ANSI_TO_TCHAR(BoundShip->GetName());
                                if (BoundName.Equals(TargetName, ESearchCase::IgnoreCase))
                                {
                                    TargetShip = BoundShip;
                                    UE_LOG(LogTemp, Log,
                                        TEXT("[LegacyCamera] Resolved '%s' through live ShipActor binding"),
                                        *TargetName);
                                    break;
                                }
                            }
                        }
                    }
                }

                if (!TargetShip)
                {
                    const FString BodyName = bBodyPrefix ? TargetName.Mid(5) : TargetName;
                    const auto BodyUtf8 = StringCast<ANSICHAR>(*BodyName);
                    TargetBody = LegacySim->FindOrbitalBody(BodyUtf8.Get());
                }
            }

            // If the simulation has no matching target, preserve the working
            // Unreal-only scene actor/body shot below, rather than losing it.
            // Command 5 sets orbit angular/range rates even if the optional
            // named target cannot be resolved. A missing target must never
            // cause those rates to be sent to the UE compatibility camera.
            const bool bResolved = (LegacyCommand == 5) || !bHasTarget ||
                TargetShip != nullptr || TargetBody != nullptr;
            if (bResolved)
            {
                // SetShip is the original CameraDirector ownership operation.
                // During a DropCam transition the director intentionally follows
                // another ship; do not interrupt that temporary ownership.
                Ship* CurrentDirectorShip = LegacyDirector->GetShip();
                if (!CurrentDirectorShip ||
                    (!CurrentDirectorShip->IsDropCam() &&
                     CurrentDirectorShip != PlayerShip))
                {
                    LegacyDirector->SetShip(PlayerShip);
                }

                if (LegacyDirector->GetShip())
                {
                    const double TransitionSeconds = (double)Event.EventRect.X;
                    switch (LegacyCommand)
                    {
                    case 1:
                        if (LegacyDirector->GetMode() != CameraManager::MODE_COCKPIT)
                            LegacyDirector->SetMode(CameraManager::MODE_COCKPIT, TransitionSeconds);
                        break;
                    case 2:
                        if (LegacyDirector->GetMode() != CameraManager::MODE_CHASE)
                            LegacyDirector->SetMode(CameraManager::MODE_CHASE, TransitionSeconds);
                        break;
                    case 3:
                        if (LegacyDirector->GetMode() != CameraManager::MODE_ORBIT)
                            LegacyDirector->SetMode(CameraManager::MODE_ORBIT, TransitionSeconds);
                        break;
                    case 4:
                        if (LegacyDirector->GetMode() != CameraManager::MODE_TARGET)
                            LegacyDirector->SetMode(CameraManager::MODE_TARGET, TransitionSeconds);
                        break;
                    default:
                        break; // 5 = orbit rates, 6 = ship-based DropCam
                    }

                    if (TargetShip)
                    {
                        LegacyDirector->SetViewOrbital(nullptr);
                        if (LegacyDirector->GetViewObject() != TargetShip)
                        {
                            if (LegacyCommand == 6)
                            {
                                const double Duration = Event.EventParam.IsValidIndex(1)
                                    ? (double)Event.EventParam[1] : 0.0;
                                const double RangeMultiplier = Event.EventParam.IsValidIndex(2)
                                    ? (double)Event.EventParam[2] : 0.0;
                                TargetShip->DropCam(Duration, RangeMultiplier);
                                LegacyDirector->SetShip(TargetShip);
                                LegacyDirector->SetMode(CameraManager::MODE_DROP, 0.0);
                            }
                            else
                            {
                                Ship* Previous = LegacyDirector->GetShip();
                                if (Previous && Previous->IsDropCam())
                                    Previous->CompleteTransition();
                                if (LegacyDirector->GetShip() != PlayerShip)
                                    LegacyDirector->SetShip(PlayerShip);
                                LegacyDirector->SetViewObject(TargetShip, true, true); // Authored cutscene may select a neutral/non-contact station.
                            }
                        }
                    }
                    else if (TargetBody)
                    {
                        LegacyDirector->SetViewOrbital(TargetBody);
                    }

                    if (bHasTarget && !bBodyPrefix)
                    {
                        UE_LOG(LogTemp, Warning,
                            TEXT("[Phase9CameraTargetBinding] Authored='%s' Resolved='%s' Active='%s' Mode=%d"),
                            *TargetName,
                            TargetShip ? ANSI_TO_TCHAR(TargetShip->GetName()) : TEXT("<none>"),
                            LegacyDirector->GetViewObject() ? ANSI_TO_TCHAR(LegacyDirector->GetViewObject()->GetName()) : TEXT("<null>"),
                            LegacyDirector->GetMode());
                    }

                    if (LegacyCommand == 3)
                    {
                        LegacyDirector->SetOrbitPoint(
                            (double)Event.EventPoint.X,
                            (double)Event.EventPoint.Y,
                            (double)Event.EventPoint.Z);
                        if (bFarcasterNewsShot && TargetShip &&
                            LegacyDirector->GetViewObject() == TargetShip)
                        {
                            // The DataTable omits the orbit-rate event for this shot.
                            // 0.14 rad/s matches the later working Meridian/Tal Vesta
                            // camera pan. The next authored param-5 event takes over.
                            LegacyDirector->SetOrbitRates(0.14, 0.0, 0.0);
                            UE_LOG(LogTemp, Display,
                                TEXT("[Phase9FarcasterPan] Target='%s' AzRate=0.14 rad/s Mode=%d"),
                                ANSI_TO_TCHAR(TargetShip->GetName()), LegacyDirector->GetMode());
                        }
                    }
                    else if (LegacyCommand == 5)
                    {
                        LegacyDirector->SetOrbitRates(
                            (double)Event.EventPoint.X,
                            (double)Event.EventPoint.Y,
                            (double)Event.EventPoint.Z);
                    }

                    // Phase 4: switch legacy events to the passive UE view.
                    // Preserve ASSWCameraManager for scene-only compatibility shots.
                    ASSWLegacyCameraViewActor* PassiveView = nullptr;
                    UWorld* CameraWorld = GetWorld();
                    if (CameraWorld)
                    {
                        for (TActorIterator<ASSWLegacyCameraViewActor> It(CameraWorld); It; ++It)
                        {
                            if (IsValid(*It))
                            {
                                PassiveView = *It;
                                break;
                            }
                        }
                        if (!IsValid(PassiveView))
                        {
                            FActorSpawnParameters CameraSpawnParams;
                            CameraSpawnParams.SpawnCollisionHandlingOverride =
                                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                            // Start from the currently displayed transform until
                            // the runtime publishes the next authoritative frame.
                            FTransform SeedTransform = FTransform::Identity;
                            if (APlayerController* PC = CameraWorld->GetFirstPlayerController())
                            {
                                if (AActor* CurrentView = PC->GetViewTarget())
                                    SeedTransform = CurrentView->GetActorTransform();
                            }
                            PassiveView = CameraWorld->SpawnActor<ASSWLegacyCameraViewActor>(
                                ASSWLegacyCameraViewActor::StaticClass(),
                                SeedTransform, CameraSpawnParams);
                        }
                    }

                    if (IsValid(PassiveView))
                    {
                        // Reuse the same actor across events; never rewind its
                        // transform to a stale compatibility-camera position.
                        PassiveView->ActivateView(0.0f);
                    }
                    else
                    {
                        // If the passive UE endpoint cannot spawn, retain the
                        // Step 2 bridge instead of discarding the legacy event.
                        if (ASSWCameraManager* Compatibility = ResolveSceneCamera())
                        {
                            Compatibility->SetLegacyViewBridgeEnabled(true);
                            Compatibility->ActivateCamera(0.0f);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error,
                                TEXT("[SceneDlg] No UE presentation actor for legacy camera event"));
                        }
                    }
                    UE_LOG(LogTemp, Log,
                        TEXT("[SceneDlg] DataTable camera command=%d handled by legacy CameraManager target='%s'"),
                        LegacyCommand, *TargetName);
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[SceneDlg] Legacy camera target '%s' not in Sim; trying UE scene-only compatibility path"),
                    *TargetName);
            }
        }
    }

    // Scene-only compatibility path. The old Unreal camera is instantiated
    // ONLY if the legacy Sim could not resolve the event's requested target.
    // Phase 6 migration diagnostic: every entry here is a remaining
    // dependency on ASSWCameraManager and must be reviewed before deletion.
    // Log the original DataTable values; no camera behavior is changed.
    UE_LOG(LogTemp, Warning,
        TEXT("[LegacyCamera][COMPAT_FALLBACK] Command=%d Target='%s' Time=%.3f "
             "Point=%s Params=%d Sim=%s SimPlayerShip=%s"),
        LegacyCommand,
        *Event.EventTarget,
        (double)Event.EventTime,
        *Event.EventPoint.ToString(),
        Event.EventParam.Num(),
        Sim::GetSim() ? TEXT("YES") : TEXT("NO"),
        Sim::GetSim() && Sim::GetSim()->GetPlayerShip() ? TEXT("YES") : TEXT("NO"));

    ASSWCameraManager* Cam = ResolveSceneCamera();
    if (!Cam)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneDlg] ExecuteCameraEvent: no compatibility camera"));
        return;
    }

    // ----------------------------------------------------
    // Resolve target actor
    // 1) existing live mission AShipActor bound to runtime Ship
    // 2) temporary CampaignSceneActor object (non-runtime scene-only content)
    // 3) persistent system body
    // ----------------------------------------------------
    AActor* TargetActor = nullptr;

    if (!Event.EventTarget.IsEmpty())
    {
        const FString SearchTarget = Event.EventTarget.TrimStartAndEnd();

        const bool bPlanetOrbitEvent =
            Event.EventParam.Num() > 0 && Event.EventParam[0] == 3;
        if (bPlanetOrbitEvent)
        {
            if (ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder())
            {
                FSpawnedSystemBody Body;
                if (Builder->FindSpawnedBodyByName(SearchTarget, Body) && IsValid(Body.Actor))
                {
                    TargetActor = Body.Actor;
                    UE_LOG(LogTemp, Warning,
                        TEXT("[PlanetCamera] Resolved planet '%s' -> '%s'"),
                        *SearchTarget, *GetNameSafe(TargetActor));
                }
            }
        }

        // Live mission ships are authoritative. Match against the legacy
        // runtime Ship name, not Unreal-generated actor names.
        if (!TargetActor)
        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<AShipActor> It(World); It; ++It)
            {
                AShipActor* ShipActor = *It;
                Ship* RuntimeShip = ShipActor ? ShipActor->GetRuntimeShip() : nullptr;
                if (!RuntimeShip)
                {
                    continue;
                }

                const FString RuntimeName = ANSI_TO_TCHAR(RuntimeShip->GetName());
                if (RuntimeName.Equals(SearchTarget, ESearchCase::IgnoreCase))
                {
                    TargetActor = ShipActor;
                    UE_LOG(LogTemp, Log,
                        TEXT("[SceneDlg] Camera target '%s' -> live AShipActor '%s'"),
                        *SearchTarget,
                        *GetNameSafe(ShipActor));
                    break;
                }
            }
        }

        // Scene-only objects may still be supplied by CampaignSceneActor, but
        // it no longer builds duplicate mission ships.
        if (!TargetActor)
        {
            ACampaignSceneActor* SceneActor = ResolveCampaignSceneActor();
            if (SceneActor)
            {
                TargetActor = SceneActor->FindSceneActorByName(SearchTarget);
            }
        }

        // Planets / moons / persistent system bodies.
        if (!TargetActor)
        {
            ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();
            if (Builder)
            {
                FSpawnedSystemBody FoundBody;

                if (Builder->FindSpawnedBodyByName(SearchTarget, FoundBody))
                {
                    TargetActor = FoundBody.Actor;
                }
            }
        }
    }

    // A scene-only target is rendered by the existing UE fallback, not
    // overwritten on the next frame by the legacy camera bridge.
    Cam->SetLegacyViewBridgeEnabled(false);

    // ----------------------------------------------------
    // Decode legacy camera param
    // ----------------------------------------------------
    const int32 Param =
        Event.EventParam.Num() > 0 ? Event.EventParam[0] : 0;

    // ----------------------------------------------------
    // PARAM 3 -> ORBIT CAMERA (planets, cinematic shots)
    // ----------------------------------------------------
    if (Param == 3 && TargetActor)
    {
        FVector Orbit = Event.EventPoint;

        ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();

        // Camera range is specified in legacy kilometres by the mission .def.
        // Reuse the system builder's conversion; do not substitute a guessed
        // camera distance derived from the visual radius of the planet.
        if (Builder)
        {
            Orbit.Z = Builder->ConvertOrbitKmToSceneUnits(Orbit.Z, false);
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("[PlanetCamera] Missing builder: cannot convert legacy camera range"));
            return;
        }

        if (Orbit.Z <= 0.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[PlanetCamera] Invalid converted range for '%s': %s"),
                *Event.EventTarget, *Orbit.ToString());
            return;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetCamera] Target='%s' Actor='%s' ActorLocation=%s CameraBefore=%s ConvertedOrbit=%s"),
            *Event.EventTarget, *GetNameSafe(TargetActor),
            *TargetActor->GetActorLocation().ToString(),
            *Cam->GetActorLocation().ToString(), *Orbit.ToString());

        Cam->SetBodyOrbitView(TargetActor, Orbit);
        Cam->ActivateCamera(0.0f);

        // Diagnostic only: identify the camera actually driving the viewport.
        if (UWorld* CameraWorld = GetWorld())
        {
            if (APlayerController* PC = CameraWorld->GetFirstPlayerController())
            {
                AActor* ActiveViewTarget = PC->GetViewTarget();
                UE_LOG(LogTemp, Error,
                    TEXT("[CAMERA CHECK] Expected=%s Actual=%s"),
                    *GetNameSafe(Cam), *GetNameSafe(ActiveViewTarget));
                UE_LOG(LogTemp, Error,
                    TEXT("[CAMERA CHECK] Camera=%s Target=%s"),
                    *Cam->GetActorLocation().ToString(), *GetNameSafe(TargetActor));
                UE_LOG(LogTemp, Error,
                    TEXT("[CAMERA CHECK] PlanetLocation=%s"),
                    TargetActor ? *TargetActor->GetActorLocation().ToString() : TEXT("NULL"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[CAMERA CHECK] No PlayerController"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[CAMERA CHECK] No World"));
        }
        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetCamera] CameraAfter=%s Rotation=%s"),
            *Cam->GetActorLocation().ToString(), *Cam->GetActorRotation().ToString());
        return;
    }

    // ----------------------------------------------------
    // PARAM 5 -> ORBIT RATES (continuous motion)
    // ----------------------------------------------------
    if (Param == 5)
    {
        FVector Rates = Event.EventPoint;

        Cam->SetOrbitRates(Rates);

        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] param 5 -> OrbitRates %s"),
            *Rates.ToString());

        return;
    }

    // ----------------------------------------------------
    // PARAM 6 -> FOLLOW TARGET (ships, hero shots)
    // ----------------------------------------------------
    if (Param == 6 && TargetActor)
    {
        ACampaignSceneActor* SceneActor = ResolveCampaignSceneActor();

        const float BlendSeconds =
            Event.EventNParams > 1
            ? FMath::Max(0.0f, (float)Event.EventParam[1])
            : 0.5f;

        if (SceneActor)
        {
            const int32 GroupCount =
                SceneActor->GetCommanderGroupActorCount(Event.EventTarget);

            if (GroupCount >= 2)
            {
                if (SceneActor->FocusCameraOnCommanderGroup(
                    Event.EventTarget,
                    BlendSeconds))
                {
                    return;
                }
            }
        }

        FVector FollowOffset = Event.EventOffset;

        if (FollowOffset.IsNearlyZero())
        {
            FollowOffset = FVector(-2500.0f, 900.0f, 650.0f);
        }

        Cam->SetActorFollowView(
            TargetActor,
            FollowOffset,
            Event.EventRotator);

        Cam->ActivateCamera(BlendSeconds);

        return;
    }

    // ----------------------------------------------------
    // FALLBACK
    // ----------------------------------------------------
    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] Camera Event: no match param=%d TargetFound=%s"),
        Param,
        TargetActor ? TEXT("true") : TEXT("false"));
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
    // UI-only input mode may consume keys before Enhanced Input sees them.
    if (InKeyEvent.GetKey() == EKeys::Delete && bSceneRunning && !bCutsceneTransitionStarted)
    {
        OnCutsceneExit();
        return FReply::Handled();
    }
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
    if (bCutsceneTransitionStarted)
    {
        return;
    }

    bCutsceneTransitionStarted = true;
    bSceneRunning = false;
    bPanelActive = false;

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] Cutscene finished -> returning to persistent Operations world"));

    ClearPanelTexture();
    Hide();

    // No camera-mode reset here: UseLiveSimulation is not part of
    // ASSWCameraManager's declared API. Scene cleanup remains unchanged.

    SceneCamera = nullptr;

    if (Manager)
    {
        Manager->CleanupActiveScene();
        Manager->ShowCmdDlg();
    }
    ResetSceneState();
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

ACampaignSceneActor* UCampaignSceneDlg::ResolveCampaignSceneActor() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneDlg] ResolveCampaignSceneActor: World is null"));
        return nullptr;
    }

    for (TActorIterator<ACampaignSceneActor> It(World); It; ++It)
    {
        ACampaignSceneActor* SceneActor = *It;
        if (IsValid(SceneActor))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SceneDlg] ResolveCampaignSceneActor: found '%s'"),
                *GetNameSafe(SceneActor));
            return SceneActor;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ResolveCampaignSceneActor: no CampaignSceneActor found"));

    return nullptr;
}

ASSWCameraManager* UCampaignSceneDlg::ResolveSceneCamera()
{
    if (IsValid(SceneCamera))
        return SceneCamera;

    UWorld* World = GetWorld();
    if (!World)
        return nullptr;

    for (TActorIterator<ASSWCameraManager> It(World); It; ++It)
    {
        ASSWCameraManager* Candidate = *It;
        if (IsValid(Candidate))
        {
            SceneCamera = Candidate;
            return SceneCamera;
        }
    }

    // This persistent world has no authored ASSWCameraManager. Create exactly one
    // on demand, after system-body preflight and before the first camera event.
    // Theater's ACameraActor is independent and must not be reused for cutscenes.
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SceneCamera = World->SpawnActor<ASSWCameraManager>(
        ASSWCameraManager::StaticClass(),
        FTransform::Identity,
        SpawnParams);

    if (!IsValid(SceneCamera))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneDlg] ResolveSceneCamera: failed to spawn ASSWCameraManager"));
        return nullptr;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneDlg] ResolveSceneCamera: spawned persistent camera=%s World=%s"),
        *GetNameSafe(SceneCamera), *GetNameSafe(World));
    // Do not activate until the camera event has positioned it at its target.
    return SceneCamera;
}

void UCampaignSceneDlg::NativeDestruct()
{
    StopCutsceneAudio();
    DisableCutsceneExitInput();
    ClearPanelTexture();

    // No unsupported camera-mode reset on widget destruction.

    SceneCamera = nullptr;
    ResetSceneState();
    Super::NativeDestruct();
}

void UCampaignSceneDlg::OnCutsceneExit()
{
    if (bSceneRunning && !bCutsceneTransitionStarted) SkipCutscene();
}

void UCampaignSceneDlg::EnableCutsceneExitInput()
{
    if (CutsceneExitInput) return;
    APlayerController* PC = GetOwningPlayer();
    if (!PC) PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Input = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Input) return;
    CutsceneExitAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_Exit.IA_Exit"));
    if (!CutsceneExitAction)
    {
        CutsceneExitAction = NewObject<UInputAction>(this,NAME_None,RF_Transient);
        CutsceneExitAction->ValueType = EInputActionValueType::Boolean;
    }
    if (CutsceneExitAction->ValueType != EInputActionValueType::Boolean)
    {
        UE_LOG(LogTemp,Warning,TEXT("[SceneDlg] IA_Exit must be Boolean."));
        CutsceneExitAction = nullptr;
        return;
    }
    CutsceneInputOwner = PC;
    CutsceneExitContext = NewObject<UInputMappingContext>(this,NAME_None,RF_Transient);
    CutsceneExitContext->MapKey(CutsceneExitAction.Get(),EKeys::Delete);
    CutsceneExitInput = NewObject<UEnhancedInputComponent>(PC);
    CutsceneExitInput->RegisterComponent();
    CutsceneExitInput->Priority = 2000;
    CutsceneExitInput->bBlockInput = true;
    CutsceneExitInput->BindAction(CutsceneExitAction.Get(),ETriggerEvent::Started,this,&UCampaignSceneDlg::OnCutsceneExit);
    Input->AddMappingContext(CutsceneExitContext.Get(),2000);
    PC->PushInputComponent(CutsceneExitInput.Get());
}

void UCampaignSceneDlg::DisableCutsceneExitInput()
{
    if (APlayerController* PC = CutsceneInputOwner.Get())
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
            if (auto* Input = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                if (CutsceneExitContext) Input->RemoveMappingContext(CutsceneExitContext.Get());
        if (CutsceneExitInput) PC->PopInputComponent(CutsceneExitInput.Get());
    }
    if (CutsceneExitInput) CutsceneExitInput->DestroyComponent();
    CutsceneExitInput = nullptr;
    CutsceneExitContext = nullptr;
    CutsceneExitAction = nullptr;
    CutsceneInputOwner.Reset();
}


void UCampaignSceneDlg::StopCutsceneAudio()
{
    // Clear ownership first: Stop may synchronously invoke audio-finished callbacks.
    const TArray<TWeakObjectPtr<UAudioComponent>> AudioToStop = MoveTemp(CutsceneAudioComponents);
    CutsceneAudioComponents.Reset();
    for (const TWeakObjectPtr<UAudioComponent>& Entry : AudioToStop)
    {
        if (UAudioComponent* Audio = Entry.Get()) { Audio->Stop(); Audio->DestroyComponent(); }
    }
}
