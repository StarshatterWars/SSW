#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"
#include "FormImageBox.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFormImageBoxClicked);

UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UFormImageBox : public UWidget
{
    GENERATED_BODY()

public:
    UFormImageBox(const FObjectInitializer& ObjectInitializer);

    // Legacy-like API:
    UFUNCTION(BlueprintCallable, Category = "Form|ImageBox")
    void Clear();

    UFUNCTION(BlueprintCallable, Category = "Form|ImageBox")
    void SetTexture(UTexture2D* InTexture);

    UFUNCTION(BlueprintCallable, Category = "Form|ImageBox")
    void SetBrush(const FSlateBrush& InBrush);

    UFUNCTION(BlueprintCallable, Category = "Form|ImageBox")
    void SetTint(const FLinearColor& InTint);

    UFUNCTION(BlueprintCallable, Category = "Form|ImageBox")
    void SetClickable(bool bInClickable);

    UFUNCTION(BlueprintCallable, Category = "Form|ImageBox")
    void SetScaleToFit(bool bInScaleToFit);

public:
    UPROPERTY(BlueprintAssignable, Category = "Form|ImageBox")
    FFormImageBoxClicked OnClicked;

protected:
    // UWidget
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    // Slate callbacks (UObject-safe bindings)
    FReply HandleClicked();

    // Attribute getter (UObject-safe)
    const FSlateBrush* GetBrushPtr() const;

private:
    // Backing data
    UPROPERTY(EditAnywhere, Category = "Form|ImageBox")
    TObjectPtr<UTexture2D> Texture = nullptr;

    UPROPERTY(EditAnywhere, Category = "Form|ImageBox")
    FSlateBrush Brush;

    UPROPERTY(EditAnywhere, Category = "Form|ImageBox")
    FLinearColor Tint = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Form|ImageBox")
    bool bClickable = false;

    UPROPERTY(EditAnywhere, Category = "Form|ImageBox")
    bool bScaleToFit = true;

private:
    // Slate widgets
    TSharedPtr<class SImage>  ImageWidget;
    TSharedPtr<class SButton> ButtonWidget;
};
