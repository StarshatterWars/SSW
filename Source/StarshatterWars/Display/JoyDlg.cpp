/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           JoyDlg.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    UJoyDlg - joystick axis mapping dialog (AutoVBox formatted).

    Notes:
    - Uses BaseScreen AutoVBox row building (like Audio/Game/Controls).
    - Delegates bound once via AddUniqueDynamic (no RemoveAll).
    - Enter/Escape routing handled via UBaseScreen ApplyButton/CancelButton.
    - Fixes axis detection warm-up gate so detection triggers reliably.
=============================================================================*/

#include "JoyDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"

// Layout helpers
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"

// Router
#include "OptionsScreen.h"

// Starshatter legacy
#include "Starshatter.h"
#include "KeyMap.h"
#include "Joystick.h"
#include "Game.h"

DEFINE_LOG_CATEGORY_STATIC(LogJoyDlg, Log, All);

// Legacy axis name tokens (for Game::GetText)
static const char* JoyAxisNames[] =
{
    "JoyDlg.axis.0",
    "JoyDlg.axis.1",
    "JoyDlg.axis.2",
    "JoyDlg.axis.3",
    "JoyDlg.axis.4",
    "JoyDlg.axis.5",
    "JoyDlg.axis.6",
    "JoyDlg.axis.7"
};

static void ForceTextIntoButton(UButton* Button, UTextBlock* TextWidget)
{
    if (!Button || !TextWidget)
        return;

    // If the text is currently parented somewhere else (like RootCanvas at 0,0), detach it:
    if (UPanelWidget* OldParent = Cast<UPanelWidget>(TextWidget->GetParent()))
        OldParent->RemoveChild(TextWidget);

    // UButton is a ContentWidget: it can only have ONE child.
    // If it already has something, you must replace it.
    Button->SetContent(TextWidget);
}

UJoyDlg::UJoyDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
}

void UJoyDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // BaseScreen Enter/Escape routing (works even if ApplyBtn/CancelBtn are null)
    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegates();
}

void UJoyDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    // Match AudioDlg pattern: ensure AutoVBox exists and clear runtime rows.
    if (UVerticalBox* VBox = EnsureAutoVerticalBox())
    {
        VBox->ClearChildren();
        VBox->SetVisibility(ESlateVisibility::Visible);
    }
}

void UJoyDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Re-assign in case BP bindings weren’t ready at OnInitialized time:
    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegates();

    // Build standardized rows into AutoVBox:
    BuildJoyRows();

    // Reset state
    SelectedAxis = -1;
    SampleAxis = -1;

    // Warm-up samples so first delta doesn't instantly "win"
    for (int32 i = 0; i < 8; ++i)
        Samples[i] = 10000000;

    if (MessageText)
        MessageText->SetText(FText::FromString(TEXT("SELECT AN AXIS SLOT, THEN MOVE A JOYSTICK AXIS")));

    RefreshAxisUIFromCurrentBindings();
}

void UJoyDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    (void)MyGeometry;
    ExecFrame((double)InDeltaTime);
}

void UJoyDlg::ExecFrame(double /*DeltaTime*/)
{
    // Only sample while user is selecting a slot:
    if (SelectedAxis < 0 || SelectedAxis >= 4)
        return;

    Joystick* Js = Joystick::GetInstance();
    if (!Js)
        return;

    Js->Acquire();

    int32 BestDelta = 1000;
    int32 BestAxis = -1;

    for (int32 i = 0; i < 8; ++i)
    {
        const int32 Raw = Joystick::ReadRawAxis(i + KEY_JOY_AXIS_X);
        const int32 Delta = FMath::Abs(Raw - Samples[i]);

        // FIX: warm-up gate must match initialization sentinel (10,000,000)
        if (Delta > BestDelta && Samples[i] < 10000000)
        {
            BestDelta = Delta;
            BestAxis = i;
        }

        Samples[i] = Raw;
    }

    if (BestAxis >= 0)
    {
        SampleAxis = BestAxis;
        MapAxis[SelectedAxis] = BestAxis;

        // Update label to detected axis name:
        const Text T = Game::GetText(JoyAxisNames[BestAxis]);
        SetAxisLabel(SelectedAxis, UTF8_TO_TCHAR(T.data()));

        // Stop capture after success:
        SelectedAxis = -1;

        if (MessageText)
            MessageText->SetText(FText::FromString(TEXT("AXIS DETECTED. SELECT ANOTHER SLOT OR APPLY.")));
    }
}

void UJoyDlg::BindFormWidgets() {}
FString UJoyDlg::GetLegacyFormText() const { return FString(); }

void UJoyDlg::HandleAccept() { OnApplyClicked(); }
void UJoyDlg::HandleCancel() { OnCancelClicked(); }

