#pragma once
#include "CoreMinimal.h"
#include "InMissionPanelBase.h"
#include "NavigationDlg.generated.h"

class UMissionNavDlg;

// Blueprint shell uses PanelTitle (TextBlock) and RuntimeHost (SizeBox),
// exactly like EngineeringDlg and WeaponsDlg.
UCLASS(Blueprintable)
class STARSHATTERWARS_API UNavigationDlg : public UInMissionPanelBase
{
    GENERATED_BODY()
protected:
    // First integration step: show the Blueprint shell without loading the map.
    UPROPERTY(EditDefaultsOnly, Category="Navigation")
    bool bFrameOnly = true;

    UPROPERTY(EditDefaultsOnly, Category="Navigation")
    TSubclassOf<UMissionNavDlg> NavigationMapClass;
    UPROPERTY(Transient) TObjectPtr<UMissionNavDlg> NavigationMap;
    virtual TSharedRef<SWidget> CreatePanelContent() override;
    virtual FText GetPanelCaption() const override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
