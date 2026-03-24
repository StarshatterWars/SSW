/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           VideoDlg.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    UVideoDlg

    Notes:
    - Delegates are bound once via AddUniqueDynamic (no RemoveAll needed).
    - OptionsScreen is the router; this page forwards if OptionsManager exists.
    - Enter/Escape routing is handled via UBaseScreen (ApplyButton/CancelButton)
      plus HandleAccept/HandleCancel overrides.
    - AutoVBox labeled rows are built using UBaseScreen helpers (AddLabeledRow).

=============================================================================*/

#include "VideoDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"

// UMG layout helpers
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"

#include "Components/ScrollBox.h"
#include "Components/CanvasPanelSlot.h"

// Model
#include "StarshatterVideoSettings.h"

// Runtime apply
#include "StarshatterVideoSubsystem.h"

// Router (cpp-only include to avoid header coupling)
#include "OptionsScreen.h"

DEFINE_LOG_CATEGORY_STATIC(LogVideoDlg, Log, All);

static void EnsureSelected(UComboBoxString* Combo, int32 DefaultIndex = 0)
{
    if (!Combo) return;
    if (Combo->GetOptionCount() <= 0) return;

    if (Combo->GetSelectedIndex() < 0)
        Combo->SetSelectedIndex(FMath::Clamp(DefaultIndex, 0, Combo->GetOptionCount() - 1));
}

UVideoDlg::UVideoDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
}

void UVideoDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // If UBaseScreen supports these pointers, set them once:
    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegates();
}

void UVideoDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    UE_LOG(LogVideoDlg, Warning, TEXT("[VideoDlg] PreConstruct RootCanvas=%s WidgetTree=%s"),
        RootCanvas ? TEXT("VALID") : TEXT("NULL"),
        WidgetTree ? TEXT("VALID") : TEXT("NULL"));

    // Build/ensure AutoVBox (BaseScreen helper)
    UVerticalBox* VBox = EnsureAutoVerticalBox();

    // Ensure runtime scroll container (no BP requirement)
    UScrollBox* Scroll = EnsureContentScrollBox();

    // Make Scroll host AutoVBox
    if (Scroll && VBox)
    {
        if (VBox->GetParent())
            VBox->RemoveFromParent();

        Scroll->ClearChildren();
        Scroll->AddChild(VBox);
    }

    if (VBox)
        VBox->ClearChildren();
}

void UVideoDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindDelegates();

    UVerticalBox* VBox = EnsureAutoVerticalBox();
    UScrollBox* Scroll = EnsureContentScrollBox();

    if (!VBox || !Scroll)
    {
        UE_LOG(LogVideoDlg, Error, TEXT("[VideoDlg] Missing VBox or Scroll after Ensure. VBox=%s Scroll=%s"),
            VBox ? TEXT("VALID") : TEXT("NULL"),
            Scroll ? TEXT("VALID") : TEXT("NULL"));
        return;
    }

    // Ensure Scroll owns VBox (in case Construct runs again)
    if (VBox->GetParent())
        VBox->RemoveFromParent();

    Scroll->ClearChildren();
    Scroll->AddChild(VBox);

    VBox->SetVisibility(ESlateVisibility::Visible);

    BuildListsIfNeeded();
    BuildVideoRows();

    UE_LOG(LogVideoDlg, Warning, TEXT("[VideoDlg] AutoVBox children after BuildVideoRows: %d"),
        VBox->GetChildrenCount());

    if (bClosed)
    {
        RefreshFromModel();
        bClosed = false;
        bDirty = false;
    }
}

