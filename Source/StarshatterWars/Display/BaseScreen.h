/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         BaseScreen.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UBaseScreen
    - Unreal base widget for Starshatter menu/dialog screens.
    - Provides FORM-style ID binding + helper ops (SetLabelText, SetVisible, etc).
    - Includes legacy .frm parse structures (form/ctrl/defctrl) and merge support.
    - Adds unified dialog input hooks: HandleAccept() / HandleCancel()
      so every dialog behaves identically (Enter/Escape).
    - Adds RichText support for legacy FORM type: text (URichTextBlock).
    - Adds FORM font mapping support (LegacyName -> UFont + size).
    - Adds slider active_color support (FORM: active_color).
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

// Input:
#include "Input/Reply.h"
#include "InputCoreTypes.h"

// Styling / Fonts:
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateTypes.h"
#include "Engine/Font.h"
#include "Styling/SlateBrush.h"

// UMG:
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/RichTextBlock.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ContentWidget.h"

#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

#include "GameStructs_UI.h"

#include "BaseScreen.generated.h"

// ====================================================================
//  LEGACY FORM PARSE TYPES
// ====================================================================

class UMenuScreen;
class USoundBase;

USTRUCT(BlueprintType)
struct FFormFontMapEntry
{
    GENERATED_BODY()

    // Legacy FORM font name, e.g. "Limerick12", "Verdana", "HUD", "GUI"
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString LegacyName;

    // Unreal font asset (UFont).
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UFont> Font = nullptr;

    // If bOverrideSize is true and Size > 0, force this size.
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Size = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bOverrideSize = false;
};

UENUM(BlueprintType)
enum class EFormCtrlType : uint8
{
    None,
    Label,
    Button,
    Image,
    Edit,
    Combo,
    List,
    Slider,
    Panel,
    Background,

    // Legacy "text" control (treated as RichText in Unreal):
    Text
};

UENUM(BlueprintType)
enum class EFormAlign : uint8
{
    None,
    Left,
    Center,
    Right
};

USTRUCT(BlueprintType)
struct FFormIntRect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 A = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 B = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 C = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 D = 0;

    FFormIntRect() = default;
    FFormIntRect(int32 InA, int32 InB, int32 InC, int32 InD)
        : A(InA), B(InB), C(InC), D(InD) {
    }
};

USTRUCT(BlueprintType)
struct FFormLayout
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> XMins;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<float> XWeights;

    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> YMins;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<float> YWeights;

    bool IsEmpty() const
    {
        return XMins.Num() == 0 && XWeights.Num() == 0 && YMins.Num() == 0 && YWeights.Num() == 0;
    }
};

USTRUCT(BlueprintType)
struct FFormColumnDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Title;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Width = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFormAlign Align = EFormAlign::Left;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Sort = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Color = FLinearColor::Transparent;
};

USTRUCT(BlueprintType)
struct FParsedCtrl
{
    GENERATED_BODY()

    // ---------------- Identity / hierarchy ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Id = -1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ParentId = -1; // pid:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFormCtrlType Type = EFormCtrlType::None;

    // ---------------- Visuals / text ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Text;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Texture;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Font;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFormAlign Align = EFormAlign::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor BackColor = FLinearColor::Transparent;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor ForeColor = FLinearColor::Transparent;

    // Slider colors (legacy):
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor ActiveColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bTransparent = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bSticky = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bBorder = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHidePartial = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bShowHeadings = false;

    // ---------------- Images / bevel ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString StandardImage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ActivatedImage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString TransitionImage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BevelWidth = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BevelDepth = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor BorderColor = FLinearColor::Transparent;

    // ---------------- Sizing ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FixedWidth = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FixedHeight = 0;

    // ---------------- Grid placement ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFormIntRect Cells;       // (x,y,w,h)
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFormIntRect CellInsets;  // (l,t,r,b)

    // ---------------- Panel/list margins ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFormIntRect Margins;     // (l,t,r,b)

    // ---------------- List style ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ScrollBar = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Style = 0;

    // ---------------- Per-ctrl layout block ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFormLayout Layout;

    // ---------------- List columns ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FFormColumnDef> Columns;

    // -----------------------------------------------------------------
    // Internal "explicitness" flags (NOT UPROPERTY; not for Blueprint).
    // Used to merge defctrl defaults correctly in BaseScreen.cpp.
    // -----------------------------------------------------------------
    bool bHasText = false;
    bool bHasTexture = false;
    bool bHasFont = false;
    bool bHasAlign = false;
    bool bHasBackColor = false;
    bool bHasForeColor = false;
    bool bHasActiveColor = false;

