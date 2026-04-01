/*
    Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Mission UI
    FILE:         MissionWeaponDlg.h

    OVERVIEW
    ========
    Code-first Mission Weapon panel using UMG host.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionWeaponDlg.generated.h"

class UBorder;
class UHorizontalBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;

class UMissionPlanner;
class UMissionBriefingDlg;
class UMissionWeaponLoadoutListObject;
class UMissionLoadoutListView;

class Mission;
class MissionElement;
class MissionLoad;

struct FShipDesign;
struct FShipLoadout;
struct FShipHardPoint;
struct FWeaponDesign;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionWeaponDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }
    void SetParentDlg(UMissionBriefingDlg* InParentDlg);

    void RefreshFromMission();

protected:
    virtual void NativeConstruct() override;

private:
    void BuildRuntimeLayout();

    UTextBlock* BuildLabelText(const FString& Text) const;
    UTextBlock* BuildValueText(const FString& Text) const;
    UBorder* BuildHeader(const FString& Text) const;

    Mission* ResolveMission() const;
    MissionElement* ResolvePlayerElement() const;
    const FShipDesign* ResolvePlayerShipDesign() const;

    void ClearLoadouts();
    void BuildLoadouts(MissionElement* Element, const FShipDesign* Design);

    bool GetSelectedLoadoutName(MissionElement* Element, FString& OutName) const;

    FString GetElementName(MissionElement* Element) const;
    FString GetDesignName(const FShipDesign* Design) const;

    FString FormatWeight(double Mass) const;

private:
    // UMG host (ONLY bind from BP)
    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* RuntimeHost = nullptr;

private:
    UPROPERTY()
    UMissionBriefingDlg* ParentDlg = nullptr;

    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;

    // Code-created widgets (NO BP name collisions)
    UPROPERTY()
    UTextBlock* ElementNameValueText = nullptr;

    UPROPERTY()
    UTextBlock* DesignNameValueText = nullptr;

    UPROPERTY()
    UTextBlock* WeightValueText = nullptr;

    UPROPERTY()
    UMissionLoadoutListView* WeaponListView = nullptr;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UUserWidget> EntryWidgetClass;

    UPROPERTY()
    TArray<TObjectPtr<UMissionWeaponLoadoutListObject>> Items;

private:
    double ComputeLoadoutMass(const FShipDesign* Design, const FShipLoadout& Loadout) const;
    double ComputeCurrentCustomMass(MissionElement* Element, const FShipDesign* Design) const;

    const FWeaponDesign* ResolveWeaponDesignForStationSelection(
        const FShipDesign* Design,
        int32 StationIndex,
        int32 PointIndex) const;
};