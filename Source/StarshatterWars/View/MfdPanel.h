#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MfdPanel.generated.h"

UCLASS()
class STARSHATTERWARS_API UMfdPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetMfdIndex(int32 InIndex) { MfdIndex = InIndex; }
    void BuildIfNeeded();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    void HandleCorner(int32 Corner, bool bRight);

    UPROPERTY() int32 MfdIndex = 0;
    UPROPERTY() TObjectPtr<class UButton> ModeButton;
    UPROPERTY() TObjectPtr<class UButton> RangeButton;
    UPROPERTY() TObjectPtr<class UButton> DispButton;
    UPROPERTY() TObjectPtr<class UButton> ProbeButton;

    UFUNCTION() void OnMode();
    UFUNCTION() void OnRange();
    UFUNCTION() void OnDisp();
    UFUNCTION() void OnProbe();
};
