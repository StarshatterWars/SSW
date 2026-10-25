/*=============================================================================
    Project:        Starshatter Wars (Unreal Port)
    Studio:         Fractal Dev Studios
    FILE:           ExitDlg.cpp
    AUTHOR:         Carlos Bott
=============================================================================*/

#include "ExitDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/RichTextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MissionUIStyle.h"
#include "StarshatterUIStyleSubsystem.h"
#include "Engine/GameInstance.h"

// UE
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/KismetSystemLibrary.h"

// Router
#include "MenuScreen.h"

DEFINE_LOG_CATEGORY_STATIC(LogExitDlg, Log, All);

/* --------------------------------------------------------------------
   Construction
   -------------------------------------------------------------------- */

UExitDlg::UExitDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

/* --------------------------------------------------------------------
   NativeConstruct
   -------------------------------------------------------------------- */

void UExitDlg::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogExitDlg, Log, TEXT("[ExitDlgUI] NativeConstruct class=%s root=%s"),
        *GetClass()->GetName(), WidgetTree ? *GetNameSafe(WidgetTree->RootWidget) : TEXT("NULL"));
    BuildModernExitScreen();
    UE_LOG(LogExitDlg, Warning, TEXT("[ExitDlgUI] NativeConstruct completed: viewport=%d visibility=%d"),
        IsInViewport() ? 1 : 0, static_cast<int32>(GetVisibility()));

    // Avoid duplicate delegate binding if widget rebuilds:
    if (ExitBtn)
    {
        ExitBtn->OnClicked.RemoveAll(this);
        ExitBtn->OnClicked.AddDynamic(this, &UExitDlg::OnExitClicked);
    }
    else
    {
        UE_LOG(LogExitDlg, Warning, TEXT("ExitBtn is null (BindWidgetOptional mismatch or not IsVariable)."));
    }

    if (CancelBtn)
    {
        CancelBtn->OnClicked.RemoveAll(this);
        CancelBtn->OnClicked.AddDynamic(this, &UExitDlg::OnCancelClicked);
    }
    else
    {
        UE_LOG(LogExitDlg, Warning, TEXT("CancelBtn is null (BindWidgetOptional mismatch or not IsVariable)."));
    }

    bExitLatch = true;
}

/* --------------------------------------------------------------------
   Modern code-built exit screen (1920x1080 friendly)
   -------------------------------------------------------------------- */

