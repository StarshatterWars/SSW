/*=============================================================================
    Project:        Starshatter Wars (Unreal Engine Port)
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:      UI Style
    FILE:           StarshatterUIStyleSubsystem.cpp
    AUTHOR:         Carlos Bott
=============================================================================*/

#include "StarshatterUIStyleSubsystem.h"

#include "StarshatterUIStyleSettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/ContentWidget.h"
#include "MenuButton.h"
#include "Kismet/GameplayStatics.h"

#include "Engine/Font.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY(LogStarshatterUIStyle);

static FSlateBrush MakeUIButtonBrush(UTexture2D* Tex, const FMargin& NineSliceMargin, const FVector2D& ImageSize)
{
    FSlateBrush B;

    if (Tex)
    {
        B.SetResourceObject(Tex);
        B.DrawAs = ESlateBrushDrawType::Box;     // nine-slice requires Box
        B.Margin = NineSliceMargin;              // must be non-zero (0..1 normalized)

        if (!ImageSize.IsNearlyZero())
            B.ImageSize = ImageSize;
    }
    else
    {
        // Safe fallback
        B.DrawAs = ESlateBrushDrawType::Box;
        B.Margin = FMargin(4.f / 16.f);
        B.TintColor = FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
    }

    return B;
}

void UStarshatterUIStyleSubsystem::ApplyButtonStyleInternal(
    UButton* Button,
    UTexture2D* NormalTex,
    UTexture2D* HoverTex,
    UTexture2D* PressedTex,
    UTexture2D* DisabledTex,
    const FMargin& NineSliceMargin,
    const FVector2D& ImageSize,
    UFont* Font,
    int32 FontSize,
    const FLinearColor& TextColor
)
{
    if (!IsValid(Button))
        return;

    // --------------------------------------------------------------------
    // 1) Build 9-slice brushes (DrawAs=Box + Margin)
    // --------------------------------------------------------------------
    const FSlateBrush Normal = MakeUIButtonBrush(NormalTex, NineSliceMargin, ImageSize);
    const FSlateBrush Hovered = MakeUIButtonBrush(HoverTex, NineSliceMargin, ImageSize);
    const FSlateBrush Pressed = MakeUIButtonBrush(PressedTex, NineSliceMargin, ImageSize);
    const FSlateBrush Disabled = MakeUIButtonBrush(DisabledTex ? DisabledTex : NormalTex, NineSliceMargin, ImageSize);

    // --------------------------------------------------------------------
    // 2) Apply brushes + default sounds via SlateSound
    //    (NO UButton delegates, no lambdas, no Clear())
    // --------------------------------------------------------------------
    FButtonStyle Style = Button->WidgetStyle;
    Style.SetNormal(Normal);
    Style.SetHovered(Hovered);
    Style.SetPressed(Pressed);
    Style.SetDisabled(Disabled);

    // Default UI sounds (set these in Project Settings via your Settings object,
    // loaded into this subsystem on boot; stored as USoundBase* here)
    if (IsValid(ButtonHoverSound))
    {
        FSlateSound HoverSfx;
        HoverSfx.SetResourceObject(ButtonHoverSound);
        Style.SetHoveredSound(HoverSfx);
    }

    if (IsValid(ButtonClickSound))
    {
        FSlateSound ClickSfx;
        ClickSfx.SetResourceObject(ButtonClickSound);
        Style.SetPressedSound(ClickSfx);
    }

    Button->SetStyle(Style);

    // Force UMG to push the new Slate style into the underlying SButton
    Button->SynchronizeProperties();
    Button->InvalidateLayoutAndVolatility();

    // --------------------------------------------------------------------
    // 3) Update the first text block under the button (if any)
    // --------------------------------------------------------------------
    UTextBlock* TextChild = FindFirstTextBlockRecursive(Button);
    if (!TextChild)
        return;

    TextChild->SetJustification(ETextJustify::Center);
    TextChild->SetColorAndOpacity(FSlateColor(TextColor));

    if (IsValid(Font))
    {
        FSlateFontInfo FontInfo = TextChild->GetFont();
        FontInfo.FontObject = Font;
        FontInfo.Size = FontSize;
        TextChild->SetFont(FontInfo);
    }

    TextChild->SynchronizeProperties();
}

void UStarshatterUIStyleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadFromSettings(true);
    UE_LOG(LogStarshatterUIStyle, Log, TEXT("[UISTYLE] Initialize"));
}

