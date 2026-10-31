#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "GameStructs.h"
#include "Components/ComboBoxString.h"
#include "PlayerDlg.generated.h"

class UButton;
class UEditableTextBox;
class UImage;
class UTextBlock;
class UScrollBox;
class UHorizontalBox;
class UVerticalBox;
class UUniformGridPanel;

class UMenuScreen;
class UStarshatterPlayerSubsystem;

UCLASS()
class STARSHATTERWARS_API UPlayerDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UPlayerDlg(const FObjectInitializer& ObjectInitializer);

    void InitializeDlg(UMenuScreen* InManager);
    virtual TSharedRef<SWidget> RebuildWidget() override;
    void BuildModernShell();
    UFUNCTION() void OnProfileSection();
    UFUNCTION() void OnServiceSection();
    UFUNCTION() void OnAwardsSection();
    UFUNCTION() void OnMacrosSection();
    void ScrollToSection(UWidget* Widget);
    friend class FModernReferenceScreensTest;
    UPROPERTY(Transient) TObjectPtr<UWidget> ModernShell=nullptr;

    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

    void ShowDlg();
    void HideDlg();

protected:
    // -------- Layout (runtime) --------
    void EnsureLayout();
    void EnsureScrollHostsStats();

    // -------- Build UI rows --------
    void BuildRosterPanel();
    void BuildSavedPlayerControls();
    void RefreshSavedPlayerList();
    bool SaveSelectedPlayer();
    UFUNCTION() void OnLoadSavedPlayer();
    UFUNCTION() void OnSaveSavedPlayer();
    UFUNCTION() void OnConfirmDeleteSavedPlayer();
    UFUNCTION() void OnKeepSavedPlayer();
    UFUNCTION() void OnSavedPlayerChanged(FString Selected,ESelectInfo::Type SelectionType);
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> SavedPlayersCombo=nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SavedPlayerStatus=nullptr;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> DeleteSavedConfirmation=nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> LoadSavedButton=nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> SaveSavedButton=nullptr;
    TArray<FString> SavedPlayerSlots;
    FString SelectedSavedSlot;
    bool bRefreshingSavedPlayers=false;
    bool bNewPilot=false;
    TOptional<FS_PlayerGameInfo> NewPilotDraft;
    void BuildStatsRows();

    // -------- Model <-> UI (SUBSYSTEM) --------
    bool UpdatePlayerFromUI_Subsystem();   // UI -> PlayerInfo + Save
    void RefreshUIFromSubsystem();         // PlayerInfo -> UI

    UStarshatterPlayerSubsystem* GetPlayerSS() const;

    // -------- Events --------
    UFUNCTION()
    void OnAdd();

    UFUNCTION()
    void OnDel();

    UFUNCTION()
    void OnApply();

    UFUNCTION()
    void OnCancel();

    void ApplyMedalsMask(uint32 MedalsMask);

    void Debug_ShowAllMedals();

    // -------- Row builders --------
    UHorizontalBox* AddStatRow(const FText& Label, UWidget* RightWidget, float RightWidth = 420.f, float RowPadY = 6.f);
    UTextBlock* MakeValueText();
    UEditableTextBox* MakeEditBox(bool bPassword = false);

    void BuildMedalsGrid(int32 Columns = 4, int32 Rows = 4);
    void BuildChatMacrosRows();

    // No lambdas:
    FString MacroLabelForIndex(int32 Index) const;

protected:
    // -------- Manager --------
    UPROPERTY()
    TObjectPtr<UMenuScreen> manager = nullptr;

    // Optional BP bind if your canvas name differs:
    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* RootCanvasPlayer = nullptr;

    // -------- Runtime layout --------
    UPROPERTY()
    TObjectPtr<UHorizontalBox> MainRow = nullptr;

    UPROPERTY()
    TObjectPtr<UVerticalBox> LeftPanel = nullptr;

    UPROPERTY()
    TObjectPtr<UScrollBox> StatsScroll = nullptr;

    UPROPERTY()
    TObjectPtr<UVerticalBox> StatsVBox = nullptr;

    // Left panel label (single profile)
    UPROPERTY()
    TObjectPtr<UTextBlock> txt_profile_name = nullptr;

    // -------- Stats widgets --------
    UPROPERTY()
    TObjectPtr<UEditableTextBox> edt_name = nullptr;

    UPROPERTY()
    TObjectPtr<UEditableTextBox> edt_password = nullptr;

    UPROPERTY()
    TObjectPtr<UEditableTextBox> edt_callsign = nullptr;

    UPROPERTY()
    TObjectPtr<UEditableTextBox> edt_squadron = nullptr;

    UPROPERTY()
    TObjectPtr<UEditableTextBox> edt_signature = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_created = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_flighttime = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_missions = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_kills = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_losses = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_points = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_empire = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> txt_rank = nullptr;

    UPROPERTY()
    TObjectPtr<UImage> img_rank = nullptr;

    UPROPERTY()
    TObjectPtr<UUniformGridPanel> medals_grid = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UImage>> MedalImages;

    UPROPERTY()
    TArray<TObjectPtr<UEditableTextBox>> MacroEdits; // 10

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<class UUniformGridPanel> grid_medals;

    // Optional cached “empty slot” texture:
    UPROPERTY(EditDefaultsOnly, Category = "Awards")
    TSoftObjectPtr<UTexture2D> MedalEmptyTexture;
};