void UExitDlg::BuildModernExitScreen()
{
    if (!WidgetTree)
        return;
    // A Blueprint can recreate its widget tree between viewport attaches.
    // Rebuild the modal into its active CanvasPanel whenever NativeConstruct runs.

    const FLinearColor Backdrop(0.012f, 0.025f, 0.045f, 0.72f);
    const FLinearColor Panel(0.025f, 0.055f, 0.083f, 0.98f);
    const FLinearColor Raised(0.052f, 0.098f, 0.138f, 1.f);
    const FLinearColor TitleBand(0.105f, 0.113f, 0.125f, 1.f);
    const FLinearColor Accent(0.16f, 0.77f, 0.91f, 1.f);
    const FLinearColor Primary(0.92f, 0.96f, 0.98f, 1.f);
    const FLinearColor Muted(0.61f, 0.72f, 0.81f, 1.f);

    auto MakeText = [&](const FString& Value, int32 FontSize, const FLinearColor& Color, bool bHeading = false) -> UTextBlock*
    {
        UTextBlock* TextWidget = WidgetTree->ConstructWidget<UTextBlock>();
        TextWidget->SetText(FText::FromString(Value));
        TextWidget->SetColorAndOpacity(FSlateColor(Color));
        TextWidget->SetFont(bHeading ? MissionUIStyle::GetHeaderFont(FontSize)
            : (FontSize <= 13 ? MissionUIStyle::GetInfoLabelFont(FontSize)
                : MissionUIStyle::GetRowFont(FontSize)));
        TextWidget->SetAutoWrapText(true);
        return TextWidget;
    };

    auto MakeBorder = [&](const FLinearColor& Color) -> UBorder*
    {
        UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
        Border->SetBrushColor(Color);
        return Border;
    };

    // IMPORTANT: NativeConstruct runs after Slate can already reference the
    // Blueprint-authored RootWidget. Replacing RootWidget here can leave the
    // legacy WBP visible. Reuse that live container and clear its old children.
    UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (Root)
    {
        Root->ClearChildren();
        UE_LOG(LogExitDlg, Log, TEXT("[ExitDlgUI] Reusing Blueprint CanvasPanel root and clearing legacy widgets."));
    }
    else if (UPanelWidget* DesignerRoot = Cast<UPanelWidget>(WidgetTree->RootWidget))
    {
        DesignerRoot->ClearChildren();
        Root = WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass(), TEXT("ExitDlg_ModernRoot"));
        DesignerRoot->AddChild(Root);
        UE_LOG(LogExitDlg, Log, TEXT("[ExitDlgUI] Reusing existing Blueprint panel root (%s)."),
            *GetNameSafe(DesignerRoot));
    }
    else
    {
        Root = WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass(), TEXT("ExitDlg_ModernRoot"));
        WidgetTree->RootWidget = Root;
        UE_LOG(LogExitDlg, Warning,
            TEXT("[ExitDlgUI] No reusable panel root; installed CanvasPanel fallback."));
    }

    // Fill the existing WBP root with a dimming overlay, then place the
    // actual dialog in an explicitly centered CanvasPanel slot. This avoids
    // collapsed desired geometry in the old full-screen Border/ScaleBox chain.
    UBorder* Back = MakeBorder(Backdrop);
    Back->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (UCanvasPanelSlot* BackdropCanvasSlot = Root->AddChildToCanvas(Back))
    {
        BackdropCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        BackdropCanvasSlot->SetOffsets(FMargin(0.f));
        BackdropCanvasSlot->SetZOrder(0);
    }

    USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("ExitDlg_ModalFrame"));
    Frame->SetWidthOverride(780.f);
    Frame->SetHeightOverride(420.f);
    if (UCanvasPanelSlot* ModalCanvasSlot = Root->AddChildToCanvas(Frame))
    {
        ModalCanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        ModalCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        ModalCanvasSlot->SetSize(FVector2D(780.f, 420.f));
        ModalCanvasSlot->SetPosition(FVector2D::ZeroVector);
        ModalCanvasSlot->SetZOrder(10);
    }
    else
    {
        UE_LOG(LogExitDlg, Error, TEXT("[ExitDlgUI] FAILED to attach modal frame to RootCanvas"));
    }

    UBorder* MainPanel = MakeBorder(Panel);
    MainPanel->SetPadding(FMargin(20.f, 16.f, 20.f, 16.f));
    Frame->SetContent(MainPanel);

    UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>();
    MainPanel->SetContent(Main);

    Main->AddChildToVerticalBox(MakeText(
        TEXT("STARSHATTER WARS  /  SESSION CONTROL"), 12, Accent))
        ->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

    UBorder* HeadingBand = MakeBorder(TitleBand);
    HeadingBand->SetPadding(FMargin(14.f, 8.f));
    HeadingBand->SetContent(MakeText(TEXT("EXIT STARSHATTER WARS"), 28, Primary, true));
    Main->AddChildToVerticalBox(HeadingBand)->SetPadding(FMargin(0.f, 0.f, 0.f, 7.f));

    Main->AddChildToVerticalBox(MakeText(
        TEXT("END CURRENT SESSION  /  RETURN TO OPERATING SYSTEM"), 12, Muted))
        ->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));

    UBorder* HeaderLine = MakeBorder(Accent);
    USizeBox* LineHeight = WidgetTree->ConstructWidget<USizeBox>();
    LineHeight->SetHeightOverride(2.f);
    HeaderLine->SetContent(LineHeight);
    Main->AddChildToVerticalBox(HeaderLine)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

    UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
    UVerticalBoxSlot* ContentSlot = Main->AddChildToVerticalBox(Content);
    ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ContentSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

    UBorder* NoticePanel = MakeBorder(Raised);
    NoticePanel->SetPadding(FMargin(15.f));
    UHorizontalBoxSlot* NoticeSlot = Content->AddChildToHorizontalBox(NoticePanel);
    FSlateChildSize NoticeSize(ESlateSizeRule::Fill);
    NoticeSize.Value = 1.05f;
    NoticeSlot->SetSize(NoticeSize);
    NoticeSlot->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));

    UVerticalBox* Notice = WidgetTree->ConstructWidget<UVerticalBox>();
    NoticePanel->SetContent(Notice);
    Notice->AddChildToVerticalBox(MakeText(TEXT("01  /  CONFIRM EXIT"), 13, Accent))
        ->SetPadding(FMargin(0.f, 0.f, 0.f, 11.f));
    Notice->AddChildToVerticalBox(MakeText(TEXT("TERMINATE SESSION"), 20, Primary, true))
        ->SetPadding(FMargin(0.f, 0.f, 0.f, 9.f));
    Notice->AddChildToVerticalBox(MakeText(
        TEXT("Are you sure you want to exit Starshatter Wars?"), 15, Primary))
        ->SetPadding(FMargin(0.f, 0.f, 0.f, 11.f));
    Notice->AddChildToVerticalBox(MakeText(
        TEXT("Any unsaved mission or campaign progress may be lost. Select CANCEL to return to the main menu."),
        13, Muted));

    UBorder* FooterLine = MakeBorder(Accent);
    USizeBox* FooterLineSize = WidgetTree->ConstructWidget<USizeBox>();
    FooterLineSize->SetHeightOverride(2.f);
    FooterLine->SetContent(FooterLineSize);
    Main->AddChildToVerticalBox(FooterLine)->SetPadding(FMargin(0.f, 0.f, 0.f, 9.f));

    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Footer);

    UTextBlock* FooterHint = MakeText(TEXT("AWAITING CONFIRMATION"), 11, Muted);
    Footer->AddChildToHorizontalBox(FooterHint)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto MakeMenuButton = [&](const TCHAR* Name, const FString& Label, TObjectPtr<UButton>& OutButton)
    {
        USizeBox* SizeWrapper = WidgetTree->ConstructWidget<USizeBox>();
        SizeWrapper->SetWidthOverride(180.f);
        SizeWrapper->SetHeightOverride(40.f);
        OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
        UTextBlock* ButtonText = MakeText(Label, 13, FLinearColor::Black);
        ButtonText->SetJustification(ETextJustify::Center);
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(OutButton->AddChild(ButtonText)))
        {
            ButtonSlot->SetPadding(FMargin(9.f, 5.f));
            ButtonSlot->SetHorizontalAlignment(HAlign_Center);
            ButtonSlot->SetVerticalAlignment(VAlign_Center);
        }
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UStarshatterUIStyleSubsystem* Style = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
                Style->ApplyMenuButtonStyle(OutButton);
        }
        SizeWrapper->SetContent(OutButton);
        Footer->AddChildToHorizontalBox(SizeWrapper)->SetPadding(FMargin(7.f, 0.f, 0.f, 0.f));
    };

    MakeMenuButton(TEXT("ExitDlg_ModernCancelBtn"), TEXT("CANCEL"), CancelBtn);
    MakeMenuButton(TEXT("ExitDlg_ModernExitBtn"), TEXT("EXIT GAME"), ExitBtn);
    UE_LOG(LogExitDlg, Warning, TEXT("[ExitDlgUI] Modal built. Root=%s, RootChildren=%d, Frame=%s, ExitBtn=%s, CancelBtn=%s"),
        *GetNameSafe(Root), Root ? Root->GetChildrenCount() : -1,
        *GetNameSafe(Frame), *GetNameSafe(ExitBtn), *GetNameSafe(CancelBtn));
}

