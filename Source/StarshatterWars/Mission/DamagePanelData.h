#pragma once
#include "CoreMinimal.h"
#include "Ship.h"
#include "Power.h"
#include "Drive.h"
#include "QuantumDrive.h"
#include "Shield.h"
#include "Thruster.h"
#include "Computer.h"
#include "NavSystem.h"
#include "FlightDeck.h"
#include "Farcaster.h"
#include "Sensor.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "ShipSystemIcon.h"
#include "FlightPanelStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

// Snapshot adapter for the legacy warning panel and capital-ship extensions.
// Can be reused by a registered panel; retains no simulation pointers.
namespace SSWDamagePanel
{
    struct FCell {
        FString Label;
        int32 Status=-1; // absent equipment leaves its fixed cell empty
        EShipSystemIcon Icon = EShipSystemIcon::Auxiliary;
        int32 WeaponSlot = 0;
    };
    struct FStatus {
        bool Present=false, Maintenance=false;
        SYSTEM_STATUS Worst=SYSTEM_STATUS::NOMINAL;
        void Add(SimSystem* S) {
            if(!S)return;
            Present=true;
            const auto Value=S->GetStatus();
            if(Value==SYSTEM_STATUS::DESTROYED || Value==SYSTEM_STATUS::CRITICAL || Value==SYSTEM_STATUS::DEGRADED) {
                if(int32(Value)<int32(Worst))Worst=Value;
            }
            else if(Value==SYSTEM_STATUS::MAINT || Value==SYSTEM_STATUS::REPAIR || Value==SYSTEM_STATUS::REPLACE) Maintenance=true;
        }
        int32 Value() const {
            return !Present ? -1 : int32(Worst==SYSTEM_STATUS::NOMINAL && Maintenance ? SYSTEM_STATUS::MAINT : Worst);
        }
    };
    template<class T> void AddList(FStatus& Status,List<T>& Items) {
        for(int32 I=0;I<Items.size();++I)Status.Add(Items[I]);
    }
    inline FString WeaponName(WeaponGroup* Group,int32 Index) {
        FString Name=Group->Name()?FString(ANSI_TO_TCHAR(Group->Name())).TrimStartAndEnd().ToUpper():FString();
        if(Name.IsEmpty() || Name.Contains(TEXT("PRI WEP")) || Name.Contains(TEXT("WEP PRI")) ||
           Name.Contains(TEXT("SEC WEP")) || Name.Contains(TEXT("WEP SEC")) || Name==TEXT("PRIMARY") || Name==TEXT("SECONDARY")) {
            if(auto* Design=Group->GetDesign())Name=FString(ANSI_TO_TCHAR(Design->name.data())).ToUpper();
        }
        if(Name.IsEmpty())Name=FString::Printf(TEXT("WEAPON %d"),Index+1);
        return Name;
    }
    inline void Build(Ship* ShipNow,TArray<FCell>& Cells) {
        Cells.Reset(); if(!ShipNow)return;
        const bool bCapital = ShipNow->IsStarship();
        Cells.SetNum(bCapital ? 16 : 12);
        const TCHAR* Labels[]={TEXT("REACTOR"),TEXT("DRIVE"),TEXT("QUANTUM"),TEXT("SHIELD"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("SENSOR"),TEXT("COMPUTER"),TEXT("THRUSTER"),TEXT("FLT DECK")};
        using EIcon = EShipSystemIcon;
        constexpr EIcon Icons[] = {EIcon::Reactor, EIcon::Engine, EIcon::Jump, EIcon::Shield,
            EIcon::Weapon, EIcon::Weapon, EIcon::Weapon, EIcon::Weapon,
            EIcon::Sensor, EIcon::Computer, EIcon::Thruster, EIcon::Deck};
        for(int32 I=0;I<12;++I) {
            Cells[I].Label=Labels[I];
            Cells[I].Icon=Icons[I];
        }
        FStatus States[16];
        AddList(States[0],ShipNow->GetReactors());
        AddList(States[1],ShipNow->GetDrives());
        States[2].Add(ShipNow->GetQuantumDrive());
        States[3].Add(ShipNow->GetShield());
        if (!bCapital) States[3].Add(ShipNow->GetDecoy());
        if(!bCapital && !ShipNow->GetShield() && ShipNow->GetDecoy()) {
            Cells[3].Label=TEXT("DECOY");
            Cells[3].Icon=EIcon::Auxiliary;
        }
        auto& Groups=ShipNow->GetWeapons();
        for(int32 I=0;I<4 && I<Groups.size();++I)if(auto* Group=Groups[I]) {
            Cells[I+4].Label=WeaponName(Group,I);
            Cells[I+4].WeaponSlot=I+1;
            if (const WeaponDesign* Design = Group->GetDesign(); Design && !Design->beam)
                Cells[I+4].Icon=EIcon::Projectile;
            AddList(States[I+4],Group->GetWeapons());
        }
        // Distinguish genuinely separate groups with identical display names.
        for(int32 I=0;I<4 && I<Groups.size();++I) {
            const FString Name=Cells[I+4].Label; if(Name.IsEmpty())continue;
            int32 Matches=0;for(int32 J=0;J<4 && J<Groups.size();++J)
                if(Groups[J] && WeaponName(Groups[J],J)==Name)++Matches;
            if(Matches>1)Cells[I+4].Label+=FString::Printf(TEXT(" %d"),I+1);
        }
        States[8].Add(ShipNow->GetSensor());
        AddList(States[9],ShipNow->GetComputers());
        if (bCapital) {
            Cells[12].Icon=EIcon::Navigation; Cells[12].Label=TEXT("NAVIGATION"); States[12].Add(ShipNow->GetNavSystem());
            Cells[13].Icon=EIcon::Jump; Cells[13].Label=TEXT("FARCASTER"); States[13].Add(ShipNow->GetFarcaster());
            Cells[14].Icon=EIcon::Auxiliary; Cells[14].Label=TEXT("DECOY"); States[14].Add(ShipNow->GetDecoy());
            Cells[15].Icon=EIcon::Projectile; Cells[15].Label=TEXT("PROBE LAUNCHER"); States[15].Add(ShipNow->GetProbeLauncher());
        } else {
            States[8].Add(ShipNow->GetProbeLauncher());
            States[9].Add(ShipNow->GetNavSystem());
            if (ShipNow->GetComputers().size() == 0 && ShipNow->GetNavSystem()) Cells[9].Icon=EIcon::Navigation;
        }
        States[10].Add(ShipNow->GetThruster());
        AddList(States[11],ShipNow->GetFlightDecks());
        for(int32 I=0;I<Cells.Num();++I)Cells[I].Status=States[I].Value();
    }
    inline FLinearColor Color(int32 Status,double Time,const FLinearColor& Nominal) {
        if(Status<0)return FLinearColor::Transparent;
        if(Status!=int32(SYSTEM_STATUS::NOMINAL) && FMath::Fmod(Time,0.5)<0.25)
            return FLinearColor(0.03f,0.03f,0.03f,1);
        switch(SYSTEM_STATUS(Status)) {
        case SYSTEM_STATUS::DESTROYED:return FLinearColor(0.12f,0.12f,0.12f,1);
        case SYSTEM_STATUS::CRITICAL:return FLinearColor::Red;
        case SYSTEM_STATUS::DEGRADED:return FLinearColor::Yellow;
        default:return Nominal;
        }
    }

    inline void PaintCell(const FCell& Cell, FSlateWindowElementList& Elements, int32 Layer,
        const FGeometry& Geometry, FVector2D Center, float Scale, const FLinearColor& Nominal)
    {
        if (Cell.Status < 0) return;
        const FLinearColor Tint = Color(Cell.Status, FPlatformTime::Seconds(), Nominal);
        const double Width = Cell.WeaponSlot ? 32.0 : 22.0;
        const FVector2D Origin = Center - FVector2D(Width * 0.5, 11.0) * Scale;
        SShipSystemIcon::PaintIcon(Elements, Layer, Geometry, Origin, Cell.Icon, Tint, Scale);
        if (Cell.WeaponSlot)
            FSlateDrawElement::MakeText(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(10 * Scale, 16 * Scale),
                FSlateLayoutTransform(FVector2f(Origin + FVector2D(23, 4) * Scale))), FString::FromInt(Cell.WeaponSlot),
                FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(6, FMath::RoundToInt(11 * Scale))), ESlateDrawEffect::None, Tint);
    }

    inline void Paint(const TArray<FCell>& Cells, FSlateWindowElementList& Elements, int32 Layer,
        const FGeometry& Geometry, FVector2D Origin, float Scale, FLinearColor Nominal, TOptional<FVector2D> LocalCursor)
    {
        constexpr double Width = FlightPanelStyle::CenterPanelWidth;
        const double Height = 42 + 34 * ((Cells.Num() + 3) / 4);
        const auto Box = [&](FVector2D Position, FVector2D Size, const FSlateBrush& Brush, FLinearColor Tint, int32 Z)
        {
            FSlateDrawElement::MakeBox(Elements, Layer + Z, Geometry.ToPaintGeometry(FVector2f(Size * Scale),
                FSlateLayoutTransform(FVector2f(Origin + Position * Scale))), &Brush, ESlateDrawEffect::None, Tint);
        };
        const auto Text = [&](FVector2D Position, const FString& Value, int32 FontSize, FLinearColor Tint, int32 Z)
        {
            FSlateDrawElement::MakeText(Elements, Layer + Z, Geometry.ToPaintGeometry(FVector2f(1, 1),
                FSlateLayoutTransform(FVector2f(Origin + Position * Scale))), Value,
                FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(6, FMath::RoundToInt(FontSize * Scale))), ESlateDrawEffect::None, Tint);
        };
        static const FSlateRoundedBoxBrush Tile(FLinearColor::White, 5.f);
        FlightPanelStyle::PaintGlass(Elements, Layer, Geometry, Origin, FVector2D(Width, Height) * Scale);
        FlightPanelStyle::PaintHeader(Elements, Layer + 1, Geometry, Origin, Width, Scale, TEXT("CAUTION"), Nominal);
        const FVector2D Cursor = LocalCursor ? (*LocalCursor - Origin) / Scale : FVector2D(-1, -1);
        const FCell* Hovered = nullptr;
        for (int32 Index = 0; Index < Cells.Num(); ++Index)
        {
            const FCell& Cell = Cells[Index];
            if (Cell.Status < 0) continue;
            constexpr double CellWidth = (Width - 24 - 18) / 4;
            const FVector2D Position(12 + (Index % 4) * (CellWidth + 6), 36 + (Index / 4) * 34);
            const bool bOver = Cursor.X >= Position.X && Cursor.X < Position.X + CellWidth &&
                Cursor.Y >= Position.Y && Cursor.Y < Position.Y + 30;
            if (bOver) Hovered = &Cell;
            Box(Position, {CellWidth, 30}, Tile, bOver ? FLinearColor(.07f,.17f,.23f,1) : FLinearColor(.025f,.055f,.08f,1), 1);
            PaintCell(Cell, Elements, Layer + 2, Geometry, Origin + (Position + FVector2D(CellWidth / 2,15)) * Scale, Scale, Nominal);
        }
        if (!Hovered) return;
        const auto Status = static_cast<SYSTEM_STATUS>(Hovered->Status);
        const UEnum* StatusEnum = StaticEnum<SYSTEM_STATUS>();
        const FString Detail = Hovered->Label + TEXT("  ·  ") + StatusEnum->GetDisplayNameTextByValue(Hovered->Status).ToString();
        const int32 FontSize = FMath::Max(6, FMath::RoundToInt(12 * Scale));
        const double TextWidth = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(
            Detail, FCoreStyle::GetDefaultFontStyle("Bold", FontSize)).X / Scale;
        FlightPanelStyle::PaintGlass(Elements, Layer + 3, Geometry, Origin + FVector2D(0, -32) * Scale,
            FVector2D(FMath::Max(Width, TextWidth + 24), 28) * Scale);
        Text({12,-26}, Detail, 12, Status == SYSTEM_STATUS::CRITICAL ? FLinearColor::Red : FLinearColor::White, 4);
    }
    struct FDiagramPoint
    {
        FVector2D UV;double Health=100;FString Name;EShipSystemIcon Icon=EShipSystemIcon::Auxiliary;
        SYSTEM_STATUS Status=SYSTEM_STATUS::NOMINAL;
        bool HasDamage()const{return FMath::IsFinite(Health) && (Health<99.9 || Status==SYSTEM_STATUS::DESTROYED || Status==SYSTEM_STATUS::CRITICAL || Status==SYSTEM_STATUS::DEGRADED);}
    };
    // Orthographic player schematic is nose-left / engine-aft-right.
    inline FVector2D DefaultLocation(EShipSystemIcon Icon,int32 Index=0,bool Capital=false)
    {
        switch(Icon)
        {
        case EShipSystemIcon::Sensor:return {.18,.5};
        case EShipSystemIcon::Navigation:return {.28,.45};
        case EShipSystemIcon::Computer:return {.35,.5};
        case EShipSystemIcon::Reactor:return {.52,.5};
        case EShipSystemIcon::Engine:return {.84,Index%2?.62:.38};
        case EShipSystemIcon::Jump:return {.69,.5};
        case EShipSystemIcon::Shield:return {.52,.77};
        case EShipSystemIcon::Deck:return {Capital?.65:.5,.67};
        case EShipSystemIcon::Thruster:return {Index%2?.78:.25,Index%4<2?.18:.82};
        case EShipSystemIcon::Weapon:case EShipSystemIcon::Projectile:return {.38+.12*(Index/2),Index%2?.74:.26};
        default:return {.55,Index%2?.65:.35};
        }
    }
    inline int32 DamageCount(TConstArrayView<FDiagramPoint> Points)
    {int32 Count=0;for(const auto& Point:Points)if(Point.HasDamage())++Count;return Count;}
    constexpr double SchematicHeight=224;
    inline void PaintSchematic(TConstArrayView<FDiagramPoint> Points,const FSlateBrush* Hull,double HullHealth,
        FSlateWindowElementList& Elements,int32 Layer,const FGeometry& Geometry,FVector2D Origin,float Scale,
        FLinearColor Theme,TOptional<FVector2D> LocalCursor)
    {
        constexpr double Width=FlightPanelStyle::CenterPanelWidth;
        FlightPanelStyle::PaintGlass(Elements,Layer,Geometry,Origin,FVector2D(Width,SchematicHeight)*Scale);
        FlightPanelStyle::PaintHeader(Elements,Layer+1,Geometry,Origin,Width,Scale,TEXT("CAUTION"),Theme);
        const FVector2D ImageOrigin=Origin+FVector2D(12,36)*Scale,ImageSize=FVector2D(Width-24,(Width-24)*.5)*Scale;
        if(Hull)FSlateDrawElement::MakeBox(Elements,Layer+2,Geometry.ToPaintGeometry(FVector2f(ImageSize),FSlateLayoutTransform(FVector2f(ImageOrigin))),Hull,ESlateDrawEffect::None,FLinearColor(.65f,.85f,.95f,1));
        const FDiagramPoint* Hovered=nullptr;TArray<FVector2D,TInlineAllocator<32>> Occupied;
        for(const auto& Point:Points)
        {
            if(!Point.HasDamage())continue;
            FVector2D Center=ImageOrigin+Point.UV*ImageSize;
            Center.X=FMath::Clamp(Center.X,ImageOrigin.X+12*Scale,ImageOrigin.X+ImageSize.X-12*Scale);
            Center.Y=FMath::Clamp(Center.Y,ImageOrigin.Y+12*Scale,ImageOrigin.Y+ImageSize.Y-12*Scale);
            const FVector2D Mount=Center;
            const auto Fits=[&](FVector2D Position){return !Occupied.ContainsByPredicate([&](FVector2D Other){return (Position-Other).SizeSquared()<FMath::Square(22*Scale);});};
            if(!Fits(Center))
            {
                const FVector2D Directions[]={{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{1,-1},{-1,1},{1,1}};
                bool Placed=false;
                for(int32 Ring=1;Ring<=4 && !Placed;++Ring)for(const auto& Direction:Directions)
                {
                    FVector2D Candidate=Mount+Direction*(24*Ring*Scale);
                    Candidate.X=FMath::Clamp(Candidate.X,ImageOrigin.X+12*Scale,ImageOrigin.X+ImageSize.X-12*Scale);
                    Candidate.Y=FMath::Clamp(Candidate.Y,ImageOrigin.Y+12*Scale,ImageOrigin.Y+ImageSize.Y-12*Scale);
                    if(Fits(Candidate)){Center=Candidate;Placed=true;break;}
                }
            }
            Occupied.Add(Center);
            if(!Center.Equals(Mount))FSlateDrawElement::MakeLines(Elements,Layer+3,Geometry.ToPaintGeometry(),{Mount,Center},ESlateDrawEffect::None,Theme*.5f,true,1);
            const FLinearColor Tint=Point.Status==SYSTEM_STATUS::DESTROYED || Point.Health<=0?FLinearColor(1,.18f,.14f,1):Point.Health<50?FLinearColor(1,.3f,.12f,1):FLinearColor(1,.78f,.15f,1);
            FSlateDrawElement::MakeBox(Elements,Layer+3,Geometry.ToPaintGeometry(FVector2f(24*Scale,24*Scale),FSlateLayoutTransform(FVector2f(Center-FVector2D(12,12)*Scale))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.015f,.025f,.04f,.95f));
            SShipSystemIcon::PaintIcon(Elements,Layer+4,Geometry,Center-FVector2D(10,10)*Scale,Point.Icon,Tint,Scale*.9f);
            if(LocalCursor && FVector2D::Distance(*LocalCursor,Center)<15*Scale)Hovered=&Point;
        }
        FString Detail=Hovered?FString::Printf(TEXT("%s  %.0f%%"),*Hovered->Name.ToUpper().Left(35),Hovered->Health):
            !Hull?TEXT("SCHEMATIC LOADING"):DamageCount(Points)>0?FString::Printf(TEXT("HULL %.0f%%  /  %d DAMAGED SYSTEMS"),HullHealth,DamageCount(Points)):TEXT("NO SYSTEM DAMAGE");
        FSlateDrawElement::MakeText(Elements,Layer+5,Geometry.ToPaintGeometry(FVector2f(Width-24,20),FSlateLayoutTransform(FVector2f(Origin+FVector2D(12,SchematicHeight-25)*Scale))),Detail,
            FCoreStyle::GetDefaultFontStyle("Regular",FMath::Max(8,FMath::RoundToInt(10*Scale))),ESlateDrawEffect::None,Hovered?FLinearColor(1,.8f,.25f,1):Theme);
    }

}
