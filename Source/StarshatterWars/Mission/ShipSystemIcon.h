#pragma once

#include "Rendering/DrawElements.h"
#include "SimSystem.h"
#include "Widgets/SLeafWidget.h"

enum class EShipSystemIcon : uint8 { Engine, Weapon, Projectile, Shield, Navigation, Reactor, Thruster, Sensor, Computer, Deck, Jump, Auxiliary, Repair, PowerOff, PowerOn, Override, Replace, Up, Down, Gun, MediumRangeMissile, Refresh, Abort, Reference, Options };

class SShipSystemIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SShipSystemIcon) : _Icon(EShipSystemIcon::Auxiliary) {}
        SLATE_ARGUMENT(EShipSystemIcon, Icon)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args) { Icon = Args._Icon; }
    FVector2D ComputeDesiredSize(float) const override { return FVector2D(22, 22); }

    static EShipSystemIcon ForSystem(const SimSystem& System)
    {
        const FString Name = UTF8_TO_TCHAR(System.GetName());
        switch (System.GetType())
        {
        case SYSTEM_CATEGORY::DRIVE: return Name.Contains(TEXT("Thruster")) ? EShipSystemIcon::Thruster : EShipSystemIcon::Engine;
        case SYSTEM_CATEGORY::WEAPON: return EShipSystemIcon::Weapon;
        case SYSTEM_CATEGORY::SHIELD: return EShipSystemIcon::Shield;
        case SYSTEM_CATEGORY::POWER_SOURCE: return EShipSystemIcon::Reactor;
        case SYSTEM_CATEGORY::SENSOR: return EShipSystemIcon::Sensor;
        case SYSTEM_CATEGORY::COMPUTER: return EShipSystemIcon::Computer;
        case SYSTEM_CATEGORY::FLIGHT_DECK: return EShipSystemIcon::Deck;
        case SYSTEM_CATEGORY::FARCASTER: return EShipSystemIcon::Jump;
        default: return Name.Contains(TEXT("Nav")) ? EShipSystemIcon::Navigation : EShipSystemIcon::Auxiliary;
        }
    }

    static void PaintIcon(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        FVector2D Offset, EShipSystemIcon Symbol, FLinearColor Tint, float Scale = 1.0f)
    {
        const auto Line = [&](std::initializer_list<FVector2f> Points)
        {
            TArray<FVector2f> Path;
            for (const FVector2f& Point : Points) Path.Add(Point * Scale + FVector2f(Offset));
            FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Path,
                ESlateDrawEffect::None, Tint, true, FMath::Max(1.0f, 1.4f * Scale));
        };
        switch (Symbol)
        {
        case EShipSystemIcon::Engine: Line({{5,3},{17,3},{15,12},{7,12},{5,3}}); Line({{7,15},{7,20}}); Line({{11,15},{11,21}}); Line({{15,15},{15,20}}); break;
        case EShipSystemIcon::Weapon: Line({{3,11},{19,11}}); Line({{11,3},{11,19}}); Line({{5,5},{17,5},{17,17},{5,17},{5,5}}); break;
        case EShipSystemIcon::Projectile: Line({{11,2},{15,7},{15,17},{7,17},{7,7},{11,2}}); Line({{7,12},{4,18},{7,17}}); Line({{15,12},{18,18},{15,17}}); Line({{9,20},{13,20}}); break;
        case EShipSystemIcon::Shield: Line({{3,4},{11,2},{19,4},{18,12},{15,17},{11,20},{7,17},{4,12},{3,4}}); break;
        case EShipSystemIcon::Navigation: Line({{11,2},{19,19},{11,15},{3,19},{11,2}}); Line({{11,9},{11,15}}); break;
        case EShipSystemIcon::Reactor: Line({{6,2},{16,2},{21,11},{16,20},{6,20},{1,11},{6,2}}); Line({{12,5},{8,12},{14,12},{10,17}}); break;
        case EShipSystemIcon::Thruster: Line({{2,11},{8,5},{8,17},{2,11}}); Line({{20,11},{14,5},{14,17},{20,11}}); break;
        case EShipSystemIcon::Sensor: Line({{3,8},{6,14},{12,17},{18,16}}); Line({{10,17},{8,21},{15,21}}); Line({{9,12},{16,4},{19,4},{19,8}}); break;
        case EShipSystemIcon::Computer: Line({{5,5},{17,5},{17,17},{5,17},{5,5}}); Line({{8,2},{8,5}}); Line({{14,2},{14,5}}); Line({{8,17},{8,20}}); Line({{14,17},{14,20}}); break;
        case EShipSystemIcon::Deck: Line({{3,20},{3,8},{11,2},{19,8},{19,20}}); Line({{7,20},{7,11},{15,11},{15,20}}); break;
        case EShipSystemIcon::Jump: Line({{2,5},{8,11},{2,17}}); Line({{8,5},{14,11},{8,17}}); Line({{14,5},{20,11},{14,17}}); break;
        case EShipSystemIcon::Repair:
            Line({{12,3},{15,2},{13,6},{16,9},{20,6},{19,11},{16,13},{12,12},
                  {6,19},{4,20},{2,18},{3,16},{10,10},{9,7},{10,5},{12,3}});
            break;
        case EShipSystemIcon::PowerOff: Line({{7,5},{4,8},{3,13},{6,18},{11,20},{16,18},{19,13},{18,8},{15,5}}); Line({{11,2},{11,12}}); break;
        case EShipSystemIcon::PowerOn: Line({{11,3},{11,19}}); break;
        case EShipSystemIcon::Override: Line({{12,2},{5,12},{11,12},{9,20},{17,9},{11,9},{12,2}}); break;
        case EShipSystemIcon::Replace: Line({{3,7},{18,7},{14,3}}); Line({{19,15},{4,15},{8,19}}); break;
        case EShipSystemIcon::Up: Line({{5,14},{11,8},{17,14}}); break;
        case EShipSystemIcon::Down: Line({{5,8},{11,14},{17,8}}); break;
        case EShipSystemIcon::Gun: Line({{3,16},{10,16},{19,7},{16,4},{7,13},{3,13},{3,16}}); Line({{12,7},{16,11}}); break;
        case EShipSystemIcon::MediumRangeMissile: Line({{11,1},{15,6},{15,16},{7,16},{7,6},{11,1}}); Line({{7,11},{3,18},{7,16}}); Line({{15,11},{19,18},{15,16}}); Line({{8,20},{14,20}}); break;
        case EShipSystemIcon::Refresh: Line({{18,8},{15,4},{9,3},{4,7},{3,12},{6,17},{11,19},{16,17},{19,13}}); Line({{18,3},{18,8},{13,8}}); break;
        case EShipSystemIcon::Abort: Line({{5,5},{17,5},{17,17},{5,17},{5,5}}); Line({{8,8},{14,14}}); Line({{14,8},{8,14}}); break;
        case EShipSystemIcon::Reference: Line({{11,5},{7,3},{2,3},{2,17},{7,17},{11,19},{15,17},{20,17},{20,3},{15,3},{11,5},{11,19}}); break;
        case EShipSystemIcon::Options: Line({{3,5},{19,5}}); Line({{7,2},{7,8}}); Line({{3,11},{19,11}}); Line({{15,8},{15,14}}); Line({{3,17},{19,17}}); Line({{9,14},{9,20}}); break;
        default: Line({{11,2},{20,11},{11,20},{2,11},{11,2}}); break;
        }
    }

    int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
    {
        PaintIcon(Elements, Layer, Geometry, (Geometry.GetLocalSize() - FVector2D(22, 22)) * 0.5,
            Icon, FLinearColor(0.28f, 0.8f, 1.0f));
        return Layer;
    }

private:
    EShipSystemIcon Icon = EShipSystemIcon::Auxiliary;
};