/* --------------------------------------------------------------------
   NativeTick
   -------------------------------------------------------------------- */

void UExitDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    (void)InDeltaTime;
}

/* --------------------------------------------------------------------
   Input handling
   -------------------------------------------------------------------- */

void UExitDlg::HandleAccept()
{
    Super::HandleAccept();
    OnExitClicked();
}

void UExitDlg::HandleCancel()
{
    // Legacy latch: swallow the very first cancel after opening
    if (bExitLatch)
    {
        bExitLatch = false;
        return;
    }

    Super::HandleCancel();
    OnCancelClicked();
}

/* --------------------------------------------------------------------
   Button handlers
   -------------------------------------------------------------------- */

void UExitDlg::OnExitClicked()
{
    UE_LOG(LogExitDlg, Log, TEXT("Exit confirmed."));

    // MenuScreen owns modal state + Z-order; we just request exit:
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, true);
}

void UExitDlg::OnCancelClicked()
{
    UE_LOG(LogExitDlg, Log, TEXT("Exit canceled. Returning to menu."));

    // Hide ourselves so we don't eat input:
    SetVisibility(ESlateVisibility::Collapsed);
    SetDialogInputEnabled(false);

    if (MenuManager)
    {
        MenuManager->ShowMenuDlg();
    }
    else
    {
        UE_LOG(LogExitDlg, Warning, TEXT("MenuManager is null (MenuScreen didn't assign it)."));
    }
}
