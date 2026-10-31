
/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           KeyDlg.cpp
    AUTHOR:         Carlos Bott

    NOTES
    =====
    - UE-only key binding list with scrollbox
    - Shows authored Enhanced Input defaults and C++ mission controls
    - Pending remaps + clears override display until Apply
    - Value/key text uses (0.5,0.5,0.5,1.0)
=============================================================================*/

#include "KeyDlg.h"
#include "MissionUIStyle.h"
#include "StarshatterInputBindings.h"
#include "StarshatterControlsSettings.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "StarshatterInputLabels.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"
// UMG
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "GameStructs.h"

// Router
#include "OptionsScreen.h"

// Model + runtime
#include "StarshatterKeyboardSettings.h"
#include "StarshatterKeyboardSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogKeyDlg, Log, All);

UKeyDlg::UKeyDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
    bIsFocusable = true;
}

void UKeyDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // BaseScreen Enter/Escape routing:
    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegatesOnce();
}

void UKeyDlg::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyGlobalTheme(true, true, false); // buttons + text, no edits

    EnsureAutoVerticalBox();
    if (AutoVBox)
        AutoVBox->ClearChildren();
}

void UKeyDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindDelegatesOnce();
    BuildKeyRows();

    bCapturing = false;
    bHasSelection = false;
    PendingEnhanced.Reset(); PendingEnhancedClears.Reset();

    RefreshDisplayFromSettings();
    SetKeyboardFocus();
}

void UKeyDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    (void)MyGeometry;
    ExecFrame((double)InDeltaTime);
}

void UKeyDlg::ExecFrame(double /*DeltaTime*/)
{
    // UE-only: no polling
}

void UKeyDlg::BindDelegatesOnce()
{
    if (bDelegatesBound)
        return;

    if (ClearButton)
        ClearButton->OnClicked.AddUniqueDynamic(this, &UKeyDlg::OnClearClicked);

    if (ApplyBtn)
        ApplyBtn->OnClicked.AddUniqueDynamic(this, &UKeyDlg::OnApplyClicked);

    if (CancelBtn)
        CancelBtn->OnClicked.AddUniqueDynamic(this, &UKeyDlg::OnCancelClicked);

    bDelegatesBound = true;
}

// ------------------------------------------------------------
// UI build
// ------------------------------------------------------------

void UKeyDlg::BuildKeyRows()
{
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox || !WidgetTree)
    {
        UE_LOG(LogKeyDlg, Warning, TEXT("[KeyDlg] BuildKeyRows: AutoVBox/WidgetTree missing."));
        return;
    }

    VBox->ClearChildren();
    EnhancedRows.Reset(); BindingContexts.Reset(); CategoryBoxes.Reset();
    bHasSelection=false; bCapturing=false; SelectedEnhancedId.Reset();
    SelectedRowButton.Reset();

    // -------------------------
    // Scroll list
    // -------------------------
    KeyScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("KeyScrollBox"));
    KeyListVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("KeyListVBox"));

    if (KeyScrollBox && KeyListVBox)
    {
        KeyScrollBox->ClearChildren();
        KeyListVBox->ClearChildren();

        KeyScrollBox->AddChild(KeyListVBox);

        // Add to AutoVBox with stable height
        UHorizontalBox* ScrollRow = AddRow(TEXT("KeyScrollRow"));
        if (ScrollRow)
        {
            USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("KeyScrollSize"));
            if (SB)
            {
                SB->SetHeightOverride(520.f);
                SB->AddChild(KeyScrollBox);

                if (UHorizontalBoxSlot* S = ScrollRow->AddChildToHorizontalBox(SB))
                {
                    S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    S->SetHorizontalAlignment(HAlign_Fill);
                    S->SetVerticalAlignment(VAlign_Fill);
                }
            }
            else
            {
                if (UHorizontalBoxSlot* S = ScrollRow->AddChildToHorizontalBox(KeyScrollBox))
                {
                    S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    S->SetHorizontalAlignment(HAlign_Fill);
                    S->SetVerticalAlignment(VAlign_Fill);
                }
            }
        }

        BuildKeyList();
    }

    // Buttons row (optional)
    if (ClearButton || ApplyBtn || CancelBtn)
    {
        UHorizontalBox* BtnRow = AddRow(TEXT("KeyButtonsRow"));
        if (BtnRow)
        {
            if (ClearButton) BtnRow->AddChildToHorizontalBox(ClearButton);
            if (ApplyBtn)    BtnRow->AddChildToHorizontalBox(ApplyBtn);
            if (CancelBtn)   BtnRow->AddChildToHorizontalBox(CancelBtn);
        }
    }
}

