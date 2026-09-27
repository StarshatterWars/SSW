/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           VideoDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UVideoDlg
    - Video options subpage hosted by UOptionsScreen.
    - AutoVBox runtime layout (RootCanvas -> AutoVBox) via UBaseScreen helpers.
    - Reads/writes ONLY UStarshatterVideoSettings (config model).
    - Runtime apply is delegated to UStarshatterVideoSubsystem::ApplySettingsToRuntime().
    - Tab routing goes through UOptionsScreen (router), identical to AudioDlg/GameOptionsDlg.

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

// Needed for ESelectInfo::Type
#include "Components/ComboBoxString.h"

#include "VideoDlg.generated.h"

class UButton;
class UComboBoxString;
class USlider;
class UScrollBox;

class UOptionsScreen;
class UStarshatterVideoSettings;
class UStarshatterVideoSubsystem;

UCLASS()
class STARSHATTERWARS_API UVideoDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UVideoDlg(const FObjectInitializer& ObjectInitializer);

    void SetOptionsManager(UOptionsScreen* InManager) { OptionsManager = InManager; }
    UOptionsScreen* GetOptionsManager() const { return OptionsManager; }

    void Show();
    virtual void ExecFrame(double DeltaTime) override;

    void Apply();
    void Cancel();

    bool IsDirty() const { return bDirty; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

private:
    void BindDelegates();

    UStarshatterVideoSettings* GetVideoSettings() const;
    UStarshatterVideoSubsystem* GetVideoSubsystem() const;

    void BuildListsIfNeeded();
    void BuildVideoRows();

    void RefreshFromModel();
    void PushToModel(bool bApplyRuntimeToo);

    // Helpers
    static int32 ClampGammaLevel(int32 InGamma) { return FMath::Clamp(InGamma, 32, 224); }
    static float Gamma01FromLevel(int32 InGammaLevel);
    static int32 GammaLevelFrom01(float V01);

    static const TArray<int32>& GetTexSizeOptions();
    static int32 TexSizeIndexFromPow2(int32 Pow2);
    static int32 TexSizePow2FromIndex(int32 Index);

private:
    // Buttons
    UFUNCTION() void OnApplyClicked();
    UFUNCTION() void OnCancelClicked();

    UScrollBox* EnsureContentScrollBox();

    // Tabs
    UFUNCTION() void OnAudioClicked();
    UFUNCTION() void OnVideoClicked();
    UFUNCTION() void OnControlsClicked();
    UFUNCTION() void OnGameClicked();
    UFUNCTION() void OnModClicked();

    // Combos / Slider
    UFUNCTION() void OnModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnTexSizeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnDetailChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnTextureChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnGammaChanged(float Value);

    UFUNCTION() void OnLensFlareChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnCoronaChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnNebulaChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnDustChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION() void OnShadowsChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnSpecMapsChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnBumpMapsChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

private:
    // Router (not UPROPERTY; OptionsScreen owns pages)
    UOptionsScreen* OptionsManager = nullptr;

    bool bClosed = true;
    bool bDelegatesBound = false;
    bool bListsBuilt = false;

    UPROPERTY(Transient)
    bool bDirty = false;

    // Cached dialog state (mirrors FStarshatterVideoConfig)
    int32 Width = 1920;
    int32 Height = 1080;
    bool  bFullscreen = false;

    int32 MaxTextureSize = 2048;
    int32 GammaLevel = 128;

    bool bShadows = true;
    bool bSpecularMaps = true;
    bool bBumpMaps = true;

    bool bLensFlare = true;
    bool bCorona = true;
    bool bNebula = true;

    int32 DustLevel = 1;
    int32 TerrainDetailIndex = 1;
    bool  bTerrainTextures = true;

protected:
    // Main combos
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> ModeCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> TexSizeCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> DetailCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> TextureCombo;

    // Effects/toggles
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> LensFlareCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> CoronaCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> NebulaCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> DustCombo;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> ShadowsCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> SpecMapsCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> BumpMapsCombo;

    // Slider
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> GammaSlider;

    // Apply/Cancel
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ApplyBtn;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelBtn;

    // Tabs
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> VidTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AudTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CtlTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OptTabButton; // GAME
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ModTabButton;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ContentScroll = nullptr;
};
