/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSelectDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Converted from FormWindow/AWEvent mapping to UBaseScreen (UUserWidget-derived).
    - Preserves original member names and intent where applicable.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Text.h"
#include "List.h"

#include "GameStructs.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"

#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "SSWGameInstance.h"
#include "TimerSubsystem.h"

#include "CampaignSelectDlg.generated.h"



// UMG fwd:
class UButton;
class UListView;
class UMenuScreen;
class UTextBlock;
class URichTextBlock;
class UImage;

// Starshatter fwd:
class Campaign;
class Starshatter;

// Legacy containers/types (ported in your codebase):
template<typename T> class List;
class Bitmap;
class Text;
class ThreadSync;


UCLASS()
class STARSHATTERWARS_API UCampaignSelectDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCampaignSelectDlg(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // UUserWidget lifecycle
    // ----------------------------------------------------------------
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Menu manager hookup (matches your existing pattern)
    virtual void SetMenuManager(UMenuScreen* InManager);
    virtual void InitializeDlg(UMenuScreen* InManager);

    // ----------------------------------------------------------------
    // Legacy dialog surface (ported)
    // ----------------------------------------------------------------
    virtual void      RegisterControls();


    virtual void      ExecFrame(double DeltaTime);
    virtual bool      CanClose();


    void ShowDlg();
    void HideDlg();

    // Operations (AWEvent -> UFUNCTION handlers):
    UFUNCTION() virtual void OnCampaignSelect();
    UFUNCTION() virtual void OnNew();
    UFUNCTION() virtual void OnSaved();
    UFUNCTION() virtual void OnDelete();
    UFUNCTION() virtual void OnConfirmDelete();
    UFUNCTION() virtual void OnAccept();

    virtual uint32    LoadProc();

protected:
    virtual void      StartLoadProc();
    virtual void      StopLoadProc();
    virtual void      ShowNewCampaigns();
    virtual void      ShowSavedCampaigns();

protected:
    // ----------------------------------------------------------------
    // UBaseScreen overrides
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

protected:
    // ----------------------------------------------------------------
    // Bound UMG controls (match FORM ids)
    // ----------------------------------------------------------------

    // Buttons:
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_new = nullptr;       // id 100
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_saved = nullptr;     // id 101
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_delete = nullptr;    // id 102
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_accept = nullptr;    // id 1
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_cancel = nullptr;    // id 2

    // List:
    UPROPERTY(meta = (BindWidgetOptional)) UListView* lst_campaigns = nullptr; // id 201

    // Text description (FORM type "text" -> URichTextBlock in BaseScreen):
    UPROPERTY(meta = (BindWidgetOptional)) URichTextBlock* description = nullptr; // id 200

    // Optional labels (if your UMG includes them):
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* lbl_title = nullptr;         // id 10
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* lbl_hdr_campaign = nullptr;  // id 901
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* lbl_hdr_desc = nullptr;      // id 902

    // Optional background images (if you mapped them in UMG):
    UPROPERTY(meta = (BindWidgetOptional)) UImage* bg_9991 = nullptr; // id 9991
    UPROPERTY(meta = (BindWidgetOptional)) UImage* bg_9992 = nullptr; // id 9992

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* PlayerNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GameTimeText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* CampaignNameText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* DescriptionText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* SituationText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* Orders1Text;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* Orders2Text;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* Orders3Text;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* Orders4Text;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* LocationSystemText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* LocationRegionText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* CampaignStartTimeText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* CampaignImage;

    UPROPERTY(meta = (BindWidgetOptional))
    class UComboBoxString* CampaignSelectDD;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* PlayButtonText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* RestartButtonText;
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* PlayButton;
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* RestartButton;
    UPROPERTY(meta = (BindWidgetOptional))
    USoundBase* HoverSound;

    UPROPERTY(EditAnywhere, Category = "UI Sound")
    USoundBase* AcceptSound;

protected:
    // ----------------------------------------------------------------
    // Legacy state (ported)
    // ----------------------------------------------------------------
    Starshatter* stars = nullptr;
    Campaign* campaign = nullptr;
    int          selected_mission = 0;

    // Threading placeholders (keep names; avoid Win32 HANDLE in UE headers):
    void* hproc = nullptr;
    ThreadSync   sync;

    bool         loading = false;
    bool         loaded = false;

    Text         load_file;
    int          load_index = -1;
    bool         show_saved = false;

    List<Bitmap> images;
    Text         select_msg;

protected:
        UPROPERTY(Transient)
        TObjectPtr<UMenuScreen> manager = nullptr;

protected:
    UTexture2D* LoadTextureFromFile();
    FSlateBrush CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize);

    // UI selection state
    int32 Selected = 0;
    FName PickedRowName = NAME_None;
    TArray<FName> CampaignRowNamesByOptionIndex;
    TArray<int32> CampaignIndexByOptionIndex;

    void RefreshUIFromSubsystem();         // PlayerInfo -> UI

    UFUNCTION()
    void OnPlayButtonClicked();
    UFUNCTION()
    void OnPlayButtonHovered();
    UFUNCTION()
    void OnPlayButtonUnHovered();
    UFUNCTION()
    void OnRestartButtonClicked();
    UFUNCTION()
    void OnRestartButtonHovered();
    UFUNCTION()
    void OnRestartButtonUnHovered();
    UFUNCTION()
    void OnCancelButtonClicked();
    UFUNCTION()
    void OnCancelButtonHovered();
    UFUNCTION()
    void OnCancelButtonUnHovered();

    UFUNCTION()
    void SetCampaignDDList();

    UFUNCTION()
    void SetSelectedData(int selected);

    UFUNCTION()
    void OnSetSelected(FString dropDownInt, ESelectInfo::Type type);

    UFUNCTION()
    void GetCampaignImageFile(int selected);
    UFUNCTION()
    void PlayUISound(UObject* WorldContext, USoundBase* UISound);
    UFUNCTION()
    bool DoesSelectedCampaignSaveExist() const;

    UFUNCTION()
    void UpdateCampaignButtons();

    UPROPERTY()
    FString ImagePath;
};
