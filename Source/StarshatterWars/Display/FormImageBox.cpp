#include "FormImageBox.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

UFormImageBox::UFormImageBox(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Default brush
    Brush = FSlateBrush();
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.TintColor = FSlateColor(Tint);

    bIsVariable = true;
}

void UFormImageBox::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    ImageWidget.Reset();
    ButtonWidget.Reset();
}

void UFormImageBox::Clear()
{
    Texture = nullptr;
    Brush.SetResourceObject(nullptr);
    SynchronizeProperties();
}

void UFormImageBox::SetTexture(UTexture2D* InTexture)
{
    Texture = InTexture;
    Brush.SetResourceObject(Texture);
    Brush.DrawAs = ESlateBrushDrawType::Image;
    SynchronizeProperties();
}

void UFormImageBox::SetBrush(const FSlateBrush& InBrush)
{
    Brush = InBrush;
    SynchronizeProperties();
}

void UFormImageBox::SetTint(const FLinearColor& InTint)
{
    Tint = InTint;
    SynchronizeProperties();
}

void UFormImageBox::SetClickable(bool bInClickable)
{
    if (bClickable != bInClickable)
    {
        bClickable = bInClickable;

        // Clickable changes the Slate hierarchy (SImage vs SButton->SImage):
        InvalidateLayoutAndVolatility();
    }
}

void UFormImageBox::SetScaleToFit(bool bInScaleToFit)
{
    bScaleToFit = bInScaleToFit;
    SynchronizeProperties();
}

const FSlateBrush* UFormImageBox::GetBrushPtr() const
{
    return &Brush;
}

TSharedRef<SWidget> UFormImageBox::RebuildWidget()
{
    // Create image
    ImageWidget =
        SNew(SImage)
        .Image(TAttribute<const FSlateBrush*>::Create(
            TAttribute<const FSlateBrush*>::FGetter::CreateUObject(
                this, &UFormImageBox::GetBrushPtr)))
        .ColorAndOpacity(Tint);

    if (bClickable)
    {
        ButtonWidget =
            SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
            .ContentPadding(FMargin(0.f))
            .OnClicked(FOnClicked::CreateUObject(this, &UFormImageBox::HandleClicked))
            [
                ImageWidget.ToSharedRef()
            ];

        return ButtonWidget.ToSharedRef();
    }

    return ImageWidget.ToSharedRef();
}

void UFormImageBox::SynchronizeProperties()
{
    Super::SynchronizeProperties();

    // Keep brush resource synced with texture if texture is set:
    if (Texture)
    {
        Brush.SetResourceObject(Texture);
        Brush.DrawAs = ESlateBrushDrawType::Image;
    }

    Brush.TintColor = FSlateColor(Tint);

    // Push tint to Slate if built:
    if (ImageWidget.IsValid())
    {
        ImageWidget->SetColorAndOpacity(Tint);
    }
}

FReply UFormImageBox::HandleClicked()
{
    OnClicked.Broadcast();
    return FReply::Handled();
}
