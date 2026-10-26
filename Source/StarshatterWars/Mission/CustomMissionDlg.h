#pragma once

#include "CoreMinimal.h"
#include "SingleMissionDlg.h"
#include "CustomMissionDlg.generated.h"

// Dedicated custom mission roster/editor, using the shared tabbed mission UI.
// Custom missions are separate from the active story campaign.
UCLASS()
class STARSHATTERWARS_API UCustomMissionDlg : public USingleMissionDlg
{
    GENERATED_BODY()
public:
    UCustomMissionDlg(const FObjectInitializer& ObjectInitializer);
    virtual bool IsCustomMissionView() const override { return true; }
};
