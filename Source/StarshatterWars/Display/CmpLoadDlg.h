/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpLoadDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpLoadDlg
    - Unreal UMG dialog equivalent of legacy CmpLoadDlg (FormWindow).
    - Inherits from UBaseScreen to use FORM parsing + ID binding.
    - Updates activity/progress each tick (ExecFrame).
    - Captures show time on Show(); IsDone() gates dismissal at 5s.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmpLoadDlg.generated.h"

class UImage;
class UTextBlock;
class USlider;

UCLASS()
class STARSHATTERWARS_API UCmpLoadDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmpLoadDlg(const FObjectInitializer& ObjectInitializer);

    // Legacy dialog operations:
    virtual void ExecFrame();
    virtual void Show();
    virtual bool IsDone() const;

    // UBaseScreen:
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

protected:
    // UUserWidget:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // FORM controls (match IDs from CmpLoadDlg.frm)
    UPROPERTY(meta = (BindWidgetOptional)) UImage* ImgTitle = nullptr; // id 100
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* LblTitle = nullptr; // id 200 (legacy C++ used it)
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* LblActivity = nullptr; // id 101
    UPROPERTY(meta = (BindWidgetOptional)) USlider* SldProgress = nullptr; // id 102

    // Optional background images if you model them in UMG:
    UPROPERTY(meta = (BindWidgetOptional)) UImage* BgTop = nullptr; // id 300
    UPROPERTY(meta = (BindWidgetOptional)) UImage* BgPanel = nullptr; // id 400

protected:
    // Show timestamp in milliseconds (legacy behavior)
    uint32 ShowTimeMs = 0;

protected:
    // Internal helpers (kept private to the class, no extra modules)
    uint32 GetRealTimeMs() const;
    void   ApplyCampaignTitleCard();
};