    bool bHasTransparent = false;
    bool bHasSticky = false;
    bool bHasBorder = false;
    bool bHasHidePartial = false;
    bool bHasShowHeadings = false;

    bool bHasStandardImage = false;
    bool bHasActivatedImage = false;
    bool bHasTransitionImage = false;

    bool bHasBevelWidth = false;
    bool bHasBevelDepth = false;
    bool bHasBorderColor = false;

    bool bHasFixedWidth = false;
    bool bHasFixedHeight = false;

    bool bHasCells = false;
    bool bHasCellInsets = false;
    bool bHasMargins = false;

    bool bHasScrollBar = false;
    bool bHasStyle = false;
    bool bHasLayout = false;
};

USTRUCT(BlueprintType)
struct FParsedForm
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor BackColor = FLinearColor::Black;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor ForeColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Font;

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Texture;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFormIntRect Margins;

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFormLayout Layout;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FParsedCtrl> Controls;
};

// ====================================================================
//  OPTIONAL COMPILED FORM DEF TYPES (DOCUMENTATION / FUTURE USE)
// ====================================================================

USTRUCT(BlueprintType)
struct FFormControlDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Id = -1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFormCtrlType Type = EFormCtrlType::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Text;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Texture;

    // Legacy grid info (documentation / future auto-layout)
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CellX = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CellY = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CellW = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CellH = 1;
};

USTRUCT(BlueprintType)
struct FFormDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Name = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FFormControlDef> Controls;
};

// ====================================================================
//  WIDGET MAP (ID -> UMG WIDGET)
// ====================================================================

USTRUCT()
struct FFormWidgetMap
{
    GENERATED_BODY()

    UPROPERTY(Transient) TMap<int32, TObjectPtr<UTextBlock>>       Labels;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<URichTextBlock>>   Texts;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UButton>>          Buttons;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UImage>>           Images;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UEditableTextBox>> Edits;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UComboBoxString>>  Combos;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UListView>>        Lists;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<USlider>>          Sliders;

    void Reset()
    {
        Labels.Reset();
        Texts.Reset();
        Buttons.Reset();
        Images.Reset();
        Edits.Reset();
        Combos.Reset();
        Lists.Reset();
        Sliders.Reset();
    }
};

// ====================================================================
//  UBaseScreen
// ====================================================================

UCLASS()
class STARSHATTERWARS_API UBaseScreen : public UUserWidget
{
    GENERATED_BODY()

public:
    UBaseScreen(const FObjectInitializer& ObjectInitializer);

    virtual void ExecFrame(double DeltaTime);
    
    // Visibility / flow:
    virtual bool IsShown()  const { return bIsShown; }
    virtual void Show();
    virtual void Hide();
    virtual void HideAll();

    // ----------------------------------------------------------------
    // Optional common widgets (bind if your UMG has them)
    // ----------------------------------------------------------------
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Title = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CancelButtonText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ApplyButtonText = nullptr;

    // Used by HandleAccept / HandleCancel (optional)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ApplyButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MenuButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelButton = nullptr; 
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AddPlayerButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeletePlayerButton = nullptr;

    // ----------------------------------------------------------------
    // FORM-style API (override per-screen as needed)
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() {}
    virtual const FFormDef* GetFormDef() const { return nullptr; }
    virtual FString GetLegacyFormText() const { return FString(); }

    // ----------------------------------------------------------------
    // Dialog input hooks (Enter/Escape)
    // Override in derived dialogs.
    // ----------------------------------------------------------------
    virtual void HandleAccept();
    virtual void HandleCancel();

    UFUNCTION(BlueprintCallable, Category = "Dialog")
    void SetDialogInputEnabled(bool bEnabled);

    // ----------------------------------------------------------------
    // Fonts: LegacyName -> UFont (+ optional size override)
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "FORM|Fonts")
    TArray<FFormFontMapEntry> FontMappings;

    UPROPERTY(EditAnywhere, Category = "FORM|Fonts")
    TObjectPtr<UFont> DefaultFont = nullptr;

    UPROPERTY(EditAnywhere, Category = "FORM|Fonts")
    int32 DefaultFontSize = 12;



