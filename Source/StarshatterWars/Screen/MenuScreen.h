#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "MenuScreen.generated.h"


// ------------------------------------------------------------
// Forward declarations (dialogs)
// ------------------------------------------------------------

class UMenuDlg;
class UExitDlg;
class UConfirmDlg;

class UFirstTimeDlg;
class UPlayerDlg;
class UAwardShowDlg;

class UMissionSelectDlg;
class UCampaignSelectDlg;
class UCmdMissionsDlg;
class UCmdDlg;

class UMissionEditorDlg;
class UMissionElementDlg;
class UMissionEventDlg;
class UMissionEditorNavDlg;

class ULoadDlg;
class UTacRefDlg;

class UOptionsScreen;

// ------------------------------------------------------------

UCLASS()
class STARSHATTERWARS_API UMenuScreen : public UBaseScreen
{
    GENERATED_BODY()

public:
    
    void Initialize(UGameInstance* InGI);
    UMenuScreen(const FObjectInitializer& ObjectInitializer);

    // ------------------------------------------------------------
    // UUserWidget / BaseScreen overrides
    // ------------------------------------------------------------

    virtual void NativeConstruct() override;
    virtual void ExecFrame(double DeltaTime) override;

    virtual void Show() override;
    virtual void Hide() override;

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

    // ------------------------------------------------------------
    // Setup / teardown
    // ------------------------------------------------------------

    void Setup();
    void TearDown();

    // ------------------------------------------------------------
    // Dialog routing API (called by dialogs)
    // ------------------------------------------------------------

    void ShowMenuDlg();
    void ShowCampaignSelectDlg();
    void ShowMissionSelectDlg();
    void ShowMissionEditorDlg();
    void ShowOperationsDlg();

    void ShowMsnElemDlg();
    void HideMsnElemDlg();

    void ShowMissionEventDlg();
    void HideMsnEventDlg();

    void ShowNavDlg();
    void HideNavDlg();
    bool IsNavShown() const;

    void ShowFirstTimeDlg();
    void ShowPlayerDlg();
    void ShowTacRefDlg();
    void ShowAwardDlg();

    void ShowExitDlg();
    void ShowConfirmDlg();
    void HideConfirmDlg();

    void ShowLoadDlg();
    void HideLoadDlg();

    // Options hub
    void ShowOptionsScreen();
    void HideOptionsScreen();
    void ReturnFromOptions();

    void ReturnFromPlayerDlg();

    UTacRefDlg* GetTacRefDlg() const { return TacRefDlg; }
    UOptionsScreen* GetOptionsScreen() const { return OptionsScreen; }
    UMissionEditorDlg* GetMsnEditDlg() const { return MsnEditDlg; }

    UBaseScreen* GetCurrentDialog() const { return CurrentDialog; }

    UMenuDlg* GetMenuDlg() const { return MenuDlg; }
    ULoadDlg* GetLoadDlg() const { return LoadDlg; }

    // ------------------------------------------------------------
    // Close / back navigation
    // ------------------------------------------------------------

    bool CloseTopmost();

protected:
    // ------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------

    template<typename TDialog>
    TDialog* EnsureDialog(TSubclassOf<TDialog> ClassToSpawn, TObjectPtr<TDialog>& Storage);

    void ShowDialog(UBaseScreen* Dialog, int32 ZOrder);
    void HideDialog(UBaseScreen* Dialog);
    void HideAll();

protected:
    // ------------------------------------------------------------
    // Class references (set in MenuScreen BP)
    // ------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UMenuDlg> MenuDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UExitDlg> ExitDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UConfirmDlg> ConfirmDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UFirstTimeDlg> FirstTimeDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UPlayerDlg> PlayerDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UAwardShowDlg> AwardDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UMissionSelectDlg> MsnSelectDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UCampaignSelectDlg> CmpSelectDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UCmdMissionsDlg> CmdMissionsDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UCmdDlg> CmdDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UMissionEditorDlg> MsnEditDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UMissionElementDlg> MsnElemDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UMissionEventDlg> MsnEventDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UMissionEditorNavDlg> MsnEditNavDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<ULoadDlg> LoadDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UTacRefDlg> TacRefDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UOptionsScreen> OptionsScreenClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UCampaignSelectDlg> CampaignSelectScreenClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UCampaignSelectDlg> MissionSelectScreenClass;

    UPROPERTY(EditDefaultsOnly, Category = "Menu|Classes")
    TSubclassOf<UCmdDlg> OperationsScreenClass;

    template<typename TWidget>
    TSubclassOf<TWidget> ResolveWidgetOrLog(UStarshatterAssetRegistrySubsystem* Assets, FName Id)
    {
        if (!Assets)
            return nullptr;

        TSubclassOf<UUserWidget> Raw = Assets->GetWidgetClass(Id, true);
        if (!Raw)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[UI] ResolveWidgetOrLog failed for AssetId=%s"),
                *Id.ToString());
            return nullptr;
        }

        // Typed cast (safe runtime check):
        TSubclassOf<TWidget> Typed = Raw.Get();
        if (!Typed)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[UI] AssetId=%s resolved to %s but is not a %s"),
                *Id.ToString(),
                *GetNameSafe(Raw.Get()),
                *TWidget::StaticClass()->GetName());
        }

        return Typed;
    }

protected:
    // ------------------------------------------------------------
    // Dialog instances (GC-safe)
    // ------------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UMenuDlg> MenuDlg;

    UPROPERTY()
    TObjectPtr<UExitDlg> ExitDlg;

    UPROPERTY()
    TObjectPtr<UConfirmDlg> ConfirmDlg;

    UPROPERTY()
    TObjectPtr<UFirstTimeDlg> FirstTimeDlg;

    UPROPERTY()
    TObjectPtr<UPlayerDlg> PlayerDlg;

    UPROPERTY()
    TObjectPtr<UAwardShowDlg> AwardDlg;

    UPROPERTY()
    TObjectPtr<UMissionSelectDlg> MissionSelectDlg;

    UPROPERTY()
    TObjectPtr<UCmdMissionsDlg> CmdMissionsDlg;

    UPROPERTY()
    TObjectPtr<UCmdDlg> CmdDlg;

    UPROPERTY()
    TObjectPtr<UCampaignSelectDlg> CmpSelectDlg;

    UPROPERTY()
    TObjectPtr<UMissionEditorDlg> MsnEditDlg;

    UPROPERTY()
    TObjectPtr<UMissionElementDlg> MsnElemDlg;

    UPROPERTY()
    TObjectPtr<UMissionEventDlg> MsnEventDlg;

    UPROPERTY()
    TObjectPtr<UMissionEditorNavDlg> MsnEditNavDlg;

    UPROPERTY()
    TObjectPtr<ULoadDlg> LoadDlg;

    UPROPERTY()
    TObjectPtr<UTacRefDlg> TacRefDlg;

    UPROPERTY()
    TObjectPtr<UOptionsScreen> OptionsScreen;



protected:
    // ------------------------------------------------------------
    // State
    // ------------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UBaseScreen> CurrentDialog;

    int32 ZCounter = 0;
    bool  bIsShown = false;

    //TSubclassOf<UUserWidget> MenuScreenWidgetClass;
    //TSubclassOf<UUserWidget> FirstTimeDlgWidgetClass;
    //TSubclassOf<UUserWidget> QuitDlgWidgetClass;
};


