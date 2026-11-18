#pragma once

#include "HUDView.h"
#include "Rendering/DrawElements.h"
#include "SimSystem.h"
#include "Widgets/SLeafWidget.h"

enum class EShipSystemIcon : uint8 { Engine, Weapon, Projectile, Shield, Navigation, Reactor, Thruster, Sensor, Computer, Deck, Jump, Auxiliary, Repair, PowerOff, PowerOn, Override, Replace, Up, Down, Gun, MediumRangeMissile, Refresh, Abort, Reference, Options, Ship, Fighter, Station, Building, Transport, Exit, Camera, Radio, Marketplace, Cargo, Production, ShipStore, Contracts, Gazetteer, PlanetMoon, Agent, Accept, Audio, Keyboard, Display, SettingsGear, Hangar, Insurance, Loadout, Skills, Mineral, Electronics, Medical, Liquid, Passengers, MissionPackage, Intel, Back, Joystick, Play, Awards, Wallet };

class SShipSystemIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SShipSystemIcon) : _Icon(EShipSystemIcon::Auxiliary) {}
        SLATE_ARGUMENT(EShipSystemIcon, Icon)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args) { Icon = Args._Icon; }
    FVector2D ComputeDesiredSize(float) const override { return FVector2D(22, 22); }

    bool ComputeVolatility() const override { return true; }

    static FLinearColor GetHUDTint()
    {
        static const FLinearColor Palette[] = {
            FLinearColor(0.35f, 0.88f, 0.48f), FLinearColor(1.0f, 0.68f, 0.22f),
            FLinearColor(0.15f, 0.55f, 1.0f), FLinearColor(0.90f, 0.94f, 1.0f)
        };
        return Palette[FMath::Clamp(HUDView::DefaultColorSet(), 0, static_cast<int32>(UE_ARRAY_COUNT(Palette)) - 1)];
    }

    // JSON image keys use native vectors so they stay sharp and follow the HUD palette.
    static EShipSystemIcon ForCatalogImage(FName Image)
    {
        if (Image == TEXT("Mineral")) return EShipSystemIcon::Mineral;
        if (Image == TEXT("Electronics")) return EShipSystemIcon::Electronics;
        if (Image == TEXT("Medical")) return EShipSystemIcon::Medical;
        if (Image == TEXT("Liquid")) return EShipSystemIcon::Liquid;
        if (Image == TEXT("Passengers")) return EShipSystemIcon::Passengers;
        if (Image == TEXT("Production")) return EShipSystemIcon::Production;
        return EShipSystemIcon::Cargo;
    }

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
        case EShipSystemIcon::Awards: Line({{5,2},{10,2},{12,8},{7,10},{5,2}}); Line({{12,2},{17,2},{15,10},{10,8},{12,2}}); Line({{7,9},{15,9},{18,15},{15,21},{7,21},{4,15},{7,9}}); Line({{11,12},{12,14},{15,15},{12,16},{11,19},{10,16},{7,15},{10,14},{11,12}}); break;
        case EShipSystemIcon::Play: Line({{6,3},{19,11},{6,19},{6,3}}); break;
        case EShipSystemIcon::Joystick: Line({{3,16},{19,16},{20,20},{2,20},{3,16}}); Line({{11,16},{11,8},{8,5},{8,2},{13,2},{15,5},{13,8},{11,8}}); break;
        case EShipSystemIcon::Back: Line({{12,3},{4,11},{12,19}}); Line({{4,11},{20,11}}); break;
        case EShipSystemIcon::Intel: Line({{3,3},{19,3},{19,19},{3,19},{3,3}}); Line({{6,6},{10,6},{10,10},{6,10},{6,6}}); Line({{13,6},{16,6}}); Line({{13,9},{16,9}}); Line({{6,13},{16,13}}); Line({{6,16},{16,16}}); break;
        case EShipSystemIcon::MissionPackage: Line({{2,4},{8,4},{8,9},{2,9},{2,4}}); Line({{14,4},{20,4},{20,9},{14,9},{14,4}}); Line({{8,15},{14,15},{14,20},{8,20},{8,15}}); Line({{5,10},{5,12},{17,12},{17,10}}); Line({{11,12},{11,15}}); break;
        case EShipSystemIcon::Mineral: Line({{4,7},{12,2},{20,8},{17,18},{7,21},{2,14},{4,7},{13,10},{12,2}}); Line({{13,10},{17,18}}); Line({{13,10},{7,21}}); break;
        case EShipSystemIcon::Electronics: Line({{5,5},{17,5},{17,17},{5,17},{5,5}}); Line({{8,1},{8,5}}); Line({{14,1},{14,5}}); Line({{8,17},{8,21}}); Line({{14,17},{14,21}}); Line({{1,8},{5,8}}); Line({{17,14},{21,14}}); Line({{9,9},{13,9},{13,13},{9,13},{9,9}}); break;
        case EShipSystemIcon::Medical: Line({{8,3},{14,3},{14,8},{20,8},{20,14},{14,14},{14,20},{8,20},{8,14},{2,14},{2,8},{8,8},{8,3}}); break;
        case EShipSystemIcon::Liquid: Line({{11,2},{18,12},{19,16},{16,20},{11,21},{6,20},{3,16},{4,12},{11,2}}); Line({{7,14},{7,17},{10,18}}); break;
        case EShipSystemIcon::Passengers: Line({{8,3},{14,3},{16,6},{14,10},{8,10},{6,6},{8,3}}); Line({{3,21},{4,15},{8,12},{14,12},{18,15},{19,21}}); break;
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
        case EShipSystemIcon::PlanetMoon:
            Line({{8,4},{12,5},{15,8},{15,13},{12,17},{8,18},{4,17},{1,13},{1,9},{4,5},{8,4}});
            Line({{18,1},{21,2},{22,5},{20,7},{17,7},{16,4},{18,1}});
            break;
        case EShipSystemIcon::Accept: Line({{3,11},{8,16},{19,5}}); break;
        case EShipSystemIcon::Audio:
            Line({{2,8},{7,8},{12,4},{12,18},{7,14},{2,14},{2,8}});
            Line({{15,8},{17,11},{15,14}}); Line({{18,5},{21,11},{18,17}}); break;
        case EShipSystemIcon::Keyboard:
            Line({{2,5},{20,5},{20,17},{2,17},{2,5}});
            Line({{5,9},{7,9}}); Line({{10,9},{12,9}}); Line({{15,9},{17,9}});
            Line({{6,13},{16,13}}); break;
        case EShipSystemIcon::Display:
            Line({{2,3},{20,3},{20,15},{2,15},{2,3}});
            Line({{11,15},{11,19}}); Line({{6,19},{16,19}}); break;
        case EShipSystemIcon::Agent:
            Line({{8,3},{14,3},{16,6},{15,10},{12,12},{10,12},{7,10},{6,6},{8,3}});
            Line({{3,20},{3,17},{7,14},{15,14},{19,17},{19,20},{3,20}});
            break;
        case EShipSystemIcon::Contracts:
            Line({{4,2},{13,2},{18,7},{18,20},{4,20},{4,2}});
            Line({{13,2},{13,7},{18,7}});
            Line({{7,9},{11,9}});
            Line({{7,14},{10,17},{15,12}});
            break;
        case EShipSystemIcon::ShipStore:
            Line({{7,5},{3,5},{3,19},{19,19},{19,5},{15,5}});
            Line({{6,12},{9,9},{14,9},{17,12},{14,15},{9,15},{6,12}});
            Line({{10,12},{13,12}});
            break;
        case EShipSystemIcon::Ship:
            Line({{2,10},{6,6},{14,6},{20,11},{14,16},{6,16},{2,12},{2,10}});
            Line({{7,9},{13,9},{16,11},{13,13},{7,13},{7,9}});
            break;
        case EShipSystemIcon::Fighter:
            Line({{11,2},{14,10},{20,17},{14,16},{11,19},{8,16},{2,17},{8,10},{11,2}});
            Line({{11,8},{11,13}});
            break;
        case EShipSystemIcon::Station:
            Line({{6,5},{16,5},{20,11},{16,17},{6,17},{2,11},{6,5}});
            Line({{8,8},{14,8},{14,14},{8,14},{8,8}});
            Line({{11,2},{11,5}}); Line({{11,17},{11,20}});
            break;
        case EShipSystemIcon::Building:
            Line({{2,19},{20,19}}); Line({{4,19},{4,4},{13,4},{13,19}});
            Line({{13,10},{18,10},{18,19}}); Line({{7,8},{10,8}}); Line({{7,12},{10,12}});
            break;
        case EShipSystemIcon::Transport:
            Line({{2,7},{15,7},{20,11},{15,15},{2,15},{2,7}});
            Line({{6,7},{6,15}}); Line({{11,7},{11,15}});
            break;
        case EShipSystemIcon::Reference:
            Line({{5,3},{18,3},{18,17},{5,17},{3,19},{5,21},{18,21}});
            Line({{5,3},{3,5},{3,19}});
            Line({{8,7},{15,7}}); Line({{8,10},{14,10}});
            break;
        case EShipSystemIcon::Gazetteer:
            Line({{11,5},{7,3},{2,3},{2,17},{7,17},{11,19},{15,17},{20,17},{20,3},{15,3},{11,5},{11,19}});
            Line({{5,7},{8,8}}); Line({{14,8},{17,7}});
            break;
        case EShipSystemIcon::Camera:
            Line({{2,6},{14,6},{14,16},{2,16},{2,6}});
            Line({{14,9},{20,6},{20,16},{14,13}});
            break;
        case EShipSystemIcon::Radio:
            Line({{4,9},{18,9},{18,20},{4,20},{4,9}});
            Line({{7,9},{7,2}}); Line({{8,13},{14,13}}); Line({{8,16},{14,16}});
            break;
        case EShipSystemIcon::Cargo:
            Line({{3,6},{11,2},{19,6},{19,16},{11,20},{3,16},{3,6},{11,10},{19,6}});
            Line({{11,10},{11,20}}); Line({{7,4},{15,8}});
            break;
        case EShipSystemIcon::Production:
            Line({{2,20},{2,11},{7,8},{7,11},{12,8},{12,11},{16,11},{16,3},{19,3},{20,20},{2,20}});
            Line({{6,15},{8,15}}); Line({{11,15},{13,15}}); Line({{16,15},{18,15}});
            break;
        case EShipSystemIcon::Marketplace:
            Line({{4,9},{18,9},{19,20},{3,20},{4,9}});
            Line({{8,11},{8,5},{10,3},{12,3},{14,5},{14,11}});
            break;
        case EShipSystemIcon::Wallet:
            Line({{3,5},{19,5},{19,9}});Line({{19,15},{19,20},{3,20},{3,5}});
            Line({{5,5},{5,2},{17,2},{17,5}});
            Line({{14,9},{21,9},{21,15},{14,15},{14,9}});
            Line({{17,12},{18,12}});
            break;
        case EShipSystemIcon::Exit:
            Line({{10,3},{3,3},{3,19},{10,19}});
            Line({{8,11},{20,11}}); Line({{15,6},{20,11},{15,16}});
            break;
        case EShipSystemIcon::Loadout:
            Line({{3,5},{19,5}}); Line({{6,5},{6,16},{9,19},{12,16},{12,5}});
            Line({{15,5},{15,16},{18,19},{21,16},{21,5}});
            break;
        case EShipSystemIcon::Skills:
            Line({{2,4},{9,4},{11,6},{13,4},{20,4},{20,18},{13,18},{11,20},{9,18},{2,18},{2,4}});
            Line({{11,6},{11,20}}); Line({{5,8},{8,8}}); Line({{5,12},{8,12}});
            Line({{14,9},{17,9}}); Line({{14,13},{17,13}}); break;
        case EShipSystemIcon::Hangar:
            Line({{2,20},{2,7},{11,2},{20,7},{20,20}});
            Line({{6,20},{6,10},{16,10},{16,20}});
            Line({{8,16},{14,16},{11,12},{8,16}});
            break;
        case EShipSystemIcon::Insurance:
            Line({{3,4},{11,2},{19,4},{18,12},{15,17},{11,20},{7,17},{4,12},{3,4}});
            Line({{7,10},{10,13},{15,7}});
            break;
        case EShipSystemIcon::SettingsGear:
            Line({{8,5},{9,2},{13,2},{14,5},{17,6},{20,5},{22,9},{19,11},{19,14},{21,16},
                  {19,20},{16,19},{13,20},{12,22},{8,21},{8,18},{5,16},{2,17},{0,13},
                  {3,11},{3,8},{2,6},{5,3},{8,5}});
            Line({{11,7},{14,8},{16,11},{14,15},{11,16},{8,15},{6,11},{8,8},{11,7}});
            break;
        case EShipSystemIcon::Options: Line({{3,5},{19,5}}); Line({{7,2},{7,8}}); Line({{3,11},{19,11}}); Line({{15,8},{15,14}}); Line({{3,17},{19,17}}); Line({{9,14},{9,20}}); break;
        default: Line({{11,2},{20,11},{11,20},{2,11},{11,2}}); break;
        }
    }

    int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
    {
        PaintIcon(Elements, Layer, Geometry, (Geometry.GetLocalSize() - FVector2D(22, 22)) * 0.5,
            Icon, GetHUDTint());
        return Layer;
    }

private:
    EShipSystemIcon Icon = EShipSystemIcon::Auxiliary;
};
