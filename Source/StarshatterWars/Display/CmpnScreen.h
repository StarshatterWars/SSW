/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpnScreen.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpnScreen
    - Legacy-faithful campaign screen manager.
    - UCmdDlg performs the main campaign UI work.
    - UCmpnScreen preserves high-level campaign flow:
        * startup cutscene check
        * training completion flow
        * campaign complete / failed flow
        * topmost modal close handling
        * message / complete / scene overlays
        * campaign screen ticking
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "GameStructs.h"
#include "CmpnScreen.generated.h"

class UCmdDlg;
class UCmdMsgDlg;
class UCmpFileDlg;
class UCmpLoadDlg;
class UCmpCompleteDlg;
class UCampaignSceneDlg;
class UMenuScreen;
class ULevelStreamingDynamic;

class Campaign;
class Starshatter;
class CombatEvent;

UCLASS()
class STARSHATTERWARS_API UCmpnScreen : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmpnScreen(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void ExecFrame(double DeltaTime) override;

    void Setup();
    void TearDown();

    bool IsShown() const { return bIsShown; }

    virtual void Show() override;
    virtual void Hide() override;
    void HideAll();

    bool CloseTopmost();

    virtual void SetMenuManager(UMenuScreen* InManager);
    virtual void InitializeDlg(UMenuScreen* InManager);

    void ShowCmdDlg();
    void HideCmdDlg();
    bool IsCmdShown() const;
    UCmdDlg* GetCmdDlg() const { return CmdDlg; }

    void ShowCmpFileDlg();
    void HideCmpFileDlg();
    bool IsCmpFileShown() const;
    UCmpFileDlg* GetCmpFileDlg() const { return CmpFileDlg; }

    void ShowCmdMsgDlg();
    void HideCmdMsgDlg();
    bool IsCmdMsgShown() const;
    UCmdMsgDlg* GetCmdMsgDlg() const { return CmdMsgDlg; }

    void ShowCmpCompleteDlg();
    void HideCmpCompleteDlg();
    bool IsCmpCompleteShown() const;
    UCmpCompleteDlg* GetCmpCompleteDlg() const { return CmpCompleteDlg; }

    void ShowCmpSceneDlg();
    void HideCmpSceneDlg();
    bool IsCmpSceneShown() const;
    UCampaignSceneDlg* GetCmpSceneDlg() const { return CmpSceneDlg; }


    void ShowCmpLoadDlg();
    void HideCmpLoadDlg();
    bool IsCmpLoadShown() const;
    UCmpLoadDlg* GetCmpLoadDlg() const { return CmpLoadDlg; }

    void SetFieldOfView(float InFOV);
    float GetFieldOfView() const;

    Campaign* GetCampaign() const { return CampaignPtr; }
    Starshatter* GetStars() const { return Stars; }

    void SetShowMissionsRequested(bool bRequested) { bShowMissionsRequested = bRequested; }

public:
    UPROPERTY(EditDefaultsOnly, Category = "Campaign|Classes")
    TSubclassOf<UCmdDlg> CmdDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Campaign|Classes")
    TSubclassOf<UCmpFileDlg> CmpFileDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Campaign|Classes")
    TSubclassOf<UCmdMsgDlg> CmdMsgDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Campaign|Classes")
    TSubclassOf<UCmpCompleteDlg> CmpCompleteDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Campaign|Classes")
    TSubclassOf<UCampaignSceneDlg> CmpSceneDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Campaign|Classes")
    TSubclassOf<UCmpLoadDlg> CmpLoadDlgClass;


protected:
    template<typename TDialog>
    TDialog* EnsureDialog(TSubclassOf<TDialog> ClassToSpawn, TObjectPtr<TDialog>& Storage, int32 ZOrder);

    void RefreshRuntimePointers();
    void ApplyManagerToChildren();

    bool TryStartSceneForEvent(CombatEvent* Event);
    float GetSceneDurationSeconds(const FString& SceneName) const;

    bool StreamSceneSystemLevel(const FS_CampaignMission& SceneMission);
    FName ResolveSceneSystemLevelName(const FString& SystemName) const;
    bool IsSceneSystemLevelLoaded(const FString& SystemName) const;

protected:
    const FS_CampaignMission* FindCampaignMissionByScene(const FString& SceneName) const;
    void AdvanceCampaignScene();

    bool IsSceneVisualReady() const;
    bool CanRevealSceneNow() const;
    bool AreShadersReadyForReveal() const;
    void BeginSceneTransition();

protected:
    FString ActiveSceneName;
    float ActiveSceneDurationSeconds = 0.0f;

protected:
    UPROPERTY()
    TObjectPtr<UCmdDlg> CmdDlg = nullptr;

    UPROPERTY()
    TObjectPtr<UCmpFileDlg> CmpFileDlg = nullptr;

    UPROPERTY()
    TObjectPtr<UCmdMsgDlg> CmdMsgDlg = nullptr;

    UPROPERTY()
    TObjectPtr<UCmpCompleteDlg> CmpCompleteDlg = nullptr;

    UPROPERTY()
    TObjectPtr<UCampaignSceneDlg> CmpSceneDlg = nullptr;

    UPROPERTY()
    TObjectPtr<UCmpLoadDlg> CmpLoadDlg = nullptr;

    UPROPERTY()
    TObjectPtr<ULevelStreamingDynamic> ActiveSceneStreamingLevel = nullptr;

protected:
    Campaign* CampaignPtr = nullptr;
    Starshatter* Stars = nullptr;

    bool bSetupComplete = false;
    bool bIsShown = false;
    bool bShowMissionsRequested = false;
    bool bExitLatch = false;
    bool bHidingAll = false;

    int32 CompletionStage = 0;

    double TimeTilChange = 0.0;
    float DefaultFallbackFOV = 90.0f;
    float DesiredFieldOfView = 90.0f;

    bool  bCampaignPaused = false;
    bool bSceneTransitionActive = false;
    bool bSceneWarmupStarted = false;

    float SceneLoadScreenStartTime = 5.0f;
    float SceneMinLoadScreenSeconds = 1.0f;
    float SceneWarmupReadyTime = 0.0f;
    float ScenePostLoadWarmupSeconds = 1.0f;
};