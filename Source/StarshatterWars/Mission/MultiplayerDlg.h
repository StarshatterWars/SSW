#pragma once
#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Components/Button.h"
#include "MultiplayerDlg.generated.h"
class UMultiplayerDlg;class UVerticalBox;class UTextBlock;class UEditableTextBox;class USpinBox;class UCheckBox;class USSWSessionSubsystem;
UCLASS()
class STARSHATTERWARS_API USSWSessionRowButton:public UButton {
 GENERATED_BODY()
public:
 void Configure(UMultiplayerDlg* Screen,int32 Row);
private:
 UFUNCTION()void Clicked();
 UPROPERTY()TWeakObjectPtr<UMultiplayerDlg> Dialog;
 int32 Index=INDEX_NONE;
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
protected:
 virtual TSharedRef<SWidget> RebuildWidget()override;
 virtual void NativeConstruct()override;
 virtual void NativeDestruct()override;
 void BuildSessionScreen();
 UTextBlock* Label(const FString& Text,int32 Size=18);
 UButton* Button(const FString& Text,UVerticalBox* Parent);
 USSWSessionSubsystem* Service()const;
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

 UPROPERTY(Transient)TObjectPtr<UTextBlock> StatusLabel;
 UPROPERTY(Transient)TObjectPtr<UEditableTextBox> SessionName;
 UPROPERTY(Transient)TObjectPtr<USpinBox> Capacity;
 UPROPERTY(Transient)TObjectPtr<UCheckBox> LAN;
 UPROPERTY(Transient)TObjectPtr<UButton> CreateButton;
 UPROPERTY(Transient)TObjectPtr<UButton> SearchButton;
 UPROPERTY(Transient)TObjectPtr<UButton> JoinButton;
 UPROPERTY(Transient)TObjectPtr<UButton> LeaveButton;
 UPROPERTY(Transient)TObjectPtr<UButton> CancelSearchButton;

 int32 Selected=INDEX_NONE;
 bool bHostMode=true;
};
