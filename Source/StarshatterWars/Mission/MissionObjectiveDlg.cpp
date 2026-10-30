#include "MissionObjectiveDlg.h"
#include "ShipBriefingPreview.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "MissionUIStyle.h"


#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"
#include "ShipDesignRegistry.h"

#include "Mission.h"
#include "MissionElement.h"
#include "ShipDesign.h"
#include "Ship.h"
#include "GameStructs.h"

UMissionObjectiveDlg::UMissionObjectiveDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}


TSharedRef<SWidget> UMissionObjectiveDlg::RebuildWidget()
{
    BuildSituationLayout();
    return Super::RebuildWidget();
}

void UMissionObjectiveDlg::BuildSituationLayout()
{
    if (!WidgetTree || PresentationRoot) return;
    if (!SituationText) SituationText = WidgetTree->ConstructWidget<UTextBlock>();
    if (!ObjectivesText) ObjectivesText = WidgetTree->ConstructWidget<UTextBlock>();
    if (!PlayerCaptionText) PlayerCaptionText = WidgetTree->ConstructWidget<UTextBlock>();
    if (!PreviewImage) PreviewImage = WidgetTree->ConstructWidget<UImage>();
    for (UWidget* Widget : { static_cast<UWidget*>(SituationText), static_cast<UWidget*>(ObjectivesText),
        static_cast<UWidget*>(PlayerCaptionText), static_cast<UWidget*>(PreviewImage) })
        Widget->RemoveFromParent();

    PresentationRoot = WidgetTree->ConstructWidget<UBorder>();
    PresentationRoot->SetPadding(FMargin(16.f));
    PresentationRoot->SetBrushColor(FLinearColor(0.045f, 0.055f, 0.065f, 0.96f));
    WidgetTree->RootWidget = PresentationRoot;
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    PresentationRoot->SetContent(Row);
    UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextColumn);
    FSlateChildSize TextSize(ESlateSizeRule::Fill); TextSize.Value = 2.f;
    TextSlot->SetSize(TextSize);
    TextSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));

    auto Section = [this](UVerticalBox* Column, const FText& Heading, UTextBlock* Text)->UScrollBox*
    {
        UBorder* Header = WidgetTree->ConstructWidget<UBorder>();
        Header->SetBrushColor(FLinearColor(0.105f, 0.113f, 0.125f, 1.f));
        Header->SetPadding(FMargin(12.f, 8.f));
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(Heading);
        Label->SetFont(MissionUIStyle::GetHeaderFont(22));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
        Header->SetContent(Label);
        Column->AddChildToVerticalBox(Header);
        UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
        Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
        Scroll->SetAnimateWheelScrolling(true);
        Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
        Scroll->AddChild(Text);
        Text->SetAutoWrapText(true);
        Text->SetWrapTextAt(0.f);
        if (UScrollBoxSlot* BodySlot = Cast<UScrollBoxSlot>(Text->Slot))
            BodySlot->SetPadding(FMargin(12.f));
        UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
        return Scroll;
    };
    SituationScroll = Section(TextColumn, NSLOCTEXT("MissionBriefing", "SituationHeading", "SITUATION"), SituationText);
    ObjectivesScroll = Section(TextColumn, NSLOCTEXT("MissionBriefing", "ObjectivesHeading", "OBJECTIVES"), ObjectivesText);

    UVerticalBox* ShipColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    Row->AddChildToHorizontalBox(ShipColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UTextBlock* ShipTitle = WidgetTree->ConstructWidget<UTextBlock>();
    ShipTitle->SetText(NSLOCTEXT("MissionBriefing", "PlayerShipHeading", "PLAYER SHIP"));
    ShipTitle->SetFont(MissionUIStyle::GetHeaderFont(22));
    ShipTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
    ShipColumn->AddChildToVerticalBox(ShipTitle)->SetPadding(FMargin(12.f, 8.f));
    UScaleBox* PreviewFit = WidgetTree->ConstructWidget<UScaleBox>();
    PreviewFit->SetStretch(EStretch::ScaleToFit);
    PreviewFit->SetContent(PreviewImage);
    UOverlay* PreviewArea = WidgetTree->ConstructWidget<UOverlay>();
    UOverlaySlot* ImageSlot = PreviewArea->AddChildToOverlay(PreviewFit);
    ImageSlot->SetHorizontalAlignment(HAlign_Fill);
    ImageSlot->SetVerticalAlignment(VAlign_Fill);
    ShipPreview = WidgetTree->ConstructWidget<UShipBriefingPreview>();
    if (ShipPreview)
    {
        UOverlaySlot* PreviewSlot = PreviewArea->AddChildToOverlay(ShipPreview);
        PreviewSlot->SetHorizontalAlignment(HAlign_Fill);
        PreviewSlot->SetVerticalAlignment(VAlign_Fill);
        ShipPreview->SetVisibility(ESlateVisibility::Collapsed);
        ShipPreview->SetToolTipText(NSLOCTEXT("MissionBriefing", "ShipPreviewControls", "Drag to rotate. Mouse wheel to zoom. Double-click to reset."));
    }
    UBorder* PreviewFrame = WidgetTree->ConstructWidget<UBorder>();
    PreviewFrame->SetBrushColor(FLinearColor(0.012f, 0.014f, 0.018f, 1.f));
    PreviewFrame->SetPadding(FMargin(4.f));
    PreviewFrame->SetContent(PreviewArea);
    UVerticalBoxSlot* FrameSlot = ShipColumn->AddChildToVerticalBox(PreviewFrame);
    FrameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    FrameSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 0.f));
    PreviewStatusText = WidgetTree->ConstructWidget<UTextBlock>();
    PreviewStatusText->SetText(NSLOCTEXT("MissionBriefing", "PreviewUnavailable", "PREVIEW UNAVAILABLE"));
    PreviewStatusText->SetFont(MissionUIStyle::GetInfoValueFont(16));
    PreviewStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.8f)));
    PreviewStatusText->SetJustification(ETextJustify::Center);
    ShipColumn->AddChildToVerticalBox(PreviewStatusText);
    PlayerCaptionText->SetFont(MissionUIStyle::GetInfoValueFont(18));
    PlayerCaptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
    PlayerCaptionText->SetAutoWrapText(true);
    PlayerCaptionText->SetJustification(ETextJustify::Center);
    ShipColumn->AddChildToVerticalBox(PlayerCaptionText)->SetPadding(FMargin(12.f));
    ApplyBriefingTextPresentation();
}

