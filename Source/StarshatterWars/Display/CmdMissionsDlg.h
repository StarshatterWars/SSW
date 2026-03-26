/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdMissionsDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdMissionsDlg

    Runtime missions panel for the command screen.
    Populates the mission roster from the active Campaign and updates
    the lower mission detail panel when the selection changes.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CmdDlg.h"
#include "GameStructs.h"
#include "BaseScreen.h"
#include "CmdMissionsDlg.generated.h"

class UButton;
class UTextBlock;
class UListView;
class UImage;
class UMissionListObject;

class Starshatter;
class Campaign;
class Mission;
class MissionInfo;
class UCmpnScreen;

UCLASS()
class STARSHATTERWARS_API UCmdMissionsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdMissionsDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    void SetManager(UCmpnScreen* InManager);
    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    void ShowMissionsDlg();

private:
    void ExecFrame();

    void RebuildMissionList();
    void AppendNewMissionsIfAny();
    void ValidateSelectionStillExists();

    int32 GetSelectedMissionId() const;
    void SetSelectedMissionId(int32 MissionId);

    void ClearDescription();
    void SetDescriptionForMissionInfo(MissionInfo* Info);
    void UpdateMissionDetailPanel(UMissionListObject* Item);
    void HandleMissionSelection(UObject* ItemObj);

    bool CanAcceptMission(MissionInfo* Info) const;
    void UpdateAcceptEnabled();

    void LoadFirstMissionData();
    void AddMissionInfoToList(MissionInfo* Info);

private:
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_save = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_exit = nullptr;

    UPROPERTY(meta = (BindWidget)) UListView* MissionList = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_accept = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionNameText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionTypeText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionStatusText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionSystemText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionRegionText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionObjectiveText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionSitrepText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MissionStartText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UImage* MissionImage = nullptr;

private:
    UCmpnScreen* Manager = nullptr;

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;
    Mission* SelectedMission = nullptr;

    ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_MISSIONS;

private:
    UFUNCTION() void OnSaveClicked();
    UFUNCTION() void OnExitClicked();
    UFUNCTION() void OnAcceptClicked();

    UFUNCTION() void OnMissionItemClicked(UObject* Item);
    UFUNCTION() void OnMissionSelectionChanged(UObject* Item);

protected:
    UPROPERTY()
    UCmdDlg* ParentCmdDlg = nullptr;

    UPROPERTY()
    UMissionListObject* SelectedMissionItem = nullptr;
};