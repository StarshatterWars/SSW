/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2024-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI
    FILE:         SystemMapPanel.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    System-level map panel for Mission Navigation.

    - Resolves the selected system from UStarshatterEnvironmentSubsystem
    - Uses FS_Galaxy::Stellar as the map hierarchy source
    - Draws the primary star using the same texture family as GalaxyMapPanel
    - Tints the surrounding ring using FS_Galaxy::Iff
*/

#include "SystemMapPanel.h"

#include "MissionUIStyle.h"
#include "StarshatterEnvironmentSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

void USystemMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    BuildLayout();

    StarTextureCache.Empty();

    auto LoadMapTexture = [](const TCHAR* AssetPath) -> UTexture2D*
        {
            if (!AssetPath || !*AssetPath)
            {
                return nullptr;
            }

            UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, AssetPath);
            if (!Texture)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SystemMapPanel] Failed to load texture: %s"), AssetPath);
            }

            return Texture;
        };

    IFFRingTexture = LoadMapTexture(TEXT("/Game/UI/GalaxyMap/IFFRing.IFFRing"));

    StarTextureCache.Add(ESPECTRAL_CLASS::A, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarA_map.StarA_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::B, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarB_map.StarB_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::F, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarF_map.StarF_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::G, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarG_map.StarG_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::K, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarK_map.StarK_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::M, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarM_map.StarM_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::O, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarO_map.StarO_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::WHITE_DWARF, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/White.White")));
    StarTextureCache.Add(ESPECTRAL_CLASS::RED_GIANT, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarG_map.StarG_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::BLACK_HOLE, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/White.White")));

    RefreshView();
}

void USystemMapPanel::SetViewedSystemName(const FString& InSystemName)
{
    ViewedSystemName = InSystemName.TrimStartAndEnd();
    RefreshView();
    Invalidate(EInvalidateWidget::Paint);
}

void USystemMapPanel::BuildLayout()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!RootCanvas)
    {
        RootCanvas =
            WidgetTree->ConstructWidget<UCanvasPanel>(
                UCanvasPanel::StaticClass(),
                TEXT("SystemMapRootCanvas"));

        WidgetTree->RootWidget = RootCanvas;
    }

    if (!HeaderText)
    {
        HeaderText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("SystemMapHeaderText"));

        HeaderText->SetFont(MissionUIStyle::GetHeaderFont(16));
        HeaderText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        HeaderText->SetJustification(ETextJustify::Left);

        if (UCanvasPanelSlot* HeaderSlot = RootCanvas->AddChildToCanvas(HeaderText))
        {
            HeaderSlot->SetAnchors(FAnchors(0.f, 0.f));
            HeaderSlot->SetPosition(FVector2D(8.f, 8.f));
            HeaderSlot->SetSize(FVector2D(420.f, 24.f));
        }
    }

    if (!InfoText)
    {
        InfoText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("SystemMapInfoText"));

        InfoText->SetFont(MissionUIStyle::GetInfoValueFont());
        InfoText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
        InfoText->SetJustification(ETextJustify::Right);

        if (UCanvasPanelSlot* InfoSlot = RootCanvas->AddChildToCanvas(InfoText))
        {
            InfoSlot->SetAnchors(FAnchors(1.f, 0.f));
            InfoSlot->SetAlignment(FVector2D(1.f, 0.f));
            InfoSlot->SetPosition(FVector2D(-8.f, 8.f));
            InfoSlot->SetSize(FVector2D(320.f, 24.f));
        }
    }
}

bool USystemMapPanel::ResolveViewedGalaxy(FS_Galaxy& OutGalaxy) const
{
    OutGalaxy = FS_Galaxy();

    if (ViewedSystemName.IsEmpty())
    {
        return false;
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        return false;
    }

    UStarshatterEnvironmentSubsystem* Env =
        GI->GetSubsystem<UStarshatterEnvironmentSubsystem>();

    if (!Env)
    {
        return false;
    }

    for (const FS_Galaxy& GalaxyRow : Env->GalaxyDataArray)
    {
        if (GalaxyRow.Name.Equals(ViewedSystemName, ESearchCase::IgnoreCase))
        {
            OutGalaxy = GalaxyRow;
            return true;
        }
    }

    return false;
}