// ------------------------------------------------------------
// Show / Apply / Cancel
// ------------------------------------------------------------

void UJoyDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    // Rebuild rows if needed (safe) and refresh from current bindings:
    BuildJoyRows();
    RefreshAxisUIFromCurrentBindings();

    SetKeyboardFocus();
}

void UJoyDlg::Apply()
{
    CommitToKeyMap();
}

void UJoyDlg::Cancel()
{
    SelectedAxis = -1;
    RefreshAxisUIFromCurrentBindings();

    if (MessageText)
        MessageText->SetText(FText::FromString(TEXT("CHANGES CANCELLED.")));
}

// ------------------------------------------------------------
// Delegate wiring
// ------------------------------------------------------------

void UJoyDlg::BindDelegates()
{
    if (bDelegatesBound)
        return;

    // Apply/Cancel
    if (ApplyBtn)  ApplyBtn->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnApplyClicked);
    if (CancelBtn) CancelBtn->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnCancelClicked);

    // Axis slot clicks
    if (AxisButton0) AxisButton0->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnAxis0Clicked);
    if (AxisButton1) AxisButton1->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnAxis1Clicked);
    if (AxisButton2) AxisButton2->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnAxis2Clicked);
    if (AxisButton3) AxisButton3->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnAxis3Clicked);

    // Tabs (optional; only if present in WBP)
    if (AudTabButton) AudTabButton->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnAudioClicked);
    if (VidTabButton) VidTabButton->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnVideoClicked);
    if (CtlTabButton) CtlTabButton->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnControlsClicked);
    if (OptTabButton) OptTabButton->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnOptionsClicked);
    if (ModTabButton) ModTabButton->OnClicked.AddUniqueDynamic(this, &UJoyDlg::OnModClicked);

    bDelegatesBound = true;
}

// ------------------------------------------------------------
// AutoVBox row building (standardized formatting)
// ------------------------------------------------------------

void UJoyDlg::BuildJoyRows()
{
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox || !WidgetTree)
    {
        UE_LOG(LogJoyDlg, Warning, TEXT("[JoyDlg] BuildJoyRows: AutoVBox/WidgetTree missing."));
        return;
    }

    VBox->ClearChildren();

    // -----------------------------
    // FIX: Move axis text into buttons
    // -----------------------------
    AttachTextToButton(AxisButton0, AxisText0);
    AttachTextToButton(AxisButton1, AxisText1);
    AttachTextToButton(AxisButton2, AxisText2);
    AttachTextToButton(AxisButton3, AxisText3);

    // Status row
    if (MessageText)
    {
        UHorizontalBox* MsgRow = AddRow(TEXT("JoyMsgRow"));
        if (MsgRow)
        {
            MsgRow->AddChildToHorizontalBox(MessageText);
        }
    }

    // Axis rows
    BuildAxisRow(TEXT("YAW AXIS"), AxisButton0, Invert0, TEXT("JoyAxisRow0"));
    BuildAxisRow(TEXT("PITCH AXIS"), AxisButton1, Invert1, TEXT("JoyAxisRow1"));
    BuildAxisRow(TEXT("ROLL AXIS"), AxisButton2, Invert2, TEXT("JoyAxisRow2"));
    BuildAxisRow(TEXT("THROTTLE AXIS"), AxisButton3, Invert3, TEXT("JoyAxisRow3"));

    // Buttons row
    if (ApplyBtn || CancelBtn)
    {
        UHorizontalBox* BtnRow = AddRow(TEXT("JoyButtonsRow"));
        if (BtnRow)
        {
            if (ApplyBtn)  BtnRow->AddChildToHorizontalBox(ApplyBtn);
            if (CancelBtn) BtnRow->AddChildToHorizontalBox(CancelBtn);
        }
    }
}

void UJoyDlg::AttachTextToButton(UButton* Button, UTextBlock* TextWidget)
{
    if (!Button || !TextWidget)
        return;

    // If the text is currently sitting on the canvas (or anywhere else),
    // remove it from its old parent.
    if (UPanelWidget* OldParent = Cast<UPanelWidget>(TextWidget->GetParent()))
    {
        OldParent->RemoveChild(TextWidget);
    }

    // UButton is a ContentWidget (single child)
    Button->SetContent(TextWidget);
}