void UVideoDlg::BindDelegates()
{
    if (bDelegatesBound)
        return;

    // Apply/Cancel
    if (ApplyBtn)  ApplyBtn->OnClicked.AddUniqueDynamic(this, &UVideoDlg::OnApplyClicked);
    if (CancelBtn) CancelBtn->OnClicked.AddUniqueDynamic(this, &UVideoDlg::OnCancelClicked);

    // Tabs (optional)
    if (AudTabButton) AudTabButton->OnClicked.AddUniqueDynamic(this, &UVideoDlg::OnAudioClicked);
    if (VidTabButton) VidTabButton->OnClicked.AddUniqueDynamic(this, &UVideoDlg::OnVideoClicked);
    if (CtlTabButton) CtlTabButton->OnClicked.AddUniqueDynamic(this, &UVideoDlg::OnControlsClicked);
    if (OptTabButton) OptTabButton->OnClicked.AddUniqueDynamic(this, &UVideoDlg::OnGameClicked);
    if (ModTabButton) ModTabButton->OnClicked.AddUniqueDynamic(this, &UVideoDlg::OnModClicked);

    // Combos
    if (ModeCombo)       ModeCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnModeChanged);
    if (TexSizeCombo)    TexSizeCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnTexSizeChanged);
    if (DetailCombo)     DetailCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnDetailChanged);
    if (TextureCombo)    TextureCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnTextureChanged);

    if (LensFlareCombo)  LensFlareCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnLensFlareChanged);
    if (CoronaCombo)     CoronaCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnCoronaChanged);
    if (NebulaCombo)     NebulaCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnNebulaChanged);
    if (DustCombo)       DustCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnDustChanged);

    if (ShadowsCombo)    ShadowsCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnShadowsChanged);
    if (SpecMapsCombo)   SpecMapsCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnSpecMapsChanged);
    if (BumpMapsCombo)   BumpMapsCombo->OnSelectionChanged.AddUniqueDynamic(this, &UVideoDlg::OnBumpMapsChanged);

    // Slider
    if (GammaSlider)     GammaSlider->OnValueChanged.AddUniqueDynamic(this, &UVideoDlg::OnGammaChanged);

    bDelegatesBound = true;
}

void UVideoDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame((double)InDeltaTime);
}

void UVideoDlg::ExecFrame(double /*DeltaTime*/)
{
    // UE-only: no polling required.
}

void UVideoDlg::BindFormWidgets() {}
FString UVideoDlg::GetLegacyFormText() const { return FString(); }

void UVideoDlg::HandleAccept() { OnApplyClicked(); }
void UVideoDlg::HandleCancel() { OnCancelClicked(); }

void UVideoDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    if (bClosed)
    {
        RefreshFromModel();
        bClosed = false;
        bDirty = false;
    }
}

// ---------------- Model ----------------

UStarshatterVideoSettings* UVideoDlg::GetVideoSettings() const
{
    return GetMutableDefault<UStarshatterVideoSettings>();
}

UStarshatterVideoSubsystem* UVideoDlg::GetVideoSubsystem() const
{
    return UStarshatterVideoSubsystem::Get(this);
}

static bool ComboOn(UComboBoxString* Combo)
{
    return Combo && Combo->GetSelectedIndex() == 1;
}

void UVideoDlg::RefreshFromModel()
{
    UStarshatterVideoSettings* S = GetVideoSettings();
    if (!S)
        return;

    S->Load();
    const FStarshatterVideoConfig& C = S->GetConfig();

    Width = C.Width;
    Height = C.Height;
    bFullscreen = C.bFullscreen;

    MaxTextureSize = C.MaxTextureSize;
    GammaLevel = C.GammaLevel;

    bShadows = C.bShadows;
    bSpecularMaps = C.bSpecularMaps;
    bBumpMaps = C.bBumpMaps;

    bLensFlare = C.bLensFlare;
    bCorona = C.bCorona;
    bNebula = C.bNebula;

    DustLevel = C.DustLevel;
    TerrainDetailIndex = C.TerrainDetailIndex;
    bTerrainTextures = C.bTerrainTextures;

    if (GammaSlider)  GammaSlider->SetValue(Gamma01FromLevel(GammaLevel));
    if (TexSizeCombo) TexSizeCombo->SetSelectedIndex(TexSizeIndexFromPow2(MaxTextureSize));
    if (DetailCombo)  DetailCombo->SetSelectedIndex(FMath::Clamp(TerrainDetailIndex, 0, 3));
    if (TextureCombo) TextureCombo->SetSelectedIndex(bTerrainTextures ? 1 : 0);

    if (ShadowsCombo)  ShadowsCombo->SetSelectedIndex(bShadows ? 1 : 0);
    if (SpecMapsCombo) SpecMapsCombo->SetSelectedIndex(bSpecularMaps ? 1 : 0);
    if (BumpMapsCombo) BumpMapsCombo->SetSelectedIndex(bBumpMaps ? 1 : 0);

    if (LensFlareCombo) LensFlareCombo->SetSelectedIndex(bLensFlare ? 1 : 0);
    if (CoronaCombo)    CoronaCombo->SetSelectedIndex(bCorona ? 1 : 0);
    if (NebulaCombo)    NebulaCombo->SetSelectedIndex(bNebula ? 1 : 0);

    if (DustCombo) DustCombo->SetSelectedIndex(FMath::Clamp(DustLevel, 0, 3));

    if (ModeCombo)
        ModeCombo->SetSelectedOption(FString::Printf(TEXT("%dx%d"), Width, Height));
}

