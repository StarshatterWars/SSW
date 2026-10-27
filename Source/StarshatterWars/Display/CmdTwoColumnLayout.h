#pragma once
// Layout-only helpers. Never construct or replace existing Blueprint ListView objects.
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

namespace SSWTwoColumn
{
inline UCanvasPanel* FindCanvas(UUserWidget* Owner)
{
    if (!Owner || !Owner->WidgetTree) return nullptr;
    if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Owner->GetWidgetFromName(TEXT("CanvasRoot")))) return Canvas;
    return Cast<UCanvasPanel>(Owner->WidgetTree->RootWidget);
}
inline void FullCanvas(UCanvasPanel* Canvas, UWidget* Child, int32 Layer)
{
    if (!Canvas || !Child) return;
    if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Child))
    {
        Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        Slot->SetOffsets(FMargin(0.f));
        Slot->SetZOrder(Layer);
    }
}
inline UTextBlock* Label(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Text, int32 Size=18)
{
    UTextBlock* Result = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
    Result->SetText(FText::FromString(Text));
    Result->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.94f, 0.97f, 1.f)));
    FSlateFontInfo Font = Result->GetFont(); Font.Size = Size; Result->SetFont(Font);
    Result->SetAutoWrapText(true);
    return Result;
}
inline void White(UTextBlock* Target, int32 Size=18)
{
    if (!Target) return;
    Target->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f,0.94f,0.97f,1.f)));
    FSlateFontInfo Font = Target->GetFont(); Font.Size = Size; Target->SetFont(Font);
    Target->SetAutoWrapText(true);
}
inline void Append(UVerticalBox* Box, UWidget* Widget, bool Fill=false)
{
    if (!Box || !Widget) return;
    Widget->RemoveFromParent();
    if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Widget))
    {
        Slot->SetPadding(FMargin(8.f, 5.f));
        Slot->SetHorizontalAlignment(HAlign_Fill);
        if (Fill)
        {
            FSlateChildSize FillSize;
            FillSize.SizeRule = ESlateSizeRule::Fill;
            FillSize.Value = 1.0f;
            Slot->SetSize(FillSize);
        }
    }
}
inline UHorizontalBox* Begin(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Prefix)
{
    UImage* Background = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(FString(Prefix)+TEXT("Background")));
    if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Panel.Panel")))
        Background->SetBrushFromTexture(Texture, true);
    else UE_LOG(LogTemp, Warning, TEXT("[SSWTwoColumn] Background /Game/UI/Panel.Panel not found"));
    FullCanvas(Canvas, Background, 0);
    UHorizontalBox* Columns = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(FString(Prefix)+TEXT("Columns")));
    FullCanvas(Canvas, Columns, 1);
    return Columns;
}
inline UVerticalBox* Column(UWidgetTree* Tree, UHorizontalBox* Columns, const TCHAR* Name, float Weight)
{
    UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(Name));
    if (UHorizontalBoxSlot* Slot = Columns->AddChildToHorizontalBox(Box))
    {
        Slot->SetPadding(FMargin(15.f, 12.f));
        FSlateChildSize ColumnSize;
        ColumnSize.SizeRule = ESlateSizeRule::Fill;
        ColumnSize.Value = Weight;
        Slot->SetSize(ColumnSize);
        Slot->SetVerticalAlignment(VAlign_Fill);
    }
    return Box;
}
inline UScrollBox* Scroller(UWidgetTree* Tree, const TCHAR* Name)
{
    return Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName(Name));
}
}
