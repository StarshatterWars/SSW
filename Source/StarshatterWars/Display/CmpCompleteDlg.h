/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpCompleteDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpCompleteDlg
    - Unreal port of legacy CmpCompleteDlg (campaign complete/title + progress dialog).
    - Uses UBaseScreen FORM ID binding and .frm parsing.
    - Loads and displays the last campaign CombatEvent image into ctrl id 100.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmpCompleteDlg.generated.h"

class UImage;
class UButton;
class UTextBlock;
class UTexture2D;

class UCmpnScreen;            
class UCampaignSubsystem;     
class Campaign;              

UCLASS()
class STARSHATTERWARS_API UCmpCompleteDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmpCompleteDlg(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // Legacy-equivalent API
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    /** Legacy Show() equivalent (call after widget is constructed/added). */
    UFUNCTION(BlueprintCallable, Category = "CmpCompleteDlg")
    void Show();

    /** Legacy ExecFrame() equivalent (optional). */
    virtual void ExecFrame(float DeltaTime);

    /** Legacy manager hook (CmpnScreen*). */
    void SetManager(UCmpnScreen* InManager) { Manager = InManager; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // ----------------------------------------------------------------
    // Widgets (BindWidgetOptional so UMG can be minimal)
    // ----------------------------------------------------------------

    /** Legacy ctrl id 100 */
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* TitleImage = nullptr;

    /** Legacy ctrl id 101 */
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* InfoLabel = nullptr;

    /** Legacy ctrl id 1 */
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* CloseButton = nullptr;

    /** Optional backgrounds from frm ids 300 and 400 */
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BgTop = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BgBottom = nullptr;

protected:
    // ----------------------------------------------------------------
    // Close handling (legacy OnClose)
    // ----------------------------------------------------------------
    UFUNCTION()
    void HandleCloseClicked();

    /** If you do not use a Manager pointer, implement this in BP to route to cmd dialog. */
    UFUNCTION(BlueprintImplementableEvent, Category = "CmpCompleteDlg")
    void OnRequestShowCmdDlg();

protected:
    // ----------------------------------------------------------------
    // Image loading hook (one place to integrate your Bitmap/DataLoader bridge)
    // ----------------------------------------------------------------

    /**
     * Resolve a campaign-relative image file (typically .pcx) to a UTexture2D.
     * Default implementation returns nullptr (override where you have your loader).
     */
    virtual UTexture2D* LoadCampaignTexture(const FString& CampaignPath, const FString& ImageFile) const;

protected:
    // ----------------------------------------------------------------
    // State
    // ----------------------------------------------------------------
    UCmpnScreen* Manager = nullptr;
    //CampaignSubsystem* CampaignSubsystem = nullptr;
    Campaign* Campaign = nullptr;

    float ShowTime = 0.0f;

    UTexture2D* BannerTexture = nullptr;
};
