#pragma once
#include "CampaignSceneDlg.h"
#include "ShipBriefingPreview.h"
#include "CutsceneReplayDlg.generated.h"

// Archive-only presentation. Never loads a mission into Sim or changes the player's camera.
UCLASS()
class STARSHATTERWARS_API UCutsceneReplayDlg : public UCampaignSceneDlg
{
    GENERATED_BODY()
    friend class FArchiveReplayTest;
public:
    void LoadReplay(const FS_CampaignMission& Scene,int32 CampaignNumber);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    void BuildPreview();
    void UpdatePreview(float DeltaTime);
    void ReplayCamera(const FS_MissionEvent& Event);
    UFUNCTION() void ReplayPlay();
    UFUNCTION() void ReplayPause();
    UFUNCTION() void ReplayRewind();
    UFUNCTION() void ReplayStop();
    UPROPERTY(Transient) TObjectPtr<class UViewport> ReplayViewport;
    UPROPERTY(Transient) TArray<FBriefingShipPreviewPart> RetainedParts;
    TMap<FString,TWeakObjectPtr<AActor>> ReplayActors;
    TMap<FString,FVector> InitialPositions;
    FString FocusName;
    FVector Orbit=FVector(1,.25,6000);
    FVector OrbitRate=FVector::ZeroVector;
    float ReplaySeconds=0;
    int32 NextReplayEvent=0;
    bool bReplayPlaying=false;
    bool bPreviewBuilt=false;
};
