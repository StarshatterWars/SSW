#include "FighterHUDDetails.h"
#include "Ship.h"
#include "ShipActor.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimContact.h"
#include "Sensor.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "QuantumDrive.h"
#include "Power.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
    const FLinearColor HUDBlue(0.15f, 0.55f, 1.0f, 1.0f);
    FLinearColor DamageColor(double P)
    {
        return P < 0 ? FLinearColor::Gray : P < 30 ? FLinearColor::Red :
            P < 60 ? FLinearColor::Yellow : HUDBlue;
    }
    Ship* PlayerShip()
    {
        Sim* S = Sim::GetSim();
        Ship* P = S ? S->GetPlayerShip() : nullptr;
        SimRegion* R = S ? S->GetActiveRegion() : nullptr;
        return P && R && R->GetShips().contains(P) && !P->IsDead() ? P : nullptr;
    }
    double Hull(Ship* P)
    {
        return P && P->Design() && P->Design()->integrity > 0
            ? FMath::Clamp(P->GetIntegrity() / P->Design()->integrity * 100.0, 0.0, 100.0) : -1;
    }
    double Charge(SimSystem* S)
    {
        return S && S->GetCapacity() > 0 ? FMath::Clamp(S->GetEnergy() / S->GetCapacity() * 100.0, 0.0, 100.0) : -1;
    }
    FString IconPath(Ship* P)
    {
        const FShipDesign* D = P && P->Design() ? ShipDesignRegistry::Find(P->Design()->name) : nullptr;
        if (!D || D->HudIconName.IsEmpty()) return TEXT("/Game/UI/HUD/hud_icon.hud_icon");
        FString Icon = D->HudIconName;
        if (Icon.StartsWith(TEXT("/Game/"))) return Icon;
        Icon.RemoveFromEnd(TEXT(".pcx"), ESearchCase::IgnoreCase);
        return FString::Printf(TEXT("/Game/UI/Ships/%s/%s.%s"), *D->ShipName, *Icon, *Icon);
    }
    FString WeaponLabel(WeaponGroup* G)
    {
        if (!G) return TEXT("NONE");
        return FString::Printf(TEXT("%hs  %s"), G->Name(),
            G->Ammo() < 0 ? TEXT("--") : *FString::FromInt(G->Ammo()));
    }
}
void SFighterHUDDetails::Construct(const FArguments& A)
{
    ShowHUD = A._ShowHUD; SelectedTarget = A._SelectedTarget; SelectedName = A._SelectedName;
    SetVisibility(EVisibility::HitTestInvisible);
    ForceVolatile(true);
    PlayerBrush.DrawAs = TargetBrush.DrawAs = ESlateBrushDrawType::Image;
    PlayerBrush.ImageSize = TargetBrush.ImageSize = FVector2D(96, 96);
    const TCHAR* Names[] = { TEXT("TAC_left"), TEXT("TAC_right"), TEXT("sensor_fov"), TEXT("sensor_hsd"), TEXT("sensor_3d"), TEXT("CAUTION_left"), TEXT("CAUTION_right") };
    for (int32 I = 0; I < UE_ARRAY_COUNT(Names); ++I)
    {
        const FString Path = FString::Printf(TEXT("/Game/UI/HUD/%s.%s"), Names[I], Names[I]);
        PanelTextures[I].Reset(LoadObject<UTexture2D>(nullptr, *Path));
        PanelBrushes[I].SetResourceObject(PanelTextures[I].Get());
        PanelBrushes[I].DrawAs = ESlateBrushDrawType::Image;
        PanelBrushes[I].TintColor = FSlateColor(FLinearColor::White);
        if (PanelTextures[I].IsValid())
            PanelBrushes[I].ImageSize = FVector2D(PanelTextures[I]->GetSizeX(), PanelTextures[I]->GetSizeY());
        UE_LOG(LogTemp, Log, TEXT("[FighterHUD] Panel %s: %s"), *Path,
            PanelTextures[I].IsValid() ? TEXT("loaded") : TEXT("MISSING"));
    }
}
void SFighterHUDDetails::Tick(const FGeometry& G, double Now, float Delta)
{
    SLeafWidget::Tick(G, Now, Delta);
    if (Now < NextSample) return;
    NextSample = Now + 0.1;
    Refresh();
}
void SFighterHUDDetails::LoadShipHUDIcon(const FString& Path, FString& Cached, TStrongObjectPtr<UTexture2D>& Texture, FSlateBrush& Brush)
{
    if (Path == Cached) return;
    Cached = Path;
    Texture.Reset(LoadObject<UTexture2D>(nullptr, *Path));
    if (!Texture.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing ship silhouette %s; using generic icon."), *Path);
        Texture.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/HUD/hud_icon.hud_icon")));
    }
    Brush.SetResourceObject(Texture.Get());
}
void SFighterHUDDetails::Refresh()
{
    StatusRows.Reset(); DamageRows.Reset(); Contacts.Reset();
    Ship* P = PlayerShip(); bHasPlayer = P != nullptr;
    PlayerHull = Hull(P); Primary = Secondary = TEXT("NONE");
    if (P)
    {
        PlayerName = ANSI_TO_TCHAR(P->GetName());
        LoadShipHUDIcon(IconPath(P), PlayerIconPath, PlayerTexture, PlayerBrush);
        if (P->GetMainDrive()) StatusRows.Add({TEXT("THRUST"), P->GetThrottle()});
        if (P->GetReactors().size() > 0 && P->GetReactors()[0])
            StatusRows.Add({TEXT("FUEL"), double(P->GetReactors()[0]->GetCharge())});
        if (P->GetQuantumDrive()) StatusRows.Add({TEXT("QUANTUM"), Charge(P->GetQuantumDrive())});
        StatusRows.Add({TEXT("HULL"), PlayerHull});
        if (P->GetShield()) StatusRows.Add({TEXT("SHIELD"), double(P->GetShieldStrength())});
        if (P->GetPrimary()) StatusRows.Add({TEXT("GUNS"), Charge(P->GetPrimary())});
        Primary = WeaponLabel(P->GetPrimaryGroup()); Secondary = WeaponLabel(P->GetSecondaryGroup());
        // Most damaged systems first for the compact damage annunciator.
        ListIter<SimSystem> Sys = P->GetSystems();
        while (++Sys)
            if (Sys.value()) DamageRows.Add({ANSI_TO_TCHAR(Sys->GetName()), Sys->GetAvailability()});
        DamageRows.Sort([](const FRow& A, const FRow& B) { return A.Percent < B.Percent; });
        Sensor* SensorData = P->GetSensor();
        SensorRange = SensorData ? FMath::Max(1.0, SensorData->GetBeamRange()) : 1.0;
        if (SensorData)
        {
            ListIter<SimContact> It = P->GetContactList();
            while (++It)
            {
                if (!It.value() || It->GetShip() == P || !(It->ActLock() || It->PasLock() || It->Visible(P))) continue;
                double Az = 0, El = 0, Range = 0;
                It->GetBearing(P, Az, El, Range);
                if (!FMath::IsFinite(Range) || !FMath::IsFinite(Az) || !FMath::IsFinite(El) || Range <= 0 || Range > SensorRange) continue;
                Range = It->Range(P, SensorRange);
                const int IFF = It->GetIFF(P);
                const FLinearColor C = IFF <= 0 ? FLinearColor::Yellow : IFF == P->GetIFF() ? HUDBlue : FLinearColor::Red;
                Contacts.Add({Az, El, Range, C});
            }
        }
    }
    Sim* S = Sim::GetSim(); SimRegion* R = S ? S->GetActiveRegion() : nullptr;
    AShipActor* A = Cast<AShipActor>(SelectedTarget.Get().Get());
    Ship* T = IsValid(A) ? A->GetRuntimeShip() : nullptr;
    if (!T && P && R)
    {
        // Only compare the target pointer until membership is established.
        ListIter<Ship> It = R->GetShips();
        while (++It) if (static_cast<SimObject*>(It.value()) == P->GetTarget()) { T = It.value(); break; }
    }
    bHasTarget = T && R && R->GetShips().contains(T);
    if (bHasTarget && T->IsDead()) bHasTarget = false;
    if (bHasTarget)
    {
        TargetName = IsValid(A) ? SelectedName.Get().ToString() : FString(ANSI_TO_TCHAR(T->GetName()));
        TargetClass = ANSI_TO_TCHAR(T->GetShipClassName());
        TargetHull = Hull(T); TargetShield = T->GetShield() ? T->GetShieldStrength() : -1;
        TargetRange = P ? FVector::Distance(P->GetLocation(), T->GetLocation()) / 1000.0 : -1;
        LoadShipHUDIcon(IconPath(T), TargetIconPath, TargetTexture, TargetBrush);
    }
}
void SFighterHUDDetails::CycleMFD(int32 Index)
{
    if (Index < 0 || Index > 1) return;
    int32 Next = (int32(Modes[Index]) + 1) % 5;
    if (Index == 1 && Next == int32(EMode::Ship)) Next = int32(EMode::FOV);
    Modes[Index] = EMode(Next);
}
void SFighterHUDDetails::CycleWeapon(bool bPrimary)
{
    if (Ship* P = PlayerShip()) { if (bPrimary) P->CyclePrimary(); else P->CycleSecondary(); Refresh(); }
}
int32 SFighterHUDDetails::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&,
    FSlateWindowElementList& E, int32 L, const FWidgetStyle&, bool) const
{
    if (!ShowHUD.Get()) return L;
    const FVector2D Size = G.GetLocalSize();
    if (Size.X <= 0 || Size.Y <= 0) return L;
    if (!bLoggedPanelPaint)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[FighterHUDPaint] Size=%.0fx%.0f Weapons=%d Caution=%d TAC=%dx%d/%dx%d CAUTION=%dx%d/%dx%d"),
            Size.X, Size.Y, bShowWeapons, bShowCaution,
            PanelTextures[0].IsValid() ? PanelTextures[0]->GetSizeX() : 0,
            PanelTextures[0].IsValid() ? PanelTextures[0]->GetSizeY() : 0,
            PanelTextures[1].IsValid() ? PanelTextures[1]->GetSizeX() : 0,
            PanelTextures[1].IsValid() ? PanelTextures[1]->GetSizeY() : 0,
            PanelTextures[5].IsValid() ? PanelTextures[5]->GetSizeX() : 0,
            PanelTextures[5].IsValid() ? PanelTextures[5]->GetSizeY() : 0,
            PanelTextures[6].IsValid() ? PanelTextures[6]->GetSizeX() : 0,
            PanelTextures[6].IsValid() ? PanelTextures[6]->GetSizeY() : 0);
        bLoggedPanelPaint = true;
    }
    const float Scale = FMath::Min(1.0, FMath::Min(Size.X / 1280.0, Size.Y / 720.0));
    auto Text = [&](FVector2D P, const FString& S, FLinearColor C = HUDBlue, int32 FontSize = 11)
    {
        FSlateDrawElement::MakeText(E, L+2, G.ToPaintGeometry(FVector2f(310*Scale,20*Scale),
            FSlateLayoutTransform(FVector2f(P))), S, FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(8, int32(FontSize*Scale))), ESlateDrawEffect::None, C);
    };
    auto Box = [&](FVector2D P, FVector2D D, FLinearColor C)
    {
        FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(D),FSlateLayoutTransform(FVector2f(P))),
            FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,C);
    };
    auto Gauge = [&](FVector2D P, const FString& Label, double Percent)
    {
        const FLinearColor C = DamageColor(Percent);
        Text(P, Label.Left(12), C);
        Box(P+FVector2D(88,5)*Scale,FVector2D(78,6)*Scale,FLinearColor(0.05f,0.1f,0.18f,0.8f));
        if (Percent >= 0) Box(P+FVector2D(88,5)*Scale,FVector2D(78*FMath::Clamp(Percent/100.0,0.0,1.0),6)*Scale,C);
        Text(P+FVector2D(171,0)*Scale,Percent < 0 ? TEXT("--") : FString::Printf(TEXT("%.0f%%"),Percent),C);
    };
    auto Icon = [&](FVector2D P, const FSlateBrush& B, bool Valid, double H)
    {
        if (Valid) FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(80*Scale,80*Scale),FSlateLayoutTransform(FVector2f(P))),&B,ESlateDrawEffect::None,DamageColor(H));
    };
    auto Artwork = [&](int32 Index, FVector2D P, FVector2D D)
    {
        if (PanelTextures[Index].IsValid())
            FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(D),FSlateLayoutTransform(FVector2f(P))),
                &PanelBrushes[Index],ESlateDrawEffect::None,HUDBlue);
    };
    // Legacy WepView anchors the two TAC halves at the top center.
    for (int32 I=0; bShowWeapons && I<2; ++I)
    {
        const FVector2D Native(256,256);
        const FVector2D D = Native * Scale;
        Artwork(I,FVector2D(Size.X*0.5 + (I==0 ? -D.X : 0.0),0),D);
    }
    for (int32 I=5; bShowCaution && I<7; ++I)
    {
        const FVector2D Native(256,256);
        const FVector2D D = Native * Scale;
        Artwork(I,FVector2D(Size.X*0.5 + (I==5 ? -D.X : 0.0),Size.Y-D.Y),D);
    }
    if (bShowWeapons && !bHasPlayer) Text(FVector2D(Size.X*0.5-100*Scale,30*Scale),TEXT("WEAPONS: NO PLAYER SHIP"));
    for (int32 Index=0; Index<2; ++Index)
    {
        const EMode M=Modes[Index]; if (M==EMode::Off) continue;
        const FVector2D P(Index==0 ? 20*Scale : Size.X-240*Scale, Size.Y-375*Scale);
        Box(P,FVector2D(220,215)*Scale,FLinearColor(0,0.025f,0.05f,0.65f));
        const TCHAR* Title=M==EMode::Ship?TEXT("SHIP STATUS  ["):M==EMode::FOV?TEXT("SENSOR FOV"):M==EMode::HSD?TEXT("SENSOR HSD"):TEXT("SENSOR 3D");
        Text(P+FVector2D(8,8)*Scale,Title);
        if (M != EMode::Ship)
        {
            const int32 ArtIndex = M == EMode::FOV ? 2 : M == EMode::HSD ? 3 : 4;
            Artwork(ArtIndex,P+FVector2D(30,32)*Scale,FVector2D(160,160)*Scale);
        }
        if (!bHasPlayer) { Text(P+FVector2D(8,36)*Scale,TEXT("NO PLAYER SHIP")); continue; }
        if (M==EMode::Ship)
        {
            for (int32 I=0;I<StatusRows.Num();++I) Gauge(P+FVector2D(8,36+I*25)*Scale,StatusRows[I].Label,StatusRows[I].Percent);
        }
        else
        {
            const FVector2D Center=P+FVector2D(110,112)*Scale;
            const double Radius=72*Scale;
            const int32 ArtIndex = M == EMode::FOV ? 2 : M == EMode::HSD ? 3 : 4;
            for (int32 Ring=1; !PanelTextures[ArtIndex].IsValid() && Ring<=3; ++Ring)
            {
                TArray<FVector2D> Points;
                for(int32 J=0;J<=48;++J) { double A=2*PI*J/48; Points.Add(Center+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius*Ring/3); }
                FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(0.1f,0.3f,0.5f,0.8f),true,1);
            }
            Box(Center-FVector2D(2,2),FVector2D(4,4),HUDBlue);
            for(const FBlip& B:Contacts)
            {
                FVector2D Offset;
                if(M==EMode::FOV)
                {
                    if(FMath::Abs(B.Az)>PI/3 || FMath::Abs(B.El)>PI/3) continue;
                    Offset=FVector2D(B.Az/(PI/3),-B.El/(PI/3))*Radius;
                }
                else
                {
                    const double R=FMath::Clamp(B.Range/SensorRange,0.0,1.0)*Radius;
                    Offset=FVector2D(FMath::Sin(B.Az),-FMath::Cos(B.Az))*R;
                    if(M==EMode::ThreeD)
                    {
                        const FVector2D Base=Center+FVector2D(Offset.X,Offset.Y*0.55);
                        Offset.Y=Offset.Y*0.55-FMath::Sin(B.El)*R*0.45;
                        TArray<FVector2D> Stem={Base,Center+Offset};
                        FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),Stem,ESlateDrawEffect::None,B.Color,true,1);
                    }
                }
                Box(Center+Offset-FVector2D(2,2),FVector2D(4,4),B.Color);
            }
            Text(P+FVector2D(8,192)*Scale,FString::Printf(TEXT("%.0f KM   %d CONTACTS"),SensorRange/1000,Contacts.Num()));
        }
    }
    if(bHasPlayer)
    {
        const FVector2D P(20*Scale,Size.Y-145*Scale);
        Text(P,PlayerName); Icon(P+FVector2D(0,25)*Scale,PlayerBrush,PlayerTexture.IsValid(),PlayerHull);
        Gauge(P+FVector2D(88,35)*Scale,TEXT("HULL"),PlayerHull);
        if (bShowWeapons)
        {
        const FVector2D W(Size.X*0.5-180*Scale,20*Scale);
        Text(W,TEXT("WEAPONS")); Text(W+FVector2D(0,22)*Scale,TEXT("PRIMARY: ")+Primary);
        Text(W+FVector2D(0,42)*Scale,TEXT("SECONDARY: ")+Secondary);
        Text(W+FVector2D(0,64)*Scale,TEXT("SHIFT+BACKSPACE / BACKSPACE"),HUDBlue,9);
        }
        const int32 Rows=FMath::Min(12,DamageRows.Num());
        for(int32 I=0;bShowCaution && I<Rows;++I)
        {
            const FVector2D D=FVector2D(Size.X*0.5-150*Scale,Size.Y-97*Scale)+FVector2D((I%4)*75,(I/4)*28)*Scale;
            Text(D,FString::Printf(TEXT("%s %.0f%%"),*DamageRows[I].Label.Left(10),DamageRows[I].Percent),DamageColor(DamageRows[I].Percent),8);
        }
    }
    if(bHasTarget)
    {
        const FVector2D P(Size.X-330*Scale,Size.Y-145*Scale);
        Text(P,TargetName); Icon(P+FVector2D(0,25)*Scale,TargetBrush,TargetTexture.IsValid(),TargetHull);
        Text(P+FVector2D(88,22)*Scale,TargetClass);
        Gauge(P+FVector2D(88,44)*Scale,TEXT("HULL"),TargetHull);
        Gauge(P+FVector2D(88,66)*Scale,TEXT("SHIELD"),TargetShield);
        Text(P+FVector2D(88,90)*Scale,TargetRange>=0?FString::Printf(TEXT("%.1f KM"),TargetRange):TEXT("RANGE --"));
    }
    return L+2;
}