const FS_StarMap* USystemMapPanel::GetPrimaryStarMap(const FS_Galaxy& InGalaxy) const
{
    if (InGalaxy.Stellar.Num() > 0)
    {
        return &InGalaxy.Stellar[0];
    }

    return nullptr;
}

UTexture2D* USystemMapPanel::GetStarTextureForClass(ESPECTRAL_CLASS InClass) const
{
    if (const TObjectPtr<UTexture2D>* Found = StarTextureCache.Find(InClass))
    {
        return Found->Get();
    }

    if (const TObjectPtr<UTexture2D>* Fallback = StarTextureCache.Find(ESPECTRAL_CLASS::G))
    {
        return Fallback->Get();
    }

    return nullptr;
}

float USystemMapPanel::ComputeStarDrawSize(const FS_StarMap& InStar) const
{
    const float RawRadius = static_cast<float>(InStar.Radius);

    if (RawRadius <= 0.0f)
    {
        return 72.0f;
    }

    const float Visual = FMath::LogX(10.0f, RawRadius + 1.0f) * 14.0f;
    return FMath::Clamp(Visual, 56.0f, 180.0f);
}

float USystemMapPanel::ComputeRingDrawSize(const FS_StarMap& InStar) const
{
    return ComputeStarDrawSize(InStar) + 28.0f;
}

FLinearColor USystemMapPanel::ComputeStarTint(const FS_StarMap& InStar) const
{
    if (InStar.Color != FColor(0, 0, 0, 0))
    {
        return FLinearColor(InStar.Color);
    }

    switch (InStar.Class)
    {
    case ESPECTRAL_CLASS::O:
        return FLinearColor(0.72f, 0.82f, 1.00f, 1.0f);

    case ESPECTRAL_CLASS::B:
        return FLinearColor(0.78f, 0.86f, 1.00f, 1.0f);

    case ESPECTRAL_CLASS::A:
        return FLinearColor(0.88f, 0.93f, 1.00f, 1.0f);

    case ESPECTRAL_CLASS::F:
        return FLinearColor(1.00f, 0.98f, 0.85f, 1.0f);

    case ESPECTRAL_CLASS::G:
        return FLinearColor(1.00f, 0.91f, 0.30f, 1.0f);

    case ESPECTRAL_CLASS::K:
        return FLinearColor(1.00f, 0.72f, 0.24f, 1.0f);

    case ESPECTRAL_CLASS::M:
        return FLinearColor(1.00f, 0.42f, 0.22f, 1.0f);

    case ESPECTRAL_CLASS::WHITE_DWARF:
        return FLinearColor(0.92f, 0.96f, 1.00f, 1.0f);

    case ESPECTRAL_CLASS::RED_GIANT:
        return FLinearColor(1.00f, 0.40f, 0.20f, 1.0f);

    case ESPECTRAL_CLASS::BLACK_HOLE:
        return FLinearColor(0.55f, 0.55f, 0.65f, 1.0f);

    default:
        return FLinearColor(1.00f, 0.90f, 0.35f, 1.0f);
    }
}

FLinearColor USystemMapPanel::ComputeSystemIFFRingTint(const FS_Galaxy& InGalaxy) const
{
    switch (InGalaxy.Iff)
    {
    case 1:
        return FLinearColor(0.20f, 1.00f, 0.20f, 0.90f);

    case 2:
        return FLinearColor(1.00f, 0.25f, 0.25f, 0.90f);

    case 3:
        return FLinearColor(1.00f, 0.85f, 0.25f, 0.90f);

    default:
        return FLinearColor(0.60f, 0.75f, 1.00f, 0.80f);
    }
}