void UStarshatterUIStyleSubsystem::Deinitialize()
{
    UE_LOG(LogStarshatterUIStyle, Log, TEXT("[UISTYLE] Deinitialize"));
    Super::Deinitialize();
}

void UStarshatterUIStyleSubsystem::ReloadFromSettings(bool bLoadNow)
{
    const UStarshatterUIStyleSettings* S = GetDefault<UStarshatterUIStyleSettings>();
    if (!S)
    {
        UE_LOG(LogStarshatterUIStyle, Error, TEXT("[UISTYLE] Settings missing"));
        return;
    }

    auto LoadTex = [bLoadNow](const TSoftObjectPtr<UTexture2D>& Ptr) -> UTexture2D*
        {
            if (Ptr.IsNull()) return nullptr;
            return bLoadNow ? Ptr.LoadSynchronous() : Ptr.Get();
        };

    auto LoadFont = [bLoadNow](const TSoftObjectPtr<UFont>& Ptr) -> UFont*
        {
            if (Ptr.IsNull()) return nullptr;
            return bLoadNow ? Ptr.LoadSynchronous() : Ptr.Get();
        };

    const UStarshatterUIStyleSettings* Settings =
        GetDefault<UStarshatterUIStyleSettings>();

    if (!Settings)
        return;

    if (bLoadNow)
    {
        ButtonHoverSound = Settings->ButtonHoverSound.LoadSynchronous();
        ButtonClickSound = Settings->ButtonClickSound.LoadSynchronous();
    }

    ButtonSoundVolume = Settings->ButtonSoundVolume;

    DefaultUIFont = LoadFont(S->DefaultUIFont);
    DefaultTextColor = S->DefaultTextColor;

    TitleUIFont = LoadFont(S->TitleUIFont);
    TitleTextColor = S->TitleTextColor;

    Btn_NormalTex = LoadTex(S->Btn_NormalTex);
    Btn_HoverTex = LoadTex(S->Btn_HoverTex);
    Btn_PressedTex = LoadTex(S->Btn_PressedTex);
    Btn_DisabledTex = LoadTex(S->Btn_DisabledTex);
    Btn_9SliceMargin = S->Btn_9SliceMargin;
    Btn_ImageSize = S->Btn_ImageSize;

    MenuBtn_NormalTex = LoadTex(S->MenuBtn_NormalTex);
    MenuBtn_HoverTex = LoadTex(S->MenuBtn_HoverTex);
    MenuBtn_PressedTex = LoadTex(S->MenuBtn_PressedTex);
    MenuBtn_DisabledTex = LoadTex(S->MenuBtn_DisabledTex);
    MenuBtn_9SliceMargin = S->MenuBtn_9SliceMargin;

    MenuButtonFontSize = S->MenuButtonFontSize;
    MenuButtonTextColor = S->MenuButtonTextColor;

    UE_LOG(LogStarshatterUIStyle, Log, TEXT("[UISTYLE] Reloaded from Project Settings"));
}

FSlateBrush UStarshatterUIStyleSubsystem::MakeBrush(UTexture2D* Tex, const FMargin& Margin, const FVector2D& ImageSize)
{
    FSlateBrush B;

    if (Tex)
    {
        B.SetResourceObject(Tex);
        B.DrawAs = ESlateBrushDrawType::Box; // 9-slice
        B.Margin = Margin;

        if (!ImageSize.IsNearlyZero())
            B.ImageSize = ImageSize;
    }
    else
    {
        B.DrawAs = ESlateBrushDrawType::Box;
        B.TintColor = FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
        B.Margin = FMargin(4.f / 16.f);
    }

    return B;
}

UTextBlock* UStarshatterUIStyleSubsystem::FindFirstTextBlockRecursive(const UWidget* Root)
{
    if (!Root)
        return nullptr;

    if (const UTextBlock* TB = Cast<UTextBlock>(Root))
        return const_cast<UTextBlock*>(TB);

    if (const UContentWidget* CW = Cast<UContentWidget>(Root))
        return FindFirstTextBlockRecursive(CW->GetContent());

    if (const UPanelWidget* PW = Cast<UPanelWidget>(Root))
    {
        const int32 N = PW->GetChildrenCount();
        for (int32 i = 0; i < N; ++i)
        {
            if (UTextBlock* Found = FindFirstTextBlockRecursive(PW->GetChildAt(i)))
                return Found;
        }
    }

    return nullptr;
}

