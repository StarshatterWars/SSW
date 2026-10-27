#pragma once
#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionsPanelDriver.generated.h"
class UMenuScreen;
class UCanvasPanel;
class UTextBlock;
class UButton;
class UUserWidget;
UCLASS()
class STARSHATTERWARS_API UMissionsPanelDriver : public UBaseScreen
{
    GENERATED_BODY()
public:
    void SetMenuManager(UMenuScreen* InManager) { Manager = InManager; }
    virtual void Show() override;
    virtual void HandleCancel() override;
    void ShowCampaignMode();
    void ShowSingleMode();
    void ShowCustomMode();
    void ShowMultiplayerMode();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    void BuildDriverLayout();
    void RefreshTabStyles();
private:
    enum class EMissionsNavigationMode : uint8 { None, Campaign, Single, Custom, Multiplayer };
    EMissionsNavigationMode ActiveMode = EMissionsNavigationMode::None;
    UPROPERTY(Transient) TObjectPtr<UButton> CampaignTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> SingleTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> CustomTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> MultiplayerTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> MenuTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UMenuScreen> Manager = nullptr;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ContentHost = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeTitle = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> ActiveChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> CampaignChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> SingleChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> CustomChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> MultiplayerChild = nullptr;
    void SelectChild(UUserWidget* Child, const TCHAR* Title);
    void HideChildHeader(UUserWidget* Child);
    UFUNCTION() void OpenCampaign();
    UFUNCTION() void OpenSingle();
    UFUNCTION() void OpenCustom();
    UFUNCTION() void OpenMultiplayer();
    UFUNCTION() void Back();
};
