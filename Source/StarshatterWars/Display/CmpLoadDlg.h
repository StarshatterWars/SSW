/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpLoadDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Code-built campaign loading dialog.

    This replaces the legacy FORM-driven CmpLoadDlg layout with a
    native Unreal UMG screen built entirely in C++.

    Visual layout:
    - Full-screen background image (starfield fallback tint if missing)
    - Centered scrCampaignLoad texture
    - Campaign name centered over the art in large Serpentine font
    - Bottom panel with loading activity text and progress bar

    Behavior parity:
    - Show() captures display time
    - ExecFrame() refreshes activity/progress
    - IsDone() enforces a 5 second minimum display duration
    - When complete, transitions to UCmpnScreen once
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmpLoadDlg.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UOverlay;
class UProgressBar;
class USizeBox;
class USpacer;
class UTextBlock;
class UVerticalBox;
class UTexture2D;

UCLASS()
class STARSHATTERWARS_API UCmpLoadDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmpLoadDlg(const FObjectInitializer& ObjectInitializer);

    virtual void Show() override;
    virtual void Hide() override;
    virtual void ExecFrame(double DeltaTime) override;
    virtual bool IsDone() const;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    void BuildScreen();
    void BuildBackgroundLayer();
    void BuildMainLayout();
    void BuildBottomPanel();

    void LoadArtAssets();
    void ApplyStaticArt();
    void ApplyTitleFont();
    void ApplyCampaignTitleCard();
    void ApplyInitialVisualState();
    void RefreshLoadState();

    uint32 GetRealTimeMs() const;

    UTextBlock* BuildVBoxLabel(
        UVerticalBox* Parent,
        const FString& Text,
        int32 FontSize,
        const FLinearColor& Color);

    USpacer* BuildVBoxSpacer(UVerticalBox* Parent, float Height);

protected:
    bool bScreenBuilt = false;
    bool bTransitionedToCmpnScreen = false;
    uint32 ShowTimeMs = 0;

protected:
    UPROPERTY()
    TObjectPtr<UImage> BackgroundImage = nullptr;

    UPROPERTY()
    TObjectPtr<UVerticalBox> MainVBox = nullptr;

    UPROPERTY()
    TObjectPtr<USizeBox> CenterArtBox = nullptr;

    UPROPERTY()
    TObjectPtr<UOverlay> CenterArtOverlay = nullptr;

    UPROPERTY()
    TObjectPtr<UImage> CenterImage = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> CenterTitleText = nullptr;

    UPROPERTY()
    TObjectPtr<UBorder> BottomPanel = nullptr;

    UPROPERTY()
    TObjectPtr<UVerticalBox> BottomPanelVBox = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> LblActivity = nullptr;

    UPROPERTY()
    TObjectPtr<UProgressBar> ProgressBar = nullptr;

protected:
    UPROPERTY()
    TObjectPtr<UTexture2D> DefaultCenterTexture = nullptr;

    UPROPERTY()
    TObjectPtr<UTexture2D> DefaultBackgroundTexture = nullptr;

    UPROPERTY()
    TObjectPtr<UObject> SerpentineFontObject = nullptr;
};