void UKeyDlg::ApplyStandardValueTextStyle(UTextBlock* Block) const
{
    if (!Block)
        return;

    // Requested: 0.5, 0.5, 0.5, 1.0
    Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)));
    Block->SetJustification(ETextJustify::Right);
}

UVerticalBox* UKeyDlg::CategoryBox(const FString& Category)
{
    if (auto* Found=CategoryBoxes.Find(Category)) return Found->Get();
    auto* Header=WidgetTree->ConstructWidget<UTextBlock>();
    Header->SetText(FText::FromString(Category));
    Header->SetColorAndOpacity(FLinearColor(0.2f,0.85f,1.f));
    auto Font=Header->GetFont();Font.Size=20;Header->SetFont(Font);
    KeyListVBox->AddChildToVerticalBox(Header)->SetPadding(FMargin(0,18,0,6));
    auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();
    KeyListVBox->AddChildToVerticalBox(Box);
    CategoryBoxes.Add(Category,Box);return Box;
}

void UKeyDlg::BuildKeyList()
{
    if (!KeyListVBox || !WidgetTree) return;
    KeyListVBox->ClearChildren();CategoryBoxes.Reset();
    EnhancedRows.Reset();BindingContexts.Reset();
    bHasSelection=false;bCapturing=false;SelectedEnhancedId.Reset();
    auto* Help=WidgetTree->ConstructWidget<UTextBlock>();
    Help->SetText(FText::FromString(TEXT("SELECT A CONTROL TO REBIND. ESC CANCELS CAPTURE. APPLY SAVES CHANGES.")));
    Help->SetAutoWrapText(true);Help->SetColorAndOpacity(UITextColor);
    KeyListVBox->AddChildToVerticalBox(Help)->SetPadding(FMargin(0,0,0,8));
    const TCHAR* Categories[]={TEXT("MOVEMENT & FLIGHT"),TEXT("CAMERA & VIEWS"),TEXT("WEAPONS"),TEXT("TARGETING"),TEXT("SENSORS"),TEXT("NAVIGATION & SHIP SYSTEMS"),TEXT("HUD & PANELS"),TEXT("COMMUNICATIONS"),TEXT("MODIFIER KEYS"),TEXT("GENERAL")};
    for (const TCHAR* Category:Categories) CategoryBox(Category);
    AppendFighterHUDBindings();

}