void UVideoDlg::PushToModel(bool bApplyRuntimeToo)
{
    UStarshatterVideoSettings* S = GetVideoSettings();
    if (!S)
        return;

    FStarshatterVideoConfig C = S->GetConfig();

    C.Width = Width;
    C.Height = Height;
    C.bFullscreen = bFullscreen;

    C.MaxTextureSize = MaxTextureSize;
    C.GammaLevel = ClampGammaLevel(GammaLevel);

    C.bShadows = bShadows;
    C.bSpecularMaps = bSpecularMaps;
    C.bBumpMaps = bBumpMaps;

    C.bLensFlare = bLensFlare;
    C.bCorona = bCorona;
    C.bNebula = bNebula;

    C.DustLevel = FMath::Clamp(DustLevel, 0, 3);
    C.TerrainDetailIndex = FMath::Clamp(TerrainDetailIndex, 0, 3);
    C.bTerrainTextures = bTerrainTextures;

    S->SetConfig(C);
    S->Save();

    if (bApplyRuntimeToo)
    {
        if (UStarshatterVideoSubsystem* VideoSS = GetVideoSubsystem())
            VideoSS->ApplySettingsToRuntime();
    }

    bDirty = false;
}

// ---------------- Apply / Cancel ----------------

void UVideoDlg::Apply()
{
    if (bClosed)
        return;

    PushToModel(true);
    bClosed = true;
}

void UVideoDlg::Cancel()
{
    RefreshFromModel();
    bDirty = false;
    bClosed = true;
}

// ---------------- Lists ----------------

const TArray<int32>& UVideoDlg::GetTexSizeOptions()
{
    static TArray<int32> Options = { 256, 512, 1024, 2048, 4096, 8192, 16384 };
    return Options;
}

int32 UVideoDlg::TexSizeIndexFromPow2(int32 Pow2)
{
    const TArray<int32>& Opt = GetTexSizeOptions();
    for (int32 i = 0; i < Opt.Num(); ++i)
    {
        if (Opt[i] == Pow2) return i;
    }

    // fallback closest
    int32 BestIdx = 0;
    int32 BestDist = INT32_MAX;
    for (int32 i = 0; i < Opt.Num(); ++i)
    {
        const int32 Dist = FMath::Abs(Opt[i] - Pow2);
        if (Dist < BestDist) { BestDist = Dist; BestIdx = i; }
    }
    return BestIdx;
}

int32 UVideoDlg::TexSizePow2FromIndex(int32 Index)
{
    const TArray<int32>& Opt = GetTexSizeOptions();
    return Opt.IsValidIndex(Index) ? Opt[Index] : 2048;
}

float UVideoDlg::Gamma01FromLevel(int32 InGammaLevel)
{
    const int32 G = ClampGammaLevel(InGammaLevel);
    return (float)(G - 32) / (float)(224 - 32);
}

