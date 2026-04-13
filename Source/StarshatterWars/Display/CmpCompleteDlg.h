/*  Project Starshatter Wars
    Fractal Dev Studios
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

    float ShowTime = 0.0f;
    bool bScreenBuilt = false;
};