void UJoyDlg::BuildAxisRow(const FString& LabelText, UButton* AxisButton, UCheckBox* InvertCheck, const FName& RowName)
{
    if (!WidgetTree || !AxisButton)
        return;

    // Build a small "control group": [AxisButton][Invert]
    UHorizontalBox* ControlGroup = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!ControlGroup)
        return;

    // Button wrapper (stable width)
    {
        USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        if (BtnSize)
        {
            BtnSize->SetWidthOverride(360.f);   // tweak if you want a wider/narrower button
            BtnSize->SetHeightOverride(0.f);
            BtnSize->AddChild(AxisButton);

            if (UHorizontalBoxSlot* S = ControlGroup->AddChildToHorizontalBox(BtnSize))
            {
                S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                S->SetHorizontalAlignment(HAlign_Left);
                S->SetVerticalAlignment(VAlign_Center);
                S->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
            }
        }
        else
        {
            // Fallback: add button directly
            ControlGroup->AddChildToHorizontalBox(AxisButton);
        }
    }

    // Invert checkbox
    if (InvertCheck)
    {
        USizeBox* InvSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        if (InvSize)
        {
            InvSize->SetWidthOverride(120.f);
            InvSize->AddChild(InvertCheck);
            if (UHorizontalBoxSlot* S = ControlGroup->AddChildToHorizontalBox(InvSize))
            {
                S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                S->SetHorizontalAlignment(HAlign_Right);
                S->SetVerticalAlignment(VAlign_Center);
            }
        }
        else
        {
            ControlGroup->AddChildToHorizontalBox(InvertCheck);
        }
    }

    // Add the labeled row using BaseScreen helper (consistent typography + spacing)
    AddLabeledRow(LabelText, ControlGroup, 520.f);
}

// ------------------------------------------------------------
// Click handlers
// ------------------------------------------------------------

void UJoyDlg::OnAxis0Clicked() { HandleAxisClicked(0); }
void UJoyDlg::OnAxis1Clicked() { HandleAxisClicked(1); }
void UJoyDlg::OnAxis2Clicked() { HandleAxisClicked(2); }
void UJoyDlg::OnAxis3Clicked() { HandleAxisClicked(3); }

void UJoyDlg::HandleAxisClicked(int32 AxisIndex)
{
    if (AxisIndex < 0 || AxisIndex >= 4)
        return;

    // Clicking the same slot toggles capture off:
    if (SelectedAxis == AxisIndex)
    {
        SelectedAxis = -1;
        RefreshAxisUIFromCurrentBindings();

        if (MessageText)
            MessageText->SetText(FText::FromString(TEXT("CAPTURE CANCELLED.")));

        return;
    }

    SelectedAxis = AxisIndex;
    SampleAxis = -1;

    RefreshAxisUIFromCurrentBindings();
    SetAxisLabel(AxisIndex, TEXT("SELECT… THEN MOVE AXIS"));

    // Reset sampling window:
    for (int32 i = 0; i < 8; ++i)
        Samples[i] = 10000000;

    if (MessageText)
        MessageText->SetText(FText::FromString(TEXT("MOVE THE AXIS YOU WANT TO BIND…")));
}

void UJoyDlg::OnApplyClicked()
{
    // Let OptionsScreen orchestrate if present:
    if (OptionsManager)
        OptionsManager->ApplyOptions();
    else
        Apply();
}

void UJoyDlg::OnCancelClicked()
{
    if (OptionsManager)
        OptionsManager->CancelOptions();
    else
        Cancel();
}

// ------------------------------------------------------------
// Tabs (route to OptionsScreen hub)
// ------------------------------------------------------------

void UJoyDlg::OnAudioClicked() { if (OptionsManager) OptionsManager->ShowAudDlg(); }
void UJoyDlg::OnVideoClicked() { if (OptionsManager) OptionsManager->ShowVidDlg(); }
void UJoyDlg::OnControlsClicked() { if (OptionsManager) OptionsManager->ShowCtlDlg(); }
void UJoyDlg::OnOptionsClicked() { if (OptionsManager) OptionsManager->ShowGameDlg(); }
void UJoyDlg::OnModClicked() { if (OptionsManager) OptionsManager->ShowModDlg(); }

// ------------------------------------------------------------
// UI helpers
// ------------------------------------------------------------

