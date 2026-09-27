/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmdTitleDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdTitleDlg
    - Unreal UUserWidget equivalent of legacy CmdTitleDlg (Campaign Title Card).
    - Uses UBaseScreen FORM ID binding and optional legacy .frm parsing.
    - Binds Image control ID 200 as the title card image.
    - ExecFrame is called from NativeTick.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmdTitleDlg.generated.h"

class UTexture2D;
class UCampaignSubsystem;
class Campaign;
class UCmpnScreen;   // If you have an Unreal manager equivalent; otherwise remove/replace.

UCLASS()
class STARSHATTERWARS_API UCmdTitleDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdTitleDlg(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // Legacy-equivalent API
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() override;

    /** Optional: if you embed the legacy .frm text for this dialog, return it here */
    virtual FString GetLegacyFormText() const override;

    /** Called every tick (legacy ExecFrame equivalent) */
    virtual void ExecFrame(float DeltaTime);

    /** Optional manager hook (legacy CmpnScreen*) */
    void SetManager(UCmpnScreen* InManager) { Manager = InManager; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // ----------------------------------------------------------------
    // Widgets (BindWidgetOptional so you can keep pure C++ screens)
    // ----------------------------------------------------------------

    /** Legacy ctrl id 200 */
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* TitleImage = nullptr;

public:
    // ----------------------------------------------------------------
    // Optional content setup
    // ----------------------------------------------------------------
    void SetTitleTexture(UTexture2D* InTexture);
    float GetShowTime() const { return ShowTime; }

    float AutoFinishAfterSeconds = 0.0f;
    void OnTitleFinished();

protected:
    // ----------------------------------------------------------------
    // Legacy state equivalents
    // ----------------------------------------------------------------
    UCmpnScreen* Manager = nullptr;

    UCampaignSubsystem* CampaignSubsystem = nullptr;
    Campaign* Campaign = nullptr;

    float ShowTime = 0.0f;
    bool bFinished = false;
};
