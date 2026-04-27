/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpLoadDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Campaign loading dialog (modernized).

    This class is a PURE VISUAL OVERLAY used during:
    - Campaign startup
    - Level streaming transitions
    - Scene preparation

    UCmpnScreen controls transition flow.
    This class only displays loading visuals and status.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmpLoadDlg.generated.h"

class UCmpnScreen;
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

    void SetCmpnScreen(UCmpnScreen* InScreen) { CmpnScreen = InScreen; }
    void SetCampaignName(const FString& InName);

    void SetLoadingStage(const FString& InStageText, float InPercent);
    void ClearManualLoadingStage();
    bool IsUsingManualStage() const { return bUseManualStage; }

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
    uint32 ShowTimeMs = 0;

protected:
    UPROPERTY()
    TObjectPtr<UCmpnScreen> CmpnScreen = nullptr;

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

protected:
    bool bUseManualStage = false;
    FString ManualStageText = TEXT("LOADING...");
    float ManualStagePercent = 0.0f;
};