void UStarshatterUIStyleSubsystem::ApplyDefaultButtonStyle(UButton* Button, int32 FontSize, FLinearColor ButtonColor)
{
    ApplyButtonStyleInternal(
        Button,
        Btn_NormalTex,
        Btn_HoverTex,
        Btn_PressedTex,
        Btn_DisabledTex,
        Btn_9SliceMargin,
        Btn_ImageSize,
        DefaultUIFont,
        FontSize,
        ButtonColor
    );
}

void UStarshatterUIStyleSubsystem::ApplyDefaultButtonStyle_Override(
    UButton* Button,
    int32 FontSize,
    bool bOverrideTextColor,
    FLinearColor OverrideTextColor,
    UFont* OverrideFont
)
{
    const FLinearColor FinalColor = bOverrideTextColor ? OverrideTextColor : DefaultTextColor;
    UFont* FinalFont = OverrideFont ? OverrideFont : DefaultUIFont.Get();

    ApplyButtonStyleInternal(
        Button,
        Btn_NormalTex,
        Btn_HoverTex,
        Btn_PressedTex,
        Btn_DisabledTex,
        Btn_9SliceMargin,
        Btn_ImageSize,
        FinalFont,
        FontSize,
        FinalColor
    );
}

void UStarshatterUIStyleSubsystem::ApplyMenuButtonStyle(UButton* Button)
{
    // Menu uses TitleUIFont if set, else DefaultUIFont.
    UFont* FontToUse = TitleUIFont ? TitleUIFont : DefaultUIFont;

    ApplyButtonStyleInternal(
        Button,
        MenuBtn_NormalTex,
        MenuBtn_HoverTex,
        MenuBtn_PressedTex,
        MenuBtn_DisabledTex,
        MenuBtn_9SliceMargin,
        FVector2D::ZeroVector,      // DO NOT force size for Box brushes
        FontToUse,
        MenuButtonFontSize,
        MenuButtonTextColor
    );
}

void UStarshatterUIStyleSubsystem::ApplyMenuListButtonStyle(UMenuButton* Button)
{
    // Menu uses TitleUIFont if set, else DefaultUIFont.
    UFont* FontToUse = TitleUIFont ? TitleUIFont : DefaultUIFont;

    ApplyButtonStyleInternal(
        (UButton*)Button,
        MenuBtn_NormalTex,
        MenuBtn_HoverTex,
        MenuBtn_PressedTex,
        MenuBtn_DisabledTex,
        MenuBtn_9SliceMargin,
        FVector2D::ZeroVector,      // DO NOT force size for Box brushes
        FontToUse,
        MenuButtonFontSize,
        MenuButtonTextColor
    );
}

void UStarshatterUIStyleSubsystem::ApplyDefaultTextStyle(UTextBlock* Text, int32 FontSize)
{
    if (!Text)
        return;

    Text->SetColorAndOpacity(FSlateColor(DefaultTextColor));

    if (DefaultUIFont)
    {
        FSlateFontInfo FI = Text->GetFont();
        FI.FontObject = DefaultUIFont;
        FI.Size = FontSize;
        Text->SetFont(FI);
    }
}

void UStarshatterUIStyleSubsystem::ApplyTitleTextStyle(UTextBlock* Text, int32 FontSize)
{
    if (!Text)
        return;

    Text->SetColorAndOpacity(FSlateColor(TitleTextColor));

    UFont* FontToUse = TitleUIFont ? TitleUIFont : DefaultUIFont;
    if (FontToUse)
        Text->SetFont(FSlateFontInfo(FontToUse, FontSize));
}

void UStarshatterUIStyleSubsystem::ApplyDefaultEditBoxStyle(UEditableTextBox* Edit, int32 FontSize)
{
    if (!Edit)
        return;

    FEditableTextBoxStyle& Style = Edit->WidgetStyle;

    if (DefaultUIFont)
    {
        Style.TextStyle.SetFont(FSlateFontInfo(DefaultUIFont, FontSize));
        Style.TextStyle.SetColorAndOpacity(FSlateColor(DefaultTextColor));
    }

    FSlateBrush Bg;
    Bg.DrawAs = ESlateBrushDrawType::Box;
    Bg.TintColor = FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
    Bg.Margin = FMargin(4.f / 16.f);

    Style.SetBackgroundImageNormal(Bg);
    Style.SetBackgroundImageHovered(Bg);
    Style.SetBackgroundImageFocused(Bg);
    Style.SetBackgroundImageReadOnly(Bg);

    Edit->SynchronizeProperties();
}