int32 UVideoDlg::GammaLevelFrom01(float V01)
{
    const float C = FMath::Clamp(V01, 0.0f, 1.0f);
    const float G = 32.0f + (224.0f - 32.0f) * C;
    return ClampGammaLevel(FMath::RoundToInt(G));
}

void UVideoDlg::BuildListsIfNeeded()
{
    if (bListsBuilt)
        return;

    auto FillOnOff = [](UComboBoxString* Combo)
        {
            if (!Combo) return;

            const int32 PrevIdx = Combo->GetSelectedIndex(); // may be -1
            Combo->ClearOptions();
            Combo->AddOption(TEXT("OFF"));
            Combo->AddOption(TEXT("ON"));

            // Preserve selection if valid, else default to OFF:
            Combo->SetSelectedIndex(PrevIdx >= 0 ? PrevIdx : 0);
        };

    // Mode list (keep minimal/stable)
    if (ModeCombo)
    {
        // 16:9 – standard
        ModeCombo->AddOption(TEXT("1280x720"));    // HD
        ModeCombo->AddOption(TEXT("1600x900"));    // HD+
        ModeCombo->AddOption(TEXT("1920x1080"));   // Full HD
        ModeCombo->AddOption(TEXT("2560x1440"));   // QHD
        ModeCombo->AddOption(TEXT("3200x1800"));   // QHD+
        ModeCombo->AddOption(TEXT("3840x2160"));   // 4K UHD

        // 16:10
        ModeCombo->AddOption(TEXT("1280x800"));
        ModeCombo->AddOption(TEXT("1680x1050"));
        ModeCombo->AddOption(TEXT("1920x1200"));
        ModeCombo->AddOption(TEXT("2560x1600"));

        // Ultrawide 21:9
        ModeCombo->AddOption(TEXT("2560x1080"));
        ModeCombo->AddOption(TEXT("3440x1440"));
        ModeCombo->AddOption(TEXT("3840x1600"));

        // Super ultrawide 32:9
        ModeCombo->AddOption(TEXT("5120x1440"));
        ModeCombo->AddOption(TEXT("7680x2160"));   // Dual 4K wide

        // Set sane default
        ModeCombo->SetSelectedOption(TEXT("1920x1080"));
    }

    // Tex sizes
    if (TexSizeCombo)
    {
        if (TexSizeCombo->GetOptionCount() == 0)
        {
            TexSizeCombo->ClearOptions();
            for (int32 Pow2 : GetTexSizeOptions())
                TexSizeCombo->AddOption(FString::FromInt(Pow2));
            TexSizeCombo->SetSelectedIndex(3); // 2048
        }
    }

    // Detail levels
    if (DetailCombo)
    {
        if (DetailCombo->GetOptionCount() == 0)
        {
            DetailCombo->ClearOptions();
            DetailCombo->AddOption(TEXT("LOW"));
            DetailCombo->AddOption(TEXT("MEDIUM"));
            DetailCombo->AddOption(TEXT("HIGH"));
            DetailCombo->AddOption(TEXT("ULTRA"));
            DetailCombo->SetSelectedIndex(1);
        }
    }

    // Terrain textures
    if (TextureCombo)
        FillOnOff(TextureCombo);   EnsureSelected(TextureCombo, bTerrainTextures ? 1 : 0);

    FillOnOff(LensFlareCombo); EnsureSelected(LensFlareCombo, 1);
    FillOnOff(CoronaCombo);    EnsureSelected(CoronaCombo, 1);
    FillOnOff(NebulaCombo);    EnsureSelected(NebulaCombo, 1);
    FillOnOff(ShadowsCombo);   EnsureSelected(ShadowsCombo, 1);
    FillOnOff(SpecMapsCombo);  EnsureSelected(SpecMapsCombo, 1);
    FillOnOff(BumpMapsCombo);  EnsureSelected(BumpMapsCombo, 1);

    // Dust
    if (DustCombo)
    {
        if (DustCombo->GetOptionCount() == 0)
        {
            DustCombo->ClearOptions();
            DustCombo->AddOption(TEXT("0"));
            DustCombo->AddOption(TEXT("1"));
            DustCombo->AddOption(TEXT("2"));
            DustCombo->AddOption(TEXT("3"));
            DustCombo->SetSelectedIndex(1);
        }
    }

    bListsBuilt = true;
}