void UKeyDlg::AppendFighterHUDBindings()
{
    TArray<UInputMappingContext*> Contexts;
    const TCHAR* Paths[]={TEXT("/Game/Input/IMC_Game.IMC_Game"),TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD")};
    for (const TCHAR* Path:Paths) if(auto* Context=LoadObject<UInputMappingContext>(nullptr,Path)) Contexts.AddUnique(Context);
    auto* Controls=UStarshatterControlsSettings::Get();
    const TSoftObjectPtr<UInputMappingContext> Models[]={Controls->GetArcadeContext(),Controls->GetFlightSimContext(),Controls->GetHybridContext()};
    for (const auto& Model:Models) if (!Model.IsNull()) if(auto* Context=Model.LoadSynchronous()) Contexts.AddUnique(Context);
    Contexts.Add(StarshatterInputBindings::SupplementalContext(this));
    for (auto* Context:Contexts)
    {
        BindingContexts.Add(Context);
        for (const auto& Mapping:Context->GetMappings())
        {
            if (!Mapping.Action) continue;
            const FName Name=Mapping.Action->GetFName();
            // Superseded camera actions are not bound by the mission camera controller.
            if(Name==TEXT("IA_ViewCockpit") || Name==TEXT("IA_ViewVirtualCockpit") ||
                Name==TEXT("IA_ViewChase") || Name==TEXT("IA_ViewOrbit") || Name==TEXT("IA_MouseZoom") || Name==TEXT("IA_GroundMode") || Name==TEXT("IA_HudMode")) continue;
            const FString Id=StarshatterInputBindings::Id(Mapping);
            if (EnhancedRows.Contains(Id)) continue;
            auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* Button=WidgetTree->ConstructWidget<UButton>();
            auto* Label=WidgetTree->ConstructWidget<UTextBlock>();
            auto* Key=WidgetTree->ConstructWidget<UTextBlock>();
            StyleButton_Default(Button,18);
            MissionUIStyle::ApplyActionButtonStyle(Button,TEXT("Blue"));
            FString Text=UStarshatterInputLabels::GetActionLabel(Mapping.Action).ToString().ToUpper();
            if(Mapping.Action->ValueType==EInputActionValueType::Axis1D && !Mapping.Key.IsAxis1D())
            {
                bool Negative=false;
                for(const auto& Modifier:Mapping.Modifiers)
                {
                    if(const auto* Negate=Cast<UInputModifierNegate>(Modifier)) if(Negate->bX) Negative=!Negative;
                    if(const auto* Scalar=Cast<UInputModifierScalar>(Modifier)) if(Scalar->Scalar.X<0) Negative=!Negative;
                }
                Text+=Negative?TEXT(" (NEGATIVE)"):TEXT(" (POSITIVE)");
            }

            Label->SetText(FText::FromString(Text));Label->SetColorAndOpacity(UITextColor);Label->SetAutoWrapText(true);
            Button->SetContent(Label);Button->OnPressed.AddUniqueDynamic(this,&UKeyDlg::OnRowButtonPressed);
            auto* RowSlot=Row->AddChildToHorizontalBox(Button);FSlateChildSize Size(ESlateSizeRule::Fill);Size.Value=3;RowSlot->SetSize(Size);RowSlot->SetPadding(FMargin(0,3,20,3));
            ApplyStandardValueTextStyle(Key);RowSlot=Row->AddChildToHorizontalBox(Key);RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));RowSlot->SetVerticalAlignment(VAlign_Center);
            CategoryBox(StarshatterInputBindings::Category(Mapping.Action->GetName()))->AddChildToVerticalBox(Row);
            FEnhancedRow Item;Item.Mapping=Mapping;Item.Button=Button;Item.KeyText=Key;EnhancedRows.Add(Id,Item);
        }
    }
}

