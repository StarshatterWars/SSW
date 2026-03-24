/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         NavDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Converted from FormWindow to UBaseScreen (UUserWidget-derived).
    - Preserves original member names, structure, and intent.
    - AWEvent-style callbacks mapped to UFUNCTION handlers.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "List.h"

#include "NavDlg.generated.h"



// Unreal widget forward declarations:
class UButton;
class UListView;
class UTextBlock;
class UImage;
class UCanvasPanel;

// Starshatter forward declarations (ported gameplay/core):
class BaseScreen;
class MapView;
class StarSystem;
class Ship;
class SimRegion;
class Orbital;
class OrbitalRegion;
class Mission;

/**
 * Navigation Active Window class (UE UBaseScreen port)
 */
UCLASS()
class STARSHATTERWARS_API UNavDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UNavDlg(const FObjectInitializer& ObjectInitializer);

    // Original API surface (ported):
    virtual void      RegisterControls();
    virtual void      ExecFrame();

    StarSystem* GetSystem() const { return star_system; }
    void              SetSystem(StarSystem* s);

    Mission* GetMission() const { return mission; }
    void              SetMission(Mission* m);

    Ship* GetShip() const { return ship; }
    void              SetShip(Ship* s);

    bool              GetEditorMode() const { return editor; }
    void              SetEditorMode(bool b);

    void              UseViewMode(int mode);
    void              UseFilter(int index);
    void              SelectObject(int index);
    void              UpdateSelection();
    void              UpdateLists();
    void              CoordinateSelection();

    void              SelectStar(Orbital* star);
    void              SelectPlanet(Orbital* planet);
    void              SelectRegion(OrbitalRegion* rgn);

    enum NAV_EDIT_MODE
    {
        NAV_EDIT_NONE = 0,
        NAV_EDIT_ADD = 1,
        NAV_EDIT_DEL = 2,
        NAV_EDIT_MOVE = 3
    };

    void              SetNavEditMode(int mode);
    int               GetNavEditMode() const { return nav_edit_mode; }

    // Button handlers (ported from AWEvent):
    UFUNCTION()
    void              OnView();

    UFUNCTION()
    void              OnFilter();

    UFUNCTION()
    void              OnSelect();

    UFUNCTION()
    void              OnCommit();

    UFUNCTION()
    void              OnCancel();

    UFUNCTION()
    void              OnEngage();

    UFUNCTION()
    void              OnMapDown();

    UFUNCTION()
    void              OnMapMove();

    UFUNCTION()
    void              OnMapClick();

    UFUNCTION()
    void              OnClose();

    MapView*          star_map;

protected:
    // UUserWidget lifecycle:
    virtual void      NativeOnInitialized() override;
    virtual void      NativeConstruct() override;
    virtual void      NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // Buttons:
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* view_btn_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* view_btn_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* view_btn_2 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* filter_btn_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* filter_btn_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* filter_btn_2 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* filter_btn_3 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* filter_btn_4 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* filter_btn_5 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* commit_btn = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* zoom_in_btn = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* zoom_out_btn = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UButton* close_btn = nullptr;

    // Map / panels:
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UCanvasPanel* map_win = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UCanvasPanel* loc_labels = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UCanvasPanel* dst_labels = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UCanvasPanel* loc_data = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UCanvasPanel* dst_data = nullptr;

    // Lists:
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UListView* seln_list = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    UListView* info_list = nullptr;

    // Manager (original BaseScreen*):
    UPROPERTY(BlueprintReadWrite, Category = "NavDlg")
    UObject* manager = nullptr;

    // State:
    UPROPERTY(BlueprintReadOnly, Category = "NavDlg")
    int32 seln_mode = 0;

    UPROPERTY(BlueprintReadOnly, Category = "NavDlg")
    int32 nav_edit_mode = NAV_EDIT_NONE;

    // Gameplay references (non-UObject, managed externally):
    StarSystem* star_system = nullptr;
    Ship* ship = nullptr;
    Mission* mission = nullptr;

    List<Orbital>        stars;
    List<Orbital>        planets;
    List<OrbitalRegion>  regions;
    List<Ship>           contacts;

    // Editor flag:
    UPROPERTY(BlueprintReadOnly, Category = "NavDlg")
    bool editor = false;
};
