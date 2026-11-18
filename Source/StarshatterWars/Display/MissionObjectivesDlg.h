#pragma once
#include "MissionsDlg.h"
#include "MissionObjectivesDlg.generated.h"
class Mission;
UCLASS(Blueprintable)
class STARSHATTERWARS_API UMissionObjectivesDlg : public UMissionsDlg
{
    GENERATED_BODY()
public:
    void SetMissionBriefing(const Mission&,FText InTitle,FText Agent,FText AssignmentStatus,
        FSimpleDelegate OnAccept,FSimpleDelegate OnDecline,FSimpleDelegate OnClose);
protected:
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UShipBriefingPreview> Rehearsal;
};
