#pragma once
#include "BaseScreen.h"
#include "LoadoutsDlg.generated.h"
class UMissionWeaponDlg;
UCLASS(Blueprintable)
class STARSHATTERWARS_API ULoadoutsDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    void SetEditor(UMissionWeaponDlg* InEditor) { Editor = InEditor; }
protected:
    TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY(Transient) TObjectPtr<UMissionWeaponDlg> Editor;
};
