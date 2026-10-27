#pragma once
#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionsPanelDriver.generated.h"
class UMenuScreen;
UCLASS()
class STARSHATTERWARS_API UMissionsPanelDriver : public UBaseScreen
{
    GENERATED_BODY()
public:
    void SetMenuManager(UMenuScreen* InManager) { Manager = InManager; }
    virtual void Show() override;
    virtual void HandleCancel() override;
protected:
    virtual void NativeConstruct() override;
    void BuildDriverLayout();
private:
    UPROPERTY(Transient) TObjectPtr<UMenuScreen> Manager;
    UFUNCTION() void OpenCampaign();
    UFUNCTION() void OpenSingle();
    UFUNCTION() void OpenCustom();
    UFUNCTION() void OpenMultiplayer();
    UFUNCTION() void Back();
};