void UKeyDlg::OnRowButtonPressed()
{
    for (const auto& Pair:EnhancedRows)
        if (Pair.Value.Button && Pair.Value.Button->IsPressed())
        {
            SelectedEnhancedId=Pair.Key;bHasSelection=true;SelectedRowButton=Pair.Value.Button;
            BeginCaptureForSelected();SetKeyboardFocus();return;
        }
}
void UKeyDlg::BeginCaptureForSelected()
{
    if (!bHasSelection) return;
    bCapturing=true;CaptureStarted=FPlatformTime::Seconds();CaptureMouseDelta=FVector2D::ZeroVector;
    RefreshDisplayFromSettings();
}
bool UKeyDlg::CaptureKey(FKey Key)
{
    if (!bCapturing || !bHasSelection || !Key.IsValid()) return false;
    if (SelectedEnhancedId.IsEmpty()) return false;
    PendingEnhanced.Add(SelectedEnhancedId,Key);PendingEnhancedClears.Remove(SelectedEnhancedId);
    bCapturing=false;RefreshDisplayFromSettings();return true;
}
FReply UKeyDlg::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if (Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::Virtual_Back)
    {
        if (bCapturing) {bCapturing=false;RefreshDisplayFromSettings();}
        else OnCancelClicked();
        return FReply::Handled();
    }
    return CaptureKey(Event.GetKey())?FReply::Handled():Super::NativeOnKeyDown(Geometry,Event);
}
FReply UKeyDlg::NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    return CaptureKey(Event.GetEffectingButton())?FReply::Handled():Super::NativeOnMouseButtonDown(Geometry,Event);
}
FReply UKeyDlg::NativeOnMouseWheel(const FGeometry& Geometry,const FPointerEvent& Event)
{
    FKey Key=Event.GetWheelDelta()>0?EKeys::MouseScrollUp:EKeys::MouseScrollDown;
    if (const auto* Row=EnhancedRows.Find(SelectedEnhancedId))
        if (Row->Mapping.Action && Row->Mapping.Action->ValueType!=EInputActionValueType::Boolean) Key=EKeys::MouseWheelAxis;
    return CaptureKey(Key)?FReply::Handled():Super::NativeOnMouseWheel(Geometry,Event);
}
FReply UKeyDlg::NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (bCapturing && FPlatformTime::Seconds()-CaptureStarted>0.35)
        if (const auto* Row=EnhancedRows.Find(SelectedEnhancedId))
            if (Row->Mapping.Action && Row->Mapping.Action->ValueType!=EInputActionValueType::Boolean)
            {
                CaptureMouseDelta+=Event.GetCursorDelta();
                if (CaptureMouseDelta.GetAbsMax()>30)
                    if (CaptureKey(FMath::Abs(CaptureMouseDelta.X)>FMath::Abs(CaptureMouseDelta.Y)?EKeys::MouseX:EKeys::MouseY)) return FReply::Handled();
            }
    return Super::NativeOnMouseMove(Geometry,Event);
}
FReply UKeyDlg::NativeOnAnalogValueChanged(const FGeometry& Geometry,const FAnalogInputEvent& Event)
{
    return FMath::Abs(Event.GetAnalogValue())>0.6f && CaptureKey(Event.GetKey())?FReply::Handled():Super::NativeOnAnalogValueChanged(Geometry,Event);
}
void UKeyDlg::HandleAccept() {OnApplyClicked();}
void UKeyDlg::HandleCancel() {OnCancelClicked();}
void UKeyDlg::OnClearClicked()
{
    if (!bHasSelection) return;
    PendingEnhanced.Remove(SelectedEnhancedId);PendingEnhancedClears.Add(SelectedEnhancedId);
    bCapturing=false;RefreshDisplayFromSettings();
}
void UKeyDlg::Apply() {CommitPendingToSettings();}
void UKeyDlg::Cancel()
{
    PendingEnhanced.Reset();PendingEnhancedClears.Reset();bCapturing=false;
    RefreshDisplayFromSettings();
}
void UKeyDlg::OnApplyClicked()
{
    CommitPendingToSettings();
    if (OptionsManager) OptionsManager->ReturnFromKeyDlg();else SetVisibility(ESlateVisibility::Collapsed);
}
void UKeyDlg::OnCancelClicked()
{
    Cancel();if (OptionsManager) OptionsManager->ReturnFromKeyDlg();else SetVisibility(ESlateVisibility::Collapsed);
}
void UKeyDlg::CommitPendingToSettings()
{
    auto* Settings=UStarshatterKeyboardSettings::Get();Settings->Load();
    auto Config=Settings->GetKeyboardConfig();
    for (const auto& Id:PendingEnhancedClears) Config.EnhancedBindings.Remove(Id);
    for (const auto& Pair:PendingEnhanced) Config.EnhancedBindings.Add(Pair.Key,Pair.Value);
    Settings->SetKeyboardConfig(Config);Settings->Save();
    if (auto* Subsystem=UStarshatterKeyboardSubsystem::Get(this)) Subsystem->ApplySettingsToRuntime(this);
    PendingEnhanced.Reset();PendingEnhancedClears.Reset();bCapturing=false;
    RefreshDisplayFromSettings();
}
void UKeyDlg::RefreshDisplayFromSettings()
{
    auto* Settings=UStarshatterKeyboardSettings::Get();Settings->Load();const auto& Config=Settings->GetKeyboardConfig();
    for (auto& Pair:EnhancedRows)
    {
        auto Mapping=Pair.Value.Mapping;
        if (!PendingEnhancedClears.Contains(Pair.Key))
        {
            if (const auto* Saved=Config.EnhancedBindings.Find(Pair.Key)) Mapping.Key=*Saved;
            if (const auto* Pending=PendingEnhanced.Find(Pair.Key)) Mapping.Key=*Pending;
        }
        FText KeyLabel=UStarshatterInputLabels::GetBindingKeyLabel(Mapping).ToUpper();
        if(Mapping.Action && (Mapping.Action->GetName()==TEXT("SSW_SensorRangeUp") || Mapping.Action->GetName()==TEXT("SSW_SensorRangeDown")))
            KeyLabel=FText::FromString(TEXT("SHIFT + ")+KeyLabel.ToString());
        Pair.Value.KeyText->SetText(bCapturing && SelectedEnhancedId==Pair.Key?FText::FromString(TEXT("PRESS A KEY...")):KeyLabel);
    }
}
FReply UKeyDlg::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    return CaptureKey(Event.GetEffectingButton())?FReply::Handled():Super::NativeOnPreviewMouseButtonDown(Geometry,Event);
}