protected:
    // Font setup (load *_Font assets)
    void EnsureThemeFontsLoaded();
    UFont* GetThemeFont(ESSWThemeFont Which) const;

    void ApplyGlobalTheme(bool bStyleButtons, bool bStyleText, bool bStyleEdits);

    void StyleButton_Default(
        UButton* Button,
        int32 FontSize,
        const FLinearColor* OverrideTextColor = nullptr,
        UFont* OverrideFont = nullptr
    );
    
    void StyleText_Default(UTextBlock* Text, int32 FontSize);

    void StyleEdit_Default(UEditableTextBox* Edit, int32 FontSize = 18);
    void ApplyDefaultEditBoxStyle(UEditableTextBox* Edit, int32 FontSize = 18) const;

    // Widget tree traversal (no lambdas)
    void GatherButtonsRecursive(UWidget* Root, TArray<UButton*>& OutButtons);
    void GatherTextBlocksRecursive(UWidget* Root, TArray<UTextBlock*>& OutTexts);
    void GatherEditableTextBoxesRecursive(UWidget* Root, TArray<UEditableTextBox*>& OutEdits);
    UTextBlock* FindFirstTextBlockRecursive(const UWidget* Root) const;

protected:
    // Fonts (loaded from /Game/Font/*_Font assets)
    UPROPERTY(Transient) TObjectPtr<UFont> Font_LimerickBold = nullptr;
    UPROPERTY(Transient) TObjectPtr<UFont> Font_VerdanaItalic = nullptr;
    UPROPERTY(Transient) TObjectPtr<UFont> Font_Serpntb = nullptr;

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|ButtonSkin")
    TObjectPtr<UTexture2D> Btn_NormalTex = nullptr;

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|ButtonSkin")
    TObjectPtr<UTexture2D> Btn_HoverTex = nullptr;

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|ButtonSkin")
    TObjectPtr<UTexture2D> Btn_PressedTex = nullptr;

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|ButtonSkin")
    TObjectPtr<UTexture2D> Btn_DisabledTex = nullptr;

    // 9-slice margin in normalized UV space (0..1). Start around 0.25 if your bevel is thick.
    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|ButtonSkin")
    FMargin Btn_9SliceMargin = FMargin(0.25f, 0.25f, 0.25f, 0.25f);

    // Optional: fixed image size hint (can be zero; Slate can infer)
    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|ButtonSkin")
    FVector2D Btn_ImageSize = FVector2D(0.f, 0.f);
public:
    // Theme knobs (general defaults)
    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|Theme")
    ESSWThemeFont DefaultUIFont = ESSWThemeFont::LimerickBold;

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|Theme")
    FLinearColor UIButtonFill = FLinearColor(0.18f, 0.18f, 0.18f, 1.f);

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|Theme")
    FLinearColor UIButtonFillHover = FLinearColor(0.22f, 0.22f, 0.22f, 1.f);

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|Theme")
    FLinearColor UIButtonFillPressed = FLinearColor(0.14f, 0.14f, 0.14f, 1.f);

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|Theme")
    FLinearColor UIButtonBorder = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|Theme")
    float UIButtonBorderWidth = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Starshatter|UI|Theme")
    FLinearColor UITextColor = FLinearColor::Black;



protected:
    // ----------------------------------------------------------------
    // Binding helpers (call from BindFormWidgets)
    // ----------------------------------------------------------------
    void BindLabel(int32 Id, UTextBlock* Widget);
    void BindText(int32 Id, URichTextBlock* Widget);
    void BindButton(int32 Id, UButton* Widget);
    void BindImage(int32 Id, UImage* Widget);
    void BindEdit(int32 Id, UEditableTextBox* Widget);
    void BindCombo(int32 Id, UComboBoxString* Widget);
    void BindList(int32 Id, UListView* Widget);
    void BindSlider(int32 Id, USlider* Widget);

protected:
    // ----------------------------------------------------------------
    // Operations helpers
    // ----------------------------------------------------------------
    void SetLabelText(int32 Id, const FText& Text);
    void SetEditText(int32 Id, const FText& Text);
    void SetVisible(int32 Id, bool bVisible);
    void SetEnabled(int32 Id, bool bEnabled);
   

protected:
    // ----------------------------------------------------------------
    // Lookup helpers
    // ----------------------------------------------------------------
    UTextBlock* GetLabel(int32 Id) const;
    URichTextBlock* GetText(int32 Id) const;
    UButton* GetButton(int32 Id) const;
    UImage* GetImage(int32 Id) const;
    UEditableTextBox* GetEdit(int32 Id) const;
    UComboBoxString* GetCombo(int32 Id) const;
    UListView* GetList(int32 Id) const;
    USlider* GetSlider(int32 Id) const;
    UCanvasPanel* ResolveRootCanvas();