// ---------------- Rows ----------------

void UVideoDlg::BuildVideoRows()
{
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
        return;

    VBox->ClearChildren();

    auto Require = [&](UWidget* W, const TCHAR* Name) -> bool
        {
            if (!W)
            {
                UE_LOG(LogVideoDlg, Error, TEXT("[VideoDlg] Widget '%s' is NULL (BP name mismatch or not IsVariable)."), Name);
                return false;
            }
            return true;
        };

    // These are the core controls we expect for the page to look correct:
    const bool bOK =
        Require(ModeCombo, TEXT("ModeCombo")) &
        Require(TexSizeCombo, TEXT("TexSizeCombo")) &
        Require(DetailCombo, TEXT("DetailCombo")) &
        Require(TextureCombo, TEXT("TextureCombo")) &
        Require(GammaSlider, TEXT("GammaSlider")) &
        Require(LensFlareCombo, TEXT("LensFlareCombo")) &
        Require(CoronaCombo, TEXT("CoronaCombo")) &
        Require(NebulaCombo, TEXT("NebulaCombo")) &
        Require(DustCombo, TEXT("DustCombo")) &
        Require(ShadowsCombo, TEXT("ShadowsCombo")) &
        Require(SpecMapsCombo, TEXT("SpecMapsCombo")) &
        Require(BumpMapsCombo, TEXT("BumpMapsCombo"));

    if (!bOK)
    {
        UE_LOG(LogVideoDlg, Error, TEXT("[VideoDlg] BuildVideoRows aborted due to NULL controls."));
        return;
    }

    // Match AudioDlg label pattern:
    AddLabeledRow(TEXT("DISPLAY MODE"), ModeCombo, 520.f);
    AddLabeledRow(TEXT("MAX TEXTURE SIZE"), TexSizeCombo, 520.f);
    AddLabeledRow(TEXT("TERRAIN DETAIL"), DetailCombo, 520.f);
    AddLabeledRow(TEXT("TERRAIN TEXTURES"), TextureCombo, 520.f);
    AddLabeledRow(TEXT("GAMMA"), GammaSlider, 520.f);

    AddLabeledRow(TEXT("LENS FLARE"), LensFlareCombo, 520.f);
    AddLabeledRow(TEXT("CORONA"), CoronaCombo, 520.f);
    AddLabeledRow(TEXT("NEBULA"), NebulaCombo, 520.f);
    AddLabeledRow(TEXT("DUST"), DustCombo, 520.f);

    AddLabeledRow(TEXT("SHADOWS"), ShadowsCombo, 520.f);
    AddLabeledRow(TEXT("SPECULAR MAPS"), SpecMapsCombo, 520.f);
    AddLabeledRow(TEXT("BUMP MAPS"), BumpMapsCombo, 520.f);
}

// ---------------- Handlers ----------------

void UVideoDlg::OnModeChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/)
{
    if (!ModeCombo) return;

    FString Opt = ModeCombo->GetSelectedOption();
    FString L, R;
    if (Opt.Split(TEXT("x"), &L, &R))
    {
        Width = FCString::Atoi(*L);
        Height = FCString::Atoi(*R);
        bDirty = true;
        bClosed = false;
    }
}

void UVideoDlg::OnTexSizeChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/)
{
    if (!TexSizeCombo) return;
    MaxTextureSize = TexSizePow2FromIndex(TexSizeCombo->GetSelectedIndex());
    bDirty = true;
    bClosed = false;
}

void UVideoDlg::OnDetailChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/)
{
    if (!DetailCombo) return;
    TerrainDetailIndex = DetailCombo->GetSelectedIndex();
    bDirty = true;
    bClosed = false;
}