void USystemMapPanel::RefreshView()
{
    bValidSystem = ResolveViewedGalaxy(CachedGalaxyRow);

    if (HeaderText)
    {
        HeaderText->SetText(FText::FromString(
            ViewedSystemName.IsEmpty()
            ? TEXT("SYSTEM")
            : FString::Printf(TEXT("SYSTEM: %s"), *ViewedSystemName.ToUpper())));
    }

    if (!bValidSystem)
    {
        CachedPrimaryStarMap = FS_StarMap();

        if (InfoText)
        {
            InfoText->SetText(FText::FromString(TEXT("NO SYSTEM")));
        }

        return;
    }

    const FS_StarMap* Star = GetPrimaryStarMap(CachedGalaxyRow);
    if (!Star)
    {
        CachedPrimaryStarMap = FS_StarMap();

        if (InfoText)
        {
            InfoText->SetText(FText::FromString(TEXT("NO STAR DATA")));
        }

        return;
    }

    CachedPrimaryStarMap = *Star;

    if (InfoText)
    {
        InfoText->SetText(FText::FromString(FString::Printf(
            TEXT("STAR: %s"),
            *CachedPrimaryStarMap.Name)));
    }
}

int32 USystemMapPanel::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    LayerId = Super::NativePaint(
        Args,
        AllottedGeometry,
        MyCullingRect,
        OutDrawElements,
        LayerId,
        InWidgetStyle,
        bParentEnabled);

    if (!bValidSystem || CachedPrimaryStarMap.Name.IsEmpty())
    {
        return LayerId;
    }

    const FVector2D PanelSize = AllottedGeometry.GetLocalSize();
    const FVector2D Center(
        PanelSize.X * 0.5f,
        PanelSize.Y * 0.55f);

    const float StarSize = ComputeStarDrawSize(CachedPrimaryStarMap);
    const float RingSize = ComputeRingDrawSize(CachedPrimaryStarMap);

    UTexture2D* StarTexture = GetStarTextureForClass(CachedPrimaryStarMap.Class);

    const FLinearColor StarTint = ComputeStarTint(CachedPrimaryStarMap);
    const FLinearColor RingTint = ComputeSystemIFFRingTint(CachedGalaxyRow);

    int32 PaintLayer = LayerId;

    if (IFFRingTexture)
    {
        FSlateBrush RingBrush;
        RingBrush.DrawAs = ESlateBrushDrawType::Image;
        RingBrush.SetResourceObject(IFFRingTexture);
        RingBrush.ImageSize = FVector2D(RingSize, RingSize);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            ++PaintLayer,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(Center.X - RingSize * 0.5f, Center.Y - RingSize * 0.5f),
                FVector2D(RingSize, RingSize)),
            &RingBrush,
            ESlateDrawEffect::None,
            RingTint);
    }

    if (StarTexture)
    {
        FSlateBrush StarBrush;
        StarBrush.DrawAs = ESlateBrushDrawType::Image;
        StarBrush.SetResourceObject(StarTexture);
        StarBrush.ImageSize = FVector2D(StarSize, StarSize);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            ++PaintLayer,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(Center.X - StarSize * 0.5f, Center.Y - StarSize * 0.5f),
                FVector2D(StarSize, StarSize)),
            &StarBrush,
            ESlateDrawEffect::None,
            StarTint);
    }

    FSlateDrawElement::MakeText(
        OutDrawElements,
        ++PaintLayer,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(Center.X - 100.0f, Center.Y + StarSize * 0.5f + 10.0f),
            FVector2D(200.0f, 20.0f)),
        CachedPrimaryStarMap.Name,
        FCoreStyle::GetDefaultFontStyle("Regular", 11),
        ESlateDrawEffect::None,
        FLinearColor::White);

    return PaintLayer;
}