void UMissionObjectiveDlg::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyBriefingTextPresentation();
}

void UMissionObjectiveDlg::ApplyBriefingTextPresentation()
{
    if (!bModernBriefingText) return;
    for (UTextBlock* Text : { SituationText, ObjectivesText })
    {
        if (!Text) continue;
        FSlateFontInfo Font = MissionUIStyle::GetInfoValueFont();
        Font.Size = FMath::Clamp(BriefingBodyFontSize, 12, 24);
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(BriefingBodyColor));
        Text->SetAutoWrapText(true);
    }
}

void UMissionObjectiveDlg::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyBriefingTextPresentation();

    UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] NativeConstruct"));
}

void UMissionObjectiveDlg::SetShipPreviewActive(bool bActive)
{
    bShipPreviewActive = bActive;
    if (ShipPreview) ShipPreview->SetPreviewActive(bActive && bUse3DShipPreview);
}

void UMissionObjectiveDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] SetParentDlg: %s"),
        ParentDlg ? TEXT("VALID") : TEXT("NULL"));
}

Mission* UMissionObjectiveDlg::ResolveMission() const
{
    if (!ParentDlg)
    {
        return nullptr;
    }

    return ParentDlg->GetMissionPtr();
}

void UMissionObjectiveDlg::RefreshFromMission()
{
    Mission* MissionPtr = ResolveMission();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] RefreshFromMission: Mission=%p Name='%s' Sit='%s' Obj='%s' Scripted=%s"),
        MissionPtr,
        (MissionPtr && MissionPtr->GetName()) ? ANSI_TO_TCHAR(MissionPtr->GetName()) : TEXT("NULL"),
        (MissionPtr && MissionPtr->GetSituation()) ? UTF8_TO_TCHAR(MissionPtr->GetSituation()) : TEXT("NULL"),
        (MissionPtr && MissionPtr->GetObjective()) ? UTF8_TO_TCHAR(MissionPtr->GetObjective()) : TEXT("NULL"),
        (MissionPtr && MissionPtr->IsScripted()) ? TEXT("true") : TEXT("false"));

    const FText OldSituation = SituationText ? SituationText->GetText() : FText::GetEmpty();
    const FText OldObjectives = ObjectivesText ? ObjectivesText->GetText() : FText::GetEmpty();
    RefreshSituationAndObjectives(MissionPtr);
    if (SituationScroll && SituationText && !OldSituation.EqualTo(SituationText->GetText()))
        SituationScroll->ScrollToStart();
    if (ObjectivesScroll && ObjectivesText && !OldObjectives.EqualTo(ObjectivesText->GetText()))
        ObjectivesScroll->ScrollToStart();
    RefreshPlayerCaption(MissionPtr);
    RefreshPreviewPlaceholder(MissionPtr);
}

