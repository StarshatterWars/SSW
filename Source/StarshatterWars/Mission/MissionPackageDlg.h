/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionPackageDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionPackageDlg
    - Unreal conversion of MsnPkgDlg (Package Elements / Nav Plan / Threat Analysis).
    - FORM-driven via UBaseScreen.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionPackageDlg.generated.h"

class UButton;
class UListView;
class UTextBlock;

UCLASS()
class STARSHATTERWARS_API UMissionPackageDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionPackageDlg(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "MissionPackageDlg")
    void ExecFrame(float DeltaSeconds);

    // UBaseScreen
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

protected:
    virtual void NativeConstruct() override;

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

private:
    // UI callbacks
    UFUNCTION() void OnClickedAccept();
    UFUNCTION() void OnClickedCancel();
    UFUNCTION() void OnClickedTabSit();
    UFUNCTION() void OnClickedTabPkg();

    // Populate
    void DrawPackages();
    void DrawNavPlan();
    void DrawThreats();

private:
    // IDs from FORM
    static constexpr int32 ID_ACCEPT = 1;
    static constexpr int32 ID_CANCEL = 2;

    static constexpr int32 ID_TAB_SIT = 900;
    static constexpr int32 ID_TAB_PKG = 901;

    static constexpr int32 ID_PKG_LIST = 320;
    static constexpr int32 ID_NAV_LIST = 330;

    static constexpr int32 ID_THREAT_0 = 251; // ..255

private:
    // Widgets (bind in BP or via BindFormWidgets)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> PackageList = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> NavList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Threat0 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Threat1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Threat2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Threat3 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Threat4 = nullptr;

    int32 PackageIndex = 0;
};