void UJoyDlg::SetAxisLabel(int32 AxisIndex, const TCHAR* InText)
{
    const FString TextStr = InText ? FString(InText) : FString();

    // Preferred targets (explicit bindings)
    UTextBlock* TargetText = nullptr;
    UButton* TargetBtn = nullptr;

    switch (AxisIndex)
    {
    case 0: TargetText = AxisText0; TargetBtn = AxisButton0; break;
    case 1: TargetText = AxisText1; TargetBtn = AxisButton1; break;
    case 2: TargetText = AxisText2; TargetBtn = AxisButton2; break;
    case 3: TargetText = AxisText3; TargetBtn = AxisButton3; break;
    default: return;
    }

    // 1) If the bound text block exists, set it.
    if (TargetText)
    {
        TargetText->SetText(FText::FromString(TextStr));
        return;
    }

    // 2) Fallback: if the button exists, try to find a TextBlock inside it (common pattern).
    if (TargetBtn)
    {
        if (UWidget* Child = TargetBtn->GetChildAt(0))
        {
            if (UTextBlock* BtnText = Cast<UTextBlock>(Child))
            {
                BtnText->SetText(FText::FromString(TextStr));
                return;
            }
        }

        // 3) Last resort: search down the button widget tree for any TextBlock.
        // (Works if the button contains a panel with a text block.)
        TArray<UWidget*> Stack;
        Stack.Reserve(8);
        Stack.Add(TargetBtn);

        while (Stack.Num() > 0)
        {
            UWidget* W = Stack.Pop(false);
            if (!W) continue;

            if (UTextBlock* Found = Cast<UTextBlock>(W))
            {
                Found->SetText(FText::FromString(TextStr));
                return;
            }

            if (UPanelWidget* Panel = Cast<UPanelWidget>(W))
            {
                const int32 N = Panel->GetChildrenCount();
                for (int32 i = 0; i < N; ++i)
                    Stack.Add(Panel->GetChildAt(i));
            }
        }
    }

    // If we get here: you have no AxisText binding and the button has no text child.
    UE_LOG(LogJoyDlg, Warning, TEXT("[JoyDlg] SetAxisLabel: No target for Axis %d (Text='%s'). Check WBP AxisText%d binding or Button child text."),
        AxisIndex, *TextStr, AxisIndex);
}


void UJoyDlg::RefreshAxisUIFromCurrentBindings()
{
    // Pull from joystick legacy mapping:
    for (int32 i = 0; i < 4; ++i)
    {
        const int32 Map = Joystick::GetAxisMap(i) - KEY_JOY_AXIS_X;
        const int32 Inv = Joystick::GetAxisInv(i);

        if (Map >= 0 && Map < 8)
            MapAxis[i] = Map;
        else
            MapAxis[i] = -1;

        // Axis label:
        if (MapAxis[i] >= 0 && MapAxis[i] < 8)
        {
            const Text T = Game::GetText(JoyAxisNames[MapAxis[i]]);
            SetAxisLabel(i, UTF8_TO_TCHAR(T.data()));
        }
        else
        {
            SetAxisLabel(i, TEXT("UNMAPPED"));
        }

        // Invert:
        const bool bInv = (Inv != 0);
        if (i == 0 && Invert0) Invert0->SetIsChecked(bInv);
        if (i == 1 && Invert1) Invert1->SetIsChecked(bInv);
        if (i == 2 && Invert2) Invert2->SetIsChecked(bInv);
        if (i == 3 && Invert3) Invert3->SetIsChecked(bInv);
    }
}

// ------------------------------------------------------------
// Commit to legacy KeyMap
// ------------------------------------------------------------

void UJoyDlg::CommitToKeyMap()
{
    Starshatter* Stars = Starshatter::GetInstance();
    if (!Stars)
        return;

    KeyMap& KM = Stars->GetKeyMap();

    // Axis binds (slot->mapped axis)
    const int32 YawAxis = (MapAxis[0] >= 0) ? (MapAxis[0] + KEY_JOY_AXIS_X) : 0;
    const int32 PitchAxis = (MapAxis[1] >= 0) ? (MapAxis[1] + KEY_JOY_AXIS_X) : 0;
    const int32 RollAxis = (MapAxis[2] >= 0) ? (MapAxis[2] + KEY_JOY_AXIS_X) : 0;
    const int32 ThrottleAxis = (MapAxis[3] >= 0) ? (MapAxis[3] + KEY_JOY_AXIS_X) : 0;

    KM.Bind(KEY_AXIS_YAW, YawAxis, 0);
    KM.Bind(KEY_AXIS_PITCH, PitchAxis, 0);
    KM.Bind(KEY_AXIS_ROLL, RollAxis, 0);
    KM.Bind(KEY_AXIS_THROTTLE, ThrottleAxis, 0);

    const bool bInvYaw = Invert0 ? Invert0->IsChecked() : false;
    const bool bInvPitch = Invert1 ? Invert1->IsChecked() : false;
    const bool bInvRoll = Invert2 ? Invert2->IsChecked() : false;
    const bool bInvThrottle = Invert3 ? Invert3->IsChecked() : false;

    KM.Bind(KEY_AXIS_YAW_INVERT, bInvYaw ? 1 : 0, 0);
    KM.Bind(KEY_AXIS_PITCH_INVERT, bInvPitch ? 1 : 0, 0);
    KM.Bind(KEY_AXIS_ROLL_INVERT, bInvRoll ? 1 : 0, 0);
    KM.Bind(KEY_AXIS_THROTTLE_INVERT, bInvThrottle ? 1 : 0, 0);

    KM.SaveKeyMap("key.cfg", 256);
    Stars->MapKeys();

    UE_LOG(LogJoyDlg, Log, TEXT("[JoyDlg] Saved joystick bindings to key.cfg and remapped keys."));
}
