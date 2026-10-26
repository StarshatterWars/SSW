#pragma once

#include "CoreMinimal.h"
#include "MissionEditorDlg.h"
#include "MissionEditorScreen.generated.h"

/** Native UMG presentation of legacy MsnEditDlg.frm; existing editor owns behavior. */
UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UMissionEditorScreen : public UMissionEditorDlg
{
    GENERATED_BODY()

public:
    UMissionEditorScreen(const FObjectInitializer& ObjectInitializer);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void BindFormWidgets() override;

private:
    void BuildNativeLayout();
};