void UVideoDlg::OnTextureChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/)
{
    bTerrainTextures = ComboOn(TextureCombo);
    bDirty = true;
    bClosed = false;
}

void UVideoDlg::OnGammaChanged(float Value)
{
    GammaLevel = GammaLevelFrom01(Value);
    bDirty = true;
    bClosed = false;
}

void UVideoDlg::OnLensFlareChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/) { bLensFlare = ComboOn(LensFlareCombo); bDirty = true; bClosed = false; }
void UVideoDlg::OnCoronaChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/) { bCorona = ComboOn(CoronaCombo); bDirty = true; bClosed = false; }
void UVideoDlg::OnNebulaChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/) { bNebula = ComboOn(NebulaCombo); bDirty = true; bClosed = false; }

void UVideoDlg::OnShadowsChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/) { bShadows = ComboOn(ShadowsCombo); bDirty = true; bClosed = false; }
void UVideoDlg::OnSpecMapsChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/) { bSpecularMaps = ComboOn(SpecMapsCombo); bDirty = true; bClosed = false; }
void UVideoDlg::OnBumpMapsChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/) { bBumpMaps = ComboOn(BumpMapsCombo); bDirty = true; bClosed = false; }

void UVideoDlg::OnDustChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/)
{
    if (!DustCombo) return;
    DustLevel = FMath::Clamp(DustCombo->GetSelectedIndex(), 0, 3);
    bDirty = true;
    bClosed = false;
}

// ---------------- Buttons / Tabs ----------------

void UVideoDlg::OnApplyClicked()
{
    if (OptionsManager)
        OptionsManager->ApplyOptions();
    else
        Apply();
}

void UVideoDlg::OnCancelClicked()
{
    if (OptionsManager)
        OptionsManager->CancelOptions();
    else
        Cancel();
}

UScrollBox* UVideoDlg::EnsureContentScrollBox()
{
    if (ContentScroll)
        return ContentScroll;

    if (!WidgetTree || !RootCanvas)
    {
        UE_LOG(LogVideoDlg, Error, TEXT("[VideoDlg] EnsureContentScrollBox failed: WidgetTree or RootCanvas is NULL"));
        return nullptr;
    }

    ContentScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ContentScroll_Auto"));
    if (!ContentScroll)
        return nullptr;

    // Basic scroll behavior (safe defaults)
    ContentScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
    ContentScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    ContentScroll->SetAnimateWheelScrolling(true);

    // Add to RootCanvas and ANCHOR IT so it doesn't sit at 0,0 with no layout
    UCanvasPanelSlot* CSlot = RootCanvas->AddChildToCanvas(ContentScroll);
    if (!CSlot)
    {
        UE_LOG(LogVideoDlg, Error, TEXT("[VideoDlg] EnsureContentScrollBox failed: could not create CanvasPanelSlot"));
        return ContentScroll;
    }

    // Full-stretch anchors
    CSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    CSlot->SetAlignment(FVector2D(0.f, 0.f));

    // Offsets: LEFT, TOP, RIGHT, BOTTOM margins.
    // Tune these to match your other options pages.
    // If your tabs/header are at the top, give TOP a bigger margin.
    CSlot->SetOffsets(FMargin(24.f, 96.f, 24.f, 96.f));

    CSlot->SetAutoSize(false);
    return ContentScroll;
}

void UVideoDlg::OnAudioClicked() { if (OptionsManager) OptionsManager->ShowAudDlg(); }
void UVideoDlg::OnVideoClicked() { if (OptionsManager) OptionsManager->ShowVidDlg(); }
void UVideoDlg::OnControlsClicked() { if (OptionsManager) OptionsManager->ShowCtlDlg(); }
void UVideoDlg::OnGameClicked() { if (OptionsManager) OptionsManager->ShowGameDlg(); }
void UVideoDlg::OnModClicked() { if (OptionsManager) OptionsManager->ShowModDlg(); }