void UMissionObjectiveDlg::RefreshSituationAndObjectives(Mission* MissionPtr)
{
    const FString SitStr = (MissionPtr && MissionPtr->GetSituation())
        ? FString(UTF8_TO_TCHAR(MissionPtr->GetSituation()))
        : FString();

    const FString ObjStr = (MissionPtr && MissionPtr->GetObjective())
        ? FString(UTF8_TO_TCHAR(MissionPtr->GetObjective()))
        : FString();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] RefreshSituationAndObjectives: Mission=%p IsOK=%s Sit='%s' Obj='%s'"),
        MissionPtr,
        (MissionPtr && MissionPtr->IsOK()) ? TEXT("true") : TEXT("false"),
        *SitStr,
        *ObjStr);

    if (ObjectivesText)
    {
        if (MissionPtr)
        {
            ObjectivesText->SetText(
                ObjStr.IsEmpty()
                ? FText::FromString(TEXT("NO OBJECTIVES"))
                : FText::FromString(ObjStr));
        }
        else
        {
            ObjectivesText->SetText(FText::FromString(TEXT("NO MISSION")));
        }
    }

    if (SituationText)
    {
        if (MissionPtr)
        {
            SituationText->SetText(
                SitStr.IsEmpty()
                ? FText::FromString(TEXT("NO SITUATION"))
                : FText::FromString(SitStr));
        }
        else
        {
            SituationText->SetText(FText::FromString(TEXT("NO MISSION")));
        }
    }
}

void UMissionObjectiveDlg::RefreshPlayerCaption(Mission* MissionPtr)
{
    if (!PlayerCaptionText)
    {
        return;
    }

    PlayerCaptionText->SetText(FText::GetEmpty());

    if (!MissionPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] RefreshPlayerCaption: MissionPtr is null"));
        return;
    }

    MissionElement* PlayerElem = MissionPtr->GetPlayer();
    if (!PlayerElem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] No player element"));
        return;
    }

    const FShipDesign* Design = PlayerElem->GetShipDesign();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] Player='%s' Design=%s"),
        ANSI_TO_TCHAR(PlayerElem->GetName().data()),
        Design ? TEXT("VALID") : TEXT("NULL"));

    if (Design)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Design ship='%s' display='%s' abrv='%s' type=%d"),
            *Design->ShipName,
            *Design->DisplayName,
            *Design->Abrv,
            Design->ShipType);
    }

    FString Caption;

    if (Design)
    {
        const FString Abbrev =
            !Design->Abrv.IsEmpty() ? Design->Abrv : TEXT("UNIT");

        const FString DisplayName =
            !Design->DisplayName.IsEmpty() ? Design->DisplayName : Design->ShipName;

        const FString ElemName = ANSI_TO_TCHAR(PlayerElem->GetName().data());

        if (Design->ShipType <= (int32)CLASSIFICATION::ATTACK)
        {
            Caption = FString::Printf(
                TEXT("%s %s"),
                *Abbrev,
                DisplayName.IsEmpty() ? TEXT("UNKNOWN") : *DisplayName);
        }
        else
        {
            Caption = FString::Printf(
                TEXT("%s %s"),
                *Abbrev,
                ElemName.IsEmpty() ? TEXT("UNKNOWN") : *ElemName);
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Caption from FShipDesign: '%s'"),
            *Caption);
    }
    else
    {
        Caption = ANSI_TO_TCHAR(PlayerElem->GetName().data());

        if (Caption.IsEmpty())
        {
            Caption = TEXT("UNKNOWN UNIT");
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Caption fallback from player element name: '%s'"),
            *Caption);
    }

    PlayerCaptionText->SetText(FText::FromString(Caption));
}

void UMissionObjectiveDlg::RefreshPreviewPlaceholder(Mission* MissionPtr)
{
    if (!PreviewImage)
    {
        return;
    }

    if (PreviewStatusText) PreviewStatusText->SetVisibility(ESlateVisibility::Visible);
    PreviewImage->SetVisibility(ESlateVisibility::Visible);
    PreviewImage->SetBrushFromTexture(nullptr);

    MissionElement* PlayerElem = (MissionPtr && MissionPtr->IsOK()) ? MissionPtr->GetPlayer() : nullptr;
    const FShipDesign* Design = PlayerElem ? PlayerElem->GetShipDesign() : nullptr;
    if (ShipPreview)
    {
        const bool bHasModel = bUse3DShipPreview && ShipPreview->SetShipDesign(Design);
        ShipPreview->SetPreviewActive(bHasModel && bShipPreviewActive);
        if (bHasModel)
        {
            PreviewImage->SetVisibility(ESlateVisibility::Collapsed);
            if (PreviewStatusText) PreviewStatusText->SetVisibility(ESlateVisibility::Collapsed);
            return;
        }
    }
    if (!Design || Design->Map.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] Preview: no player design/sprite"));
        return;
    }

    const FString ShipName = Design->ShipName;
    const FString SpriteName = Design->Map[0].SpriteName;
    const FString Path = FString::Printf(TEXT("/Game/UI/Ships/%s/%s.%s"), *ShipName, *SpriteName, *SpriteName);
    if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path))
    {
        PreviewImage->SetBrushFromTexture(Tex, true);
        if (PreviewStatusText) PreviewStatusText->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] Preview: set %s"), *Path);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] Preview: missing texture %s"), *Path);
    }
}