#pragma once
#include "CoreMinimal.h"
#include "InMissionPanelBase.h"
#include "ObjectivesDlg.generated.h"

// WBP_Objectives: PanelTitle TextBlock and RuntimeHost SizeBox in a CanvasPanel.
UCLASS(Blueprintable)
class STARSHATTERWARS_API UObjectivesDlg : public UInMissionPanelBase
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<SWidget> CreatePanelContent() override;
    virtual FText GetPanelCaption() const override;
};
