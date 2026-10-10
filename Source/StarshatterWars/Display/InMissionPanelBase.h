#pragma once
#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "InMissionPanelBase.generated.h"
class USizeBox;
class UTextBlock;
class UNativeWidgetHost;
class Ship;

UCLASS(Abstract, Blueprintable)
class STARSHATTERWARS_API UInMissionPanelBase : public UBaseScreen
{
    GENERATED_BODY()
public:
    FSimpleDelegate OnPanelClosed;
    UFUNCTION(BlueprintCallable, Category="Mission Panel") void RequestPanelClose();
protected:
    UPROPERTY(meta=(BindWidgetOptional)) USizeBox* RuntimeHost=nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* PanelTitle=nullptr;
    UPROPERTY(Transient) UNativeWidgetHost* GeneratedContent=nullptr;
    UPROPERTY(EditDefaultsOnly, Category="Mission Panel") FMargin ContentInsets=FMargin(24,96,24,24);
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry&,const FKeyEvent&) override;
    virtual TSharedRef<SWidget> CreatePanelContent();
    virtual FText GetPanelCaption() const;
    Ship* ResolvePanelShip() const;
};
