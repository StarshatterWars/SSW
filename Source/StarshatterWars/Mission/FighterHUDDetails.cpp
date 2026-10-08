#include "FighterHUDDetails.h"
#include "Ship.h"
#include "ShipActor.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimContact.h"
#include "Sensor.h"
#include "SimElement.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "Instruction.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "QuantumDrive.h"
#include "Power.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

namespace
{
    const FLinearColor HUDBlue(0.15f, 0.55f, 1.0f, 1.0f);
    // Ship summary health: green -> yellow -> red as integrity falls.
    FLinearColor ShipHealthColor(double Percent)
    {
        if (!FMath::IsFinite(Percent) || Percent < 0) return FLinearColor::Gray;
        const float Health = float(FMath::Clamp(Percent / 100.0, 0.0, 1.0));
        const FLinearColor Red(1.0f, 0.05f, 0.02f, 1.0f);
        const FLinearColor Yellow(1.0f, 0.85f, 0.0f, 1.0f);
        const FLinearColor Green(0.05f, 1.0f, 0.1f, 1.0f);
        return Health <= 0.5f ? FMath::Lerp(Red, Yellow, Health * 2.0f)
            : FMath::Lerp(Yellow, Green, (Health - 0.5f) * 2.0f);
    }

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
    bWarningFlash = FMath::Fmod(Now, 1.0) >= 0.5;
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
    NavReadouts.Reset(); MissileReadouts.Reset(); DefenseReadout.Reset();
    bAutoAvailable = bGearDown = bShoot = bClosingValid = false;
    FlightSpeed = HeadingDegrees = ClosingSpeed = JumpSeconds = 0;
    ThreatLevel = 0;
    Ship* P = PlayerShip(); bHasPlayer = P != nullptr;
    PlayerHull = Hull(P); Primary = Secondary = TEXT("NONE");
    if (P)
    {
        FlightSpeed = P->GetVelocity().Size();
        HeadingDegrees = FMath::Fmod(FMath::RadiansToDegrees(P->GetCompassHeading()) + 360.0, 360.0);
        bAutoAvailable = P->CanTimeSkip();
        bGearDown = P->IsGearDown();
        JumpSeconds = P->GetQuantumDrive() ? P->GetQuantumDrive()->JumpTime() : 0;
        WeaponGroup* Missile = P->GetSecondaryGroup();
        bShoot = Missile && Missile->Ammo() > 0 && Missile->GetSelected() && Missile->GetSelected()->Locked();
        if (P->GetShield()) DefenseReadout = FString::Printf(TEXT("SHIELD %.0f"), double(P->GetShieldStrength()));
        else if (P->GetDecoy()) DefenseReadout = FString::Printf(TEXT("DECOY %d"), P->GetDecoy()->Ammo());
        for (int32 I = 0; I < 2; ++I)
        {
            const int32 Eta = P->GetMissileEta(I);
            if (Eta > 0) MissileReadouts.Add(FString::Printf(TEXT("T %d:%02d"), Eta/60, Eta%60));
        }
        if (Instruction* Nav = P->GetNextNavPoint())
        {
            NavReadouts.Add(FString::Printf(TEXT("%s %d"), P->IsAutoNavEngaged() ? TEXT("AUTO NAV") : TEXT("NAV"), P->GetNavIndex(Nav)));
            const char* Action = Instruction::ActionName(Nav->GetAction());
            if (Action && *Action) NavReadouts.Add(FString(ANSI_TO_TCHAR(Action)).ToUpper());
            NavReadouts.Add(FString::Printf(TEXT("SPD %d"), Nav->GetSpeed()));
            const double Distance = FMath::Max(0.0, P->RangeToNavPoint(Nav));
            NavReadouts.Add(FString::Printf(TEXT("%.1f KM"), Distance/1000.0));
            if (FlightSpeed > 10)
            {
                const int32 Eta = int32(FMath::Min(Distance/FlightSpeed, 3601.0));
                NavReadouts.Add(Eta > 3600 ? TEXT("ETR XX:XX") : FString::Printf(TEXT("ETR %d:%02d"), Eta/60, Eta%60));
            }
            if (Nav->GetHoldTime() > 0)
            {
                const int32 Hold = int32(Nav->GetHoldTime());
                NavReadouts.Add(FString::Printf(TEXT("HOLD %d:%02d"), Hold/60, Hold%60));
            }
        }
        PlayerName = ANSI_TO_TCHAR(P->GetName());
        PlayerClass = ANSI_TO_TCHAR(P->GetShipClassName());
        PlayerShield = P->GetShield() ? P->GetShieldStrength() : -1;
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
            if (Sys.value())
            {
                const char* Abbreviation = Sys->Abbreviation();
                FString Label = Abbreviation && *Abbreviation
                    ? FString(ANSI_TO_TCHAR(Abbreviation))
                    : FString(ANSI_TO_TCHAR(Sys->GetName()));
                DamageRows.Add({Label.ToUpper().Left(8), Sys->GetAvailability()});
            }
        DamageRows.Sort([](const FRow& A, const FRow& B) { return A.Percent < B.Percent; });
        Sensor* SensorData = P->GetSensor();
        SensorRange = SensorData ? FMath::Max(1.0, SensorData->GetBeamRange()) : 1.0;
        if (SensorData)
        {
            ListIter<SimContact> It = P->GetContactList();
            while (++It)
            {
                if (!It.value() || It->GetShip() == P || !(It->ActLock() || It->PasLock() || It->Visible(P))) continue;
                // Legacy warnings derive from detected threatening contacts.
                if (It->Threat(P) && !P->IsStarship())
                {
                    if (It->GetShot()) ThreatLevel = 2;
                    else if (It->GetShip() && !It->GetShip()->IsStarship()) ThreatLevel = FMath::Max(ThreatLevel, 1);
                }
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
        if (P)
        {
            const FVector Separation = T->GetLocation()-P->GetLocation();
            bClosingValid = !Separation.IsNearlyZero();
            ClosingSpeed = FVector::DotProduct(P->GetVelocity()-T->GetVelocity(), Separation.GetSafeNormal());
        }
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
    const float Scale = FMath::Min(Size.X / 1280.0, Size.Y / 720.0);
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
        if (Valid) FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(80*Scale,80*Scale),FSlateLayoutTransform(FVector2f(P))),&B,ESlateDrawEffect::None,ShipHealthColor(H));
    };
    auto Artwork = [&](int32 Index, FVector2D P, FVector2D D)
    {
        if (PanelTextures[Index].IsValid())
            FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(D),FSlateLayoutTransform(FVector2f(P))),
                &PanelBrushes[Index],ESlateDrawEffect::None,HUDBlue);
    };
    // Coordinates share the 512x256 HUD artwork's center and scale.
    // Measure each string so the readouts align to the panel edges, not their first glyph.
    auto ReticleText = [&](double X, double Y, double Width, const FString& Label, bool bCentered = false)
    {
        int32 FontSize = FMath::Max(8, int32(11*Scale));
        const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold",FontSize);
        FVector2D Extent = Measure->Measure(Label,Font);
        while (Extent.X > Width*Scale && FontSize > 1)
        {
            Font = FCoreStyle::GetDefaultFontStyle("Bold",--FontSize);
            Extent = Measure->Measure(Label,Font);
        }
        const double Offset = (Width*Scale-Extent.X)*(bCentered ? 0.5 : 1.0);
        const FVector2D P(Size.X*0.5+X*Scale+Offset,Size.Y*0.5+Y*Scale);
        FSlateDrawElement::MakeText(E,L+2,G.ToPaintGeometry(FVector2f(Extent),FSlateLayoutTransform(FVector2f(P))),
            Label,Font,ESlateDrawEffect::None,HUDBlue);
    };
    // Centered boxed status annunciators from the legacy fighter HUD.
    auto Popup = [&](double Y, const FString& Label, FLinearColor Color = HUDBlue)
    {
        const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(8, int32(11*Scale)));
        const FVector2D Extent = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label,Font);
        const FVector2D P(Size.X*0.5-Extent.X*0.5-5*Scale, Size.Y*0.5+Y*Scale);
        const FVector2D D(Extent.X+10*Scale,Extent.Y+4*Scale);
        Box(P,D,FLinearColor(0,0.015f,0.03f,0.75f));
        Box(P,FVector2D(D.X,Scale),Color);
        Box(P+FVector2D(0,D.Y-Scale),FVector2D(D.X,Scale),Color);
        Box(P,FVector2D(Scale,D.Y),Color);
        Box(P+FVector2D(D.X-Scale,0),FVector2D(Scale,D.Y),Color);
        Text(P+FVector2D(5,2)*Scale,Label,Color);
    };
    if (bHasPlayer)
    {
        ReticleText(-240,-8,108,FString::Printf(TEXT("%.0f"),FlightSpeed));
        ReticleText(-40,-100,80,FString::Printf(TEXT("%03d"),FMath::RoundToInt(HeadingDegrees)%360),true);
        ReticleText(-240,88,108,TEXT("TAC"));
        if (bShowWeapons)
        {
            ReticleText(-240,44,108,DefenseReadout);
            for (int32 I=0; I<MissileReadouts.Num(); ++I)
                ReticleText(-240,58+I*14,108,MissileReadouts[I]);
        }
        for (int32 I=0; I<NavReadouts.Num(); ++I)
            ReticleText(132,30+I*14,108,NavReadouts[I]);
        if (bHasTarget)
        {
            ReticleText(132,-18,108,FString::Printf(TEXT("RNG %.1f KM"),TargetRange));
            if (bClosingValid) ReticleText(132,-4,108,FString::Printf(TEXT("CLS %+.0f M/S"),ClosingSpeed));
        }
        Popup(-142,TEXT("AUTO"),bAutoAvailable ? HUDBlue : FLinearColor(0.06f,0.16f,0.24f,1));
        if (JumpSeconds>0) Popup(-168,FString::Printf(TEXT("QUANTUM JUMP: %d"),FMath::CeilToInt(JumpSeconds)));
        else if (ThreatLevel==2 && bWarningFlash) Popup(-168,TEXT("MISSILE WARNING"),FLinearColor::Red);
        else if (ThreatLevel==1) Popup(-168,TEXT("LOCK WARNING"),FLinearColor::Yellow);
        if (bShoot && bShowWeapons) Popup(152,TEXT("SHOOT"));
        if (bGearDown) Popup(126,TEXT("GEAR DOWN"));
    }
    // The fighter uses weapon readouts beside the central HUD, not the capital-ship TAC frame.
    for (int32 I=5; bShowCaution && I<7; ++I)
    {
        const FVector2D Native(256,256);
        const FVector2D D = Native * Scale;
        Artwork(I,FVector2D(Size.X*0.5 + (I==5 ? -D.X : 0.0),Size.Y-D.Y),D);
    }
    if (bShowWeapons && !bHasPlayer) Text(FVector2D(Size.X*0.5-250*Scale,Size.Y*0.5+24*Scale),TEXT("WEAPONS: NO PLAYER SHIP"));
    for (int32 Index=0; Index<2; ++Index)
    {
        const EMode M=Modes[Index]; if (M==EMode::Off) continue;
        // Anchor the full MFD bounds to the viewport's lower corners.
        const FVector2D P(Index==0 ? 0.0 : Size.X-220*Scale, Size.Y-215*Scale);
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
                FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(0.1f,0.3f,0.5f,0.8f),true,Scale);
            }
            Box(Center-FVector2D(2,2)*Scale,FVector2D(4,4)*Scale,HUDBlue);
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
                        FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),Stem,ESlateDrawEffect::None,B.Color,true,Scale);
                    }
                }
                Box(Center+Offset-FVector2D(2,2)*Scale,FVector2D(4,4)*Scale,B.Color);
            }
            Text(P+FVector2D(8,192)*Scale,FString::Printf(TEXT("%.0f KM   %d CONTACTS"),SensorRange/1000,Contacts.Num()));
        }
    }
    // Compact inward-facing ship columns leave the center clear for CAUTION.
    // Fit long names to the column instead of overlapping the adjacent panels.
    auto ShipText = [&](FVector2D P, FString Label, FLinearColor Color = HUDBlue)
    {
        const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(8, int32(11*Scale)));
        const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        if (Measure->Measure(Label,Font).X > 176*Scale)
        {
            while (!Label.IsEmpty() && Measure->Measure(Label+TEXT("..."),Font).X > 176*Scale)
                Label.LeftChopInline(1);
            Label += TEXT("...");
        }
        Text(P,Label,Color);
    };
    if(bHasPlayer)
    {
        // Three data rows below the icon; final row ends at the bottom edge.
        const FVector2D P(232*Scale,Size.Y-172*Scale);
        ShipText(P,PlayerName);
        Icon(P+FVector2D(48,24)*Scale,PlayerBrush,PlayerTexture.IsValid(),PlayerHull);
        ShipText(P+FVector2D(0,112)*Scale,PlayerClass);
        ShipText(P+FVector2D(0,132)*Scale,PlayerHull>=0?FString::Printf(TEXT("HULL  %.0f%%"),PlayerHull):TEXT("HULL  --"),ShipHealthColor(PlayerHull));
        ShipText(P+FVector2D(0,152)*Scale,PlayerShield>=0?FString::Printf(TEXT("SHIELD  %.0f%%"),PlayerShield):TEXT("SHIELD  --"),ShipHealthColor(PlayerShield));
        if (bShowWeapons)
        {
        ReticleText(-240,16,108,Primary.ToUpper());
        ReticleText(-240,30,108,Secondary.ToUpper());
        }
        const int32 Rows=FMath::Min(12,DamageRows.Num());
        for(int32 I=0;bShowCaution && I<Rows;++I)
        {
            const FVector2D D=FVector2D(Size.X*0.5-150*Scale,Size.Y-97*Scale)+FVector2D((I%4)*75,(I/4)*28)*Scale;
            // Legacy caution cells show a centered abbreviation; color conveys health.
            const FString& Label = DamageRows[I].Label;
            const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(8, int32(11*Scale)));
            const FVector2D Extent = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, Font);
            Text(D+FVector2D((75*Scale-Extent.X)*0.5,0),Label,DamageColor(DamageRows[I].Percent),11);
        }
    }
    if(bHasTarget)
    {
        // Target has one additional range row. Keep its final row bottom-aligned too.
        const FVector2D P(Size.X-408*Scale,Size.Y-192*Scale);
        ShipText(P,TargetName);
        Icon(P+FVector2D(48,24)*Scale,TargetBrush,TargetTexture.IsValid(),TargetHull);
        ShipText(P+FVector2D(0,112)*Scale,TargetClass);
        ShipText(P+FVector2D(0,132)*Scale,FString::Printf(TEXT("HULL  %.0f%%"),TargetHull),ShipHealthColor(TargetHull));
        ShipText(P+FVector2D(0,152)*Scale,TargetShield>=0?FString::Printf(TEXT("SHIELD  %.0f%%"),TargetShield):TEXT("SHIELD  --"),ShipHealthColor(TargetShield));
        ShipText(P+FVector2D(0,172)*Scale,TargetRange>=0?FString::Printf(TEXT("%.1f KM"),TargetRange):TEXT("RANGE --"));
    }
    if (IsRadioOpen())
    {
        TArray<FString> Labels; TArray<int32> Commands; TArray<bool> Enabled; FString Title;
        BuildRadioRows(Labels,Commands,Enabled,Title);
        const FVector2D P(Size.X-260*Scale,24*Scale);
        Box(P,FVector2D(244,64+Labels.Num()*24)*Scale,FLinearColor(0,0.025f,0.05f,0.9f));
        Text(P+FVector2D(12,10)*Scale,Title);
        for (int32 I=0; I<Labels.Num(); ++I)
            Text(P+FVector2D(12,36+24*I)*Scale,FString::Printf(TEXT("%d. %s"),I+1,*Labels[I]),Enabled[I]?HUDBlue:FLinearColor(0.12f,0.2f,0.25f,1));
        Text(P+FVector2D(12,40+24*Labels.Num())*Scale,TEXT("0 BACK    R CLOSE"));
    }
    return L+2;
}

