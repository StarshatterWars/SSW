/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmpCompleteDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpCompleteDlg
    - Unreal port of legacy CmpCompleteDlg.
    - Fully code-generated dialog owned by CmpnScreen.
    - Displays the last campaign event image if available.
    - Close returns to CmdDlg.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmpCompleteDlg.generated.h"

class UCanvasPanel;
class UImage;
class UButton;
class UTextBlock;
class UTexture2D;

class UCmpnScreen;
class Campaign;

UCLASS()
class STARSHATTERWARS_API UCmpCompleteDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmpCompleteDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void ShowCompleteDlg();
    void HideCompleteDlg();
    void SetManager(UCmpnScreen* InManager) { Manager = InManager; }

protected:
    void BuildScreen();
    void BuildTopBackground();
    void BuildCenterBanner();
    void BuildBottomPanel();

    UFUNCTION()
    void HandleCloseClicked();

    virtual UTexture2D* LoadCampaignTexture(const FString& CampaignPath, const FString& ImageFile) const;

protected:
    UPROPERTY()
    TObjectPtr<UImage> BgTop = nullptr;

    UPROPERTY()
    TObjectPtr<UImage> TitleImage = nullptr;

    UPROPERTY()
    TObjectPtr<UImage> BgBottom = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> InfoLabel = nullptr;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> CloseButtonText = nullptr;

protected:
    UCmpnScreen* Manager = nullptr;
    Campaign* CampaignPtr = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> BannerTexture = nullptr;

    float ShowTime = 0.0f;
    bool bScreenBuilt = false;
};