protected:
    // ----------------------------------------------------------------
    // Defaults application
    // ----------------------------------------------------------------
    void ApplyFormDefaults();

protected:
    // ----------------------------------------------------------------
    // Legacy FORM parse + defaults application (supports defctrl)
    // ----------------------------------------------------------------
    bool ParseLegacyForm(const FString& InText, FParsedForm& OutForm, FString& OutError) const;
    void ApplyLegacyFormDefaults(const FParsedForm& Parsed);

protected:
    // ----------------------------------------------------------------
    // Optional mapping hooks
    // ----------------------------------------------------------------
    virtual bool ResolveFont(const FString& InFontName, FSlateFontInfo& OutFont) const;
    virtual bool ResolveTextJustification(EFormAlign InAlign, ETextJustify::Type& OutJustify) const;

protected:
    // ----------------------------------------------------------------
    // UUserWidget lifecycle
    // ----------------------------------------------------------------
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Screen")
    TObjectPtr<class UMenuScreen> MenuManager;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Screen")
    TObjectPtr<class UOptionsScreen> OptionsManager;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Screen")
    TObjectPtr<class UGameScreen> GameManager;


    protected:
        // ====================================================================
        //  UI STYLE / FONT CACHE (UMG-friendly *_Font assets)
        // ====================================================================

        // Call once (NativeConstruct does it) — safe to call multiple times
        void EnsureUiFontsLoaded();

        // Accessors (optional)
        UFont* GetFont_LimerickBold() const { return Font_LimerickBold; }
        UFont* GetFont_VerdanaItalic() const { return Font_VerdanaItalic; }
        UFont* GetFont_SerpentBold() const { return Font_Serpntb; }

        // Styling helpers (standard method calls; no lambdas)
        void ApplyDefaultTextStyle(UTextBlock* Text, int32 FontSize = 18) const;
        void ApplyTitleTextStyle(UTextBlock* Text, int32 FontSize = 26) const;

        // Convenience: apply to all TextBlocks under this widget tree
        void ApplyDefaultTextStyle_AllTextBlocks(int32 FontSize = 18) const;
        void ApplyDefaultButtonStyle(UButton* Button, int32 FontSize = 20) const;

protected:
    // --------------------------------------------------------------------
    // Font asset references (MUST be /Game/ paths; use "Copy Reference")
    // --------------------------------------------------------------------
    static const TCHAR* PATH_Font_LimerickBold;
    static const TCHAR* PATH_Font_VerdanaItalic;
    static const TCHAR* PATH_Font_SerpentBold;
public:
    /* ----------------------------------------------------------------
       Setters
       ---------------------------------------------------------------- */

    UFUNCTION(BlueprintCallable, Category = "Screen")
    void SetMenuManager(class UMenuScreen* InManager)
    {
        MenuManager = InManager;
    }

    UFUNCTION(BlueprintCallable, Category = "Screen")
    void SetGameManager(class UGameScreen* InManager)
    {
        GameManager = InManager;
    }

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Starshatter|Options")
    void SetOptionsManager(UOptionsScreen* InManager);

    virtual void SetOptionsManager_Implementation(UOptionsScreen* InManager);

    static UVerticalBox* EnsureAutoVBoxFill(
        UUserWidget* Owner,
        UCanvasPanel* RootCanvas,
        const FName SizeBoxName = TEXT("AutoRootSizeBox"),
        const FName VBoxName = TEXT("AutoRootVBox"))
    {
        if (!Owner || !RootCanvas)
            return nullptr;

        // Correct lookup (WidgetTree owns named widgets)
        if (UWidget* Existing = Owner->WidgetTree->FindWidget(VBoxName))
        {
            return Cast<UVerticalBox>(Existing);
        }

        // Create SizeBox + VBox
        USizeBox* SizeBox = Owner->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), SizeBoxName);
        UVerticalBox* VBox = Owner->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), VBoxName);

        SizeBox->AddChild(VBox);
        RootCanvas->AddChild(SizeBox);

        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SizeBox->Slot))
        {
            CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            CanvasSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
            CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
        }

        return VBox;
    }

    static void ApplyVBoxSpacing(UVerticalBox* VBox, float TopPad = 64.f, float BetweenPad = 32.f)
    {
        if (!VBox) return;

        const int32 Count = VBox->GetChildrenCount();
        for (int32 i = 0; i < Count; ++i)
        {
            UWidget* Child = VBox->GetChildAt(i);
            if (!Child) continue;

            if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Child->Slot))
            {
                const float ThisTop = (i == 0) ? TopPad : BetweenPad;

                FMargin P = Slot->GetPadding();
                P.Top = ThisTop;

                Slot->SetPadding(P);
                Slot->SetHorizontalAlignment(HAlign_Fill);
                // Only set Fill if you truly want each row to expand vertically:
                // Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            }
        }
    }
    /* ----------------------------------------------------------------
       Getters (optional)
       ---------------------------------------------------------------- */

    UMenuScreen* GetMenuManager() const { return MenuManager; }
    UOptionsScreen* GetOptionsManager() const { return OptionsManager; }
    UGameScreen* GetGameManager() const { return GameManager; }

    /* --------------------------------------------------------------------
      Dialog Input
      - Enabled:   Visible + hit-testable
      - Disabled:  Visible but NOT hit-testable (so overlays can sit above it)
      NOTE: Never disables widget tree (avoids greying out everything).
      -------------------------------------------------------------------- */

    UFUNCTION(BlueprintCallable, Category = "Screen")
    bool IsDialogInputEnabled() const { return bDialogInputEnabled; }

protected:
    // ----------------------------------------------------------------
    // Centralized key handling (Enter/Escape)
    // ----------------------------------------------------------------
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;


protected:
    // ----------------------------------------------------------------
    // Dialog policy
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "Dialog")
    bool bDialogInputEnabled = true;

    // If you need the old "exit latch" behavior, flip true on show and clear next tick.
    UPROPERTY(Transient)
    bool bExitLatch = false;

protected:
    // ----------------------------------------------------------------
    // Sub Panel Setup
    // ----------------------------------------------------------------

    // Root canvas in each dialog (BindWidgetOptional so old pages don’t explode)
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

    // Cached runtime container (not serialized)
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> AutoVBox = nullptr;

    // Creates/reuses a VBox named "AutoVBox" attached to RootCanvas.
    // Returns nullptr if RootCanvas/WidgetTree are missing.
    UVerticalBox* EnsureAutoVerticalBox();

    // Optional: tweak margins per screen if needed
    UVerticalBox* EnsureAutoVerticalBoxWithOffsets(const FMargin& Offsets);

    UHorizontalBox* AddRow(const FName& RowName);

    UHorizontalBox* AddLabeledControlRow(
        const FName& RowName,
        const FText& LabelText,
        UWidget* ControlWidget,
        UTextBlock** OutLabel = nullptr,
        float LabelMinWidth = 260.0f,
        float RowHeight = 0.0f
    );

    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI")
    UHorizontalBox* AddLabeledRow(
        const FString& LabelText,
        UWidget* Control,
        float ControlWidth = 500.f,
        float ControlHeight = 32.f,
        float RowPaddingY = 8.f,
        float LabelRightPad = 16.f
    );

    // Utility to create a TextBlock (so you don’t have to include TextBlock in every dlg)
    UTextBlock* MakeLabelText(const FName& Name, const FText& Text) const;


protected:
    // ----------------------------------------------------------------
    // Internal state
    // ----------------------------------------------------------------
    UPROPERTY(Transient) FFormWidgetMap FormMap;
    UPROPERTY(Transient) FParsedForm ParsedForm;

 protected:
        // ----------------------------------------------------------------
        // UI SFX (Hover/Click)
        // ----------------------------------------------------------------
        UPROPERTY(EditAnywhere, Category = "Starshatter|UI|SFX")
        bool bEnableButtonSfx = true;

        // Played when mouse goes over a button (OnHovered)
        UPROPERTY(EditAnywhere, Category = "Starshatter|UI|SFX")
        TObjectPtr<USoundBase> ButtonHoverSfx = nullptr;

        // Played when button is clicked (OnClicked)
        UPROPERTY(EditAnywhere, Category = "Starshatter|UI|SFX")
        TObjectPtr<USoundBase> ButtonClickSfx = nullptr;

        // Optional volume scalar for 2D playback (keeps tuning centralized)
        UPROPERTY(EditAnywhere, Category = "Starshatter|UI|SFX", meta = (ClampMin = "0.0"))
        float ButtonSfxVolume = 1.0f;

protected:
    void HookAllButtonSfx();
    void HookButtonSfx(UButton* Button);
    void PlayButtonSfx(USoundBase* Sound) const;

private:
    // Guards against double-binding during rebuilds
    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UButton>> SfxBoundButtons;

private:
    UPROPERTY()
    bool  bIsShown;
    
};