void SFighterHUDDetails::ToggleRadio()
{
    if (IsRadioOpen()) { CloseRadio(); return; }
    Ship* P = PlayerShip();
    if (P && P->IsDropship()) { RadioPage = 0; RadioRecipient = 0; }
}
void SFighterHUDDetails::BuildRadioRows(TArray<FString>& Labels, TArray<int32>& Commands, TArray<bool>& Enabled, FString& Title) const
{
    Ship* P = PlayerShip();
    Title = TEXT("RADIO");
    if (!P) return;
    const bool CanTransmit = P->GetEMCON() >= 2;
    auto Add = [&](const TCHAR* Label, RadioMessageAction Action, bool Allowed = true)
    {
        Labels.Add(Label); Commands.Add(int32(Action)); Enabled.Add(CanTransmit && Allowed);
    };
    auto Page = [&](const TCHAR* Label, int32 Value, bool Allowed = true)
    {
        Labels.Add(Label); Commands.Add(-Value); Enabled.Add(CanTransmit && Allowed);
    };
    SimElement* Element = P->GetElement();
    const int32 Index = P->GetElementIndex();
    const int32 WingIndex = Index==1 ? 2 : Index==2 ? 1 : Index==3 ? 4 : Index==4 ? 3 : 0;
    Ship* Wing = Element && WingIndex ? Element->GetShip(WingIndex) : nullptr;
    if (RadioPage == 0)
    {
        Page(TEXT("Wingman"),1,Wing && !Wing->IsDead());
        Page(TEXT("Element"),2,Element != nullptr);
        Page(TEXT("Control"),3,P->GetController()!=nullptr);
    }
    else if (RadioPage == 1)
    {
        Title = RadioRecipient==1 ? TEXT("WINGMAN") : TEXT("ELEMENT");
        Page(TEXT("Target"),2,P->GetTarget()!=nullptr); Page(TEXT("Combat"),3);
        Page(TEXT("Formation"),4); Page(TEXT("Mission"),5); Page(TEXT("Sensors"),6);
    }
    else if (RadioPage == 2)
    {
        Title=TEXT("TARGET");
        Add(TEXT("Attack target"),RadioMessageAction::ATTACK,P->GetTarget()!=nullptr);
        Add(TEXT("Bracket target"),RadioMessageAction::BRACKET,P->GetTarget()!=nullptr);
        Add(TEXT("Escort target"),RadioMessageAction::ESCORT,P->GetTarget()!=nullptr);
    }
    else if (RadioPage == 3)
    {
        Title=TEXT("COMBAT"); Add(TEXT("Cover me"),RadioMessageAction::COVER_ME);
        Add(TEXT("Break and attack"),RadioMessageAction::WEP_FREE); Add(TEXT("Form up"),RadioMessageAction::FORM_UP);
    }
    else if (RadioPage == 4)
    {
        Title=TEXT("FORMATION"); Add(TEXT("Diamond"),RadioMessageAction::GO_DIAMOND);
        Add(TEXT("Spread"),RadioMessageAction::GO_SPREAD); Add(TEXT("Box"),RadioMessageAction::GO_BOX); Add(TEXT("Trail"),RadioMessageAction::GO_TRAIL);
    }
    else if (RadioPage == 5)
    {
        Title=TEXT("MISSION"); Add(TEXT("Skip navpoint"),RadioMessageAction::SKIP_NAVPOINT);
        Add(TEXT("Cancel orders"),RadioMessageAction::RESUME_MISSION); Add(TEXT("Return to base"),RadioMessageAction::RTB);
    }
    else if (RadioPage == 6)
    {
        Title=TEXT("SENSORS"); Add(TEXT("EMCON 1"),RadioMessageAction::GO_EMCON1);
        Add(TEXT("EMCON 2"),RadioMessageAction::GO_EMCON2); Add(TEXT("EMCON 3"),RadioMessageAction::GO_EMCON3); Add(TEXT("Launch probe"),RadioMessageAction::LAUNCH_PROBE);
    }
    else if (RadioPage == 7)
    {
        Title=TEXT("CONTROL"); Add(TEXT("Request picture"),RadioMessageAction::REQUEST_PICTURE);
        Add(TEXT("Request backup"),RadioMessageAction::REQUEST_SUPPORT); Add(TEXT("Call inbound"),RadioMessageAction::CALL_INBOUND); Add(TEXT("Call finals"),RadioMessageAction::CALL_FINALS);
    }
}
void SFighterHUDDetails::RadioSelect(int32 Number)
{
    if (!IsRadioOpen()) return;
    Ship* P=PlayerShip();
    if (!P) { CloseRadio(); return; }
    if (Number==0)
    {
        if (RadioPage==0) CloseRadio();
        else if (RadioPage==1 || RadioPage==7) RadioPage=0;
        else RadioPage=1;
        return;
    }
    TArray<FString> Labels; TArray<int32> Commands; TArray<bool> Enabled; FString Title;
    BuildRadioRows(Labels,Commands,Enabled,Title);
    const int32 I=Number-1;
    if (!Commands.IsValidIndex(I) || !Enabled[I]) return;
    if (RadioPage==0) { RadioRecipient=Number; RadioPage=Number==3 ? 7 : 1; return; }
    if (RadioPage==1) { RadioPage=-Commands[I]; return; }
    const RadioMessageAction Action=static_cast<RadioMessageAction>(Commands[I]);
    SimElement* Element=P->GetElement();
    RadioMessage* Message=nullptr;
    if (RadioRecipient==2 && Element) Message=new RadioMessage(Element,P,Action);
    else
    {
        Ship* Destination=nullptr;
        if (RadioRecipient==3) Destination=P->GetController();
        else if (Element)
        {
            const int32 Index=P->GetElementIndex();
            const int32 Wing=Index==1?2:Index==2?1:Index==3?4:Index==4?3:0;
            if (Wing) Destination=Element->GetShip(Wing);
        }
        if (Destination && !Destination->IsDead()) Message=new RadioMessage(Destination,P,Action);
    }
    if (!Message) return;
    if (RadioPage==2) Message->AddTarget(P->GetTarget());
    RadioTraffic::Transmit(Message);
    CloseRadio();
}
