#pragma once
#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "QuitMissionMenu.generated.h"

class UButton;
class UTextBlock;

UENUM(BlueprintType)
enum class EMissionQuitAction : uint8
{
    None = 0, Accept = 1, Abort = 2, Resume = 3, Controls = 4
};

DECLARE_DELEGATE_OneParam(FMissionQuitActionRequested, EMissionQuitAction);

// Parent class for WBP_QuitMissionMenu. QuitView owns pause and mission actions.
UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UQuitMissionMenu : public UBaseScreen
{
    GENERATED_BODY()
public:
    UQuitMissionMenu(const FObjectInitializer& ObjectInitializer);
    FMissionQuitActionRequested OnActionRequested;
    UFUNCTION(BlueprintCallable, Category="Mission Menu") void AcceptMission();
    UFUNCTION(BlueprintCallable, Category="Mission Menu") void AbortMission();
    UFUNCTION(BlueprintCallable, Category="Mission Menu") void ResumeMission();
    UFUNCTION(BlueprintCallable, Category="Mission Menu") void OpenControls();
    UFUNCTION(BlueprintCallable, Category="Mission Menu") void SetStatusMessage(const FText& Message);
    UPROPERTY(BlueprintReadOnly, Category="Mission Menu") FText StatusMessage;
    UFUNCTION(BlueprintImplementableEvent, Category="Mission Menu") void OnStatusMessageChanged(const FText& Message);
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> AcceptBtn;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> AbortBtn;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ResumeBtn;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ControlsBtn;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
};
