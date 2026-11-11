#pragma once
#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Components/Button.h"
#include "MultiplayerDlg.generated.h"
class UScrollBox;
class UMultiplayerDlg;class UVerticalBox;class UTextBlock;class UEditableTextBox;class USpinBox;class UCheckBox;class USSWSessionSubsystem;
UCLASS()
class STARSHATTERWARS_API USSWSessionRowButton:public UButton {
 GENERATED_BODY()
public:
 void Configure(UMultiplayerDlg* Screen,int32 Row,bool bMissionRow = false);
private:
 UFUNCTION()void Clicked();
 UPROPERTY()TWeakObjectPtr<UMultiplayerDlg> Dialog;
 int32 Index=INDEX_NONE;
 bool bMission = false;
};
UCLASS()
class STARSHATTERWARS_API UMultiplayerDlg:public UBaseScreen {
 GENERATED_BODY()
public:
 UMultiplayerDlg(const FObjectInitializer& Initializer);
 virtual void Show() override;
 void InitializeDlg(class UMenuScreen* InManager){SetMenuManager(InManager);}
 virtual void HandleAccept() override;
 virtual void HandleCancel() override;
 bool HasSelectedMission()const{return false;} // Sessions never use legacy briefing handoff.
 void SelectSession(int32 Index);
 void HostMission(int32 Index);
protected:
 virtual TSharedRef<SWidget> RebuildWidget()override;
 virtual void NativeConstruct()override;
 virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
 virtual void NativeDestruct()override;
 void BuildSessionScreen();
 void BuildLobbyScreen(UVerticalBox* Main);
 UTextBlock* Label(const FString& Text,int32 Size=18);
 UButton* Button(const FString& Text,UVerticalBox* Parent);
 void AddMainMenuButton(UVerticalBox* Parent);
 USSWSessionSubsystem* Service()const;
 void RefreshChat();
 void RefreshRoster();
 UFUNCTION()void ToggleReady();
 UPROPERTY(Transient)TObjectPtr<UScrollBox> LobbyRoster;
 UPROPERTY(Transient)TObjectPtr<UTextBlock> ReadyLabel;
 TWeakObjectPtr<class ASSWSessionGameState> ObservedLobby;
 UPROPERTY(Transient)TObjectPtr<UTextBlock> MissionCountdownLabel;
 int32 DisplayedCountdown = INDEX_NONE;
 UFUNCTION()void SubmitChat(const FText& Text, ETextCommit::Type CommitMethod);
 UFUNCTION()void RefreshState();
 UFUNCTION()void HostMode();
 UFUNCTION()void JoinMode();
 UFUNCTION()void Create();
 UFUNCTION()void Search();
 UFUNCTION()void Join();
 UFUNCTION()void Leave();
 UFUNCTION()void Back();
 UFUNCTION()void CancelSearch();
 UPROPERTY(Transient)TObjectPtr<UVerticalBox> HostPanel;
 UPROPERTY(Transient)TObjectPtr<UVerticalBox> JoinPanel;
 UPROPERTY(Transient)TObjectPtr<UVerticalBox> Rows;

 UPROPERTY(Transient)TObjectPtr<UScrollBox> ChatLog;
 UPROPERTY(Transient)TObjectPtr<UEditableTextBox> ChatEntry;
 UPROPERTY(Transient)TObjectPtr<UTextBlock> StatusLabel;
 UPROPERTY(Transient)TObjectPtr<UEditableTextBox> SessionName;
 UPROPERTY(Transient)TObjectPtr<USpinBox> Capacity;
 UPROPERTY(Transient)TObjectPtr<UCheckBox> LAN;
 UPROPERTY(Transient)TArray<TObjectPtr<UButton>> MissionButtons;
 UPROPERTY(Transient)TObjectPtr<UButton> SearchButton;
 UPROPERTY(Transient)TObjectPtr<UButton> JoinButton;
 UPROPERTY(Transient)TObjectPtr<UButton> LeaveButton;
 UPROPERTY(Transient)TObjectPtr<UButton> CancelSearchButton;

 UPROPERTY(Transient)TObjectPtr<UButton> HostTab;
 UPROPERTY(Transient)TObjectPtr<UButton> JoinTab;
 friend class FNativeSessionUITest;
 int32 Selected=INDEX_NONE;
 bool bHostMode=true;
 bool bModeSelected=false;
};
