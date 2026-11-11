
#include "FighterHUDDetails.h"
#include "SSWSessionLobbyGameMode.h"
#include "ShipVisualCatalog.h"
#include "GearIndicator.h"
#include "NativeHUDSymbology.h"
#include "FlightPanelStyle.h"
#include "CockpitDisplays.h"
#include "FighterActor.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "FormattingUtils.h"
#include "MissionObjectTarget.h"
#include "MissionUIStyle.h"
#include "Ship.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "Drive.h"
#include "ShipActor.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "Sim.h"
#include "Mission.h"
#include "SimRegion.h"
#include "SimContact.h"
#include "Sensor.h"
#include "SimElement.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "RadioView.h"
#include "Instruction.h"
#include "Game.h"
#include "SSWRuntimeSubsystem.h"
#include "StarshatterInputBindings.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponGroup.h"
#include "QuantumDrive.h"
#include "Power.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

FString SFighterHUDDetails::FormatWeaponReadout(const WeaponGroup* Group)
{
    if (!Group) return TEXT("NONE");
    const Weapon* Selected=Group->GetSelected();
    if (Group->IsPrimary() && Selected && Selected->Design())
    {
        FString Name=ANSI_TO_TCHAR(Selected->Design()->name);
        Name.RemoveFromEnd(TEXT(" Cannon"),ESearchCase::IgnoreCase);
        return FString(TEXT("GUNS: "))+Name.ToUpper();
    }
    if (Group->IsMissile() && Group->Ammo() >= 0)
        return FString::Printf(TEXT("%hs %d RDS"), Group->Name(), Group->Ammo());

    return FString::Printf(TEXT("%hs  %s"),Group->Name(),
        Group->Ammo()<0 ? TEXT("--") : *FString::FromInt(Group->Ammo()));
}

FString SFighterHUDDetails::FormatStatusReadout(const FString& Label,double Percent,bool bAugmenter)
{
    if (!FMath::IsFinite(Percent) || Percent<0.0) return TEXT("--");
    if (Label.Equals(TEXT("HULL"),ESearchCase::IgnoreCase))
    {
        if (Percent>=99.95) return TEXT("INTACT");
        if (Percent>=75.0) return TEXT("LIGHT DMG");
        if (Percent>=50.0) return TEXT("MOD DMG");
        if (Percent>=25.0) return TEXT("HEAVY DMG");
        return Percent>0.0 ? TEXT("CRITICAL") : TEXT("DESTROYED");
    }
    if (Label.Equals(TEXT("THRUST"),ESearchCase::IgnoreCase) && bAugmenter) return TEXT("AUG");
    if (Percent<0.5) return Label.Equals(TEXT("FUEL"),ESearchCase::IgnoreCase) ? TEXT("EMPTY") : TEXT("OFF");
    if (Percent<33.0) return TEXT("LOW");
    if (Percent<66.0) return TEXT("MED");
    // Charge is sampled between recharge frames: near-full is presented as FULL.
    return Percent<99.0 ? TEXT("HIGH") : TEXT("FULL");
}

SFighterHUDDetails::~SFighterHUDDetails()=default;

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
    // The carrier's Hangar owns the authoritative staged-aircraft state.
    // Do not infer readiness from the Unreal mesh or from ship flight phase.
    bool FindPlayerAlertSlot(Ship* Player, Hangar*& OutHangar, int& OutSquadron, int& OutSlot)
    {
        OutHangar = nullptr;
        OutSquadron = -1;
        OutSlot = -1;
        if (!Player || Player->IsDead()) return false;
        Ship* Carrier = Player->GetCarrier();
        if (!Carrier && Player->GetElement()) Carrier = Player->GetElement()->GetCarrier();
        Hangar* Candidate = Carrier ? Carrier->GetHangar() : nullptr;
        if (!Candidate || !Candidate->FindSquadronAndSlot(Player, OutSquadron, OutSlot)) return false;
        const HangarSlot* Slot = Candidate->GetSlot(OutSquadron, OutSlot);
        if (!Slot || Candidate->GetState(Slot) != Hangar::ALERT ||
            Candidate->GetShip(Slot) != Player || !Candidate->GetFlightDeck(Slot) ||
            !Candidate->GetFlightDeck(Slot)->IsLaunchDeck()) return false;
        OutHangar = Candidate;
        return true;
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
        return FString::Printf(TEXT("/Game/UI/Ships/%s/%s.%s"), *ShipVisualCatalog::IconFolder(D->ShipName), *Icon, *Icon);
    }

    FLinearColor SensorContactColor(const SimContact* Contact, Ship* Observer)
    {
        if (!Contact || !Observer)
            return FLinearColor(0.45f, 0.45f, 0.45f, 1.0f); // unknown gray

        return SFighterHUDDetails::IdentificationColor(Contact->GetIFF(Observer),Observer->GetIFF());
    }
    // Do not enable launch merely because Hangar has allocated an ALERT ship.
    // The physical deck slot must still contain that fighter and be READY.
    bool IsPlayerAtLaunchPosition(Ship* Player, Hangar* PlayerHangar, int Squadron, int Slot)
    {
        if (!Player || !PlayerHangar || Player->GetFlightPhase() != EOPSMode::ALERT)
            return false;
        Sim* LiveSim = Sim::GetSim();
        if (LiveSim && LiveSim->IsTrainingLaunchLeaderPending())
            return false;
        const HangarSlot* Staged = PlayerHangar->GetSlot(Squadron, Slot);
        if (!Staged || PlayerHangar->GetState(Staged) != Hangar::ALERT)
            return false;
        FlightDeck* Deck = PlayerHangar->GetFlightDeck(Staged);
        const int DeckSlot = PlayerHangar->GetFlightDeckSlot(Staged);
        return Deck && Deck->IsLaunchDeck() && DeckSlot >= 0 &&
            DeckSlot < Deck->NumSlots() &&
            Deck->GetShip(DeckSlot) == Player &&
            Deck->State(DeckSlot) == FlightDeck::READY;
    }

}
// Match GEAR DOWN width; double its height for a larger launch target.
// GEAR DOWN is at HUD center +126; LAUNCH is below it at +158.
static FVector2D GearButtonOrigin(FVector2D Size,float Scale,bool Mini)
{
    return Mini ? FVector2D(Size.X*.5-330*Scale,20*Scale)+FVector2D(-49,78)*Scale
                : FVector2D(182*Scale,Size.Y-36*Scale);
}
static FSlateRect CarrierLaunchButtonRect(const FVector2D& Size, float Scale)
{
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold",
        FMath::Max(8, int32(11 * Scale)));
    const FVector2D Label = FSlateApplication::Get().GetRenderer()->
        GetFontMeasureService()->Measure(TEXT("LAUNCH"), Font);
    const FVector2D Extent(Label.X + 10 * Scale, (Label.Y + 4 * Scale) * 2.0);
    const FVector2D TopLeft(Size.X * 0.5 - Extent.X * 0.5,
        Size.Y * 0.5 - Extent.Y * 0.5);
    return FSlateRect(TopLeft.X, TopLeft.Y,
        TopLeft.X + Extent.X, TopLeft.Y + Extent.Y);
}

void SFighterHUDDetails::Construct(const FArguments& A)
{
    Symbology=MakeUnique<FNativeHUDSymbology>();
    ThirdPerson = A._ThirdPerson;
    NetworkController = A._NetworkController;
    ShowHUD = A._ShowHUD; SelectedTarget = A._SelectedTarget; SelectedName = A._SelectedName;
    SetVisibility(EVisibility::Visible);
    ForceVolatile(true);
    LaunchTexture.Reset(LoadObject<UTexture2D>(nullptr,
        TEXT("/Game/UI/HUD/TAC_button.TAC_button")));
    LaunchBrush.DrawAs = ESlateBrushDrawType::Image;
    LaunchBrush.SetResourceObject(LaunchTexture.Get());
    LaunchBrush.ImageSize = FVector2D(96, 20);
    UE_LOG(LogTemp, Display, TEXT("[CarrierLaunch][BUTTON_TEXTURE] %s"),
        LaunchTexture.IsValid() ? TEXT("Loaded TAC_button") : TEXT("Missing TAC_button"));
    ILSTexture.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/HUD/ILS.ILS")));
    ILSBrush.SetResourceObject(ILSTexture.Get());
    ILSBrush.DrawAs = ESlateBrushDrawType::Image;
    ILSBrush.ImageSize = FVector2D(360,180);
    PlayerBrush.DrawAs = TargetBrush.DrawAs = ESlateBrushDrawType::Image;
    PlayerBrush.ImageSize = TargetBrush.ImageSize = FVector2D(96, 96);
    const TCHAR* Names[] = { TEXT("TAC_left"), TEXT("TAC_right"), TEXT("sensor_fov"), TEXT("sensor_hsd"), TEXT("sensor_3d"), TEXT("CAUTION_left"), TEXT("CAUTION_right"), TEXT("INSTR_left"), TEXT("INSTR_right") };
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
    ChildSlot
    [ SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom)
        [ SNew(SBox)
            .WidthOverride_Lambda([this]{ return 326.f * InformationScale; })
            .HeightOverride_Lambda([this]{ return 124.f * InformationScale; })
            .Visibility_Lambda([this]{ return ShowHUD.Get() && bShowInformation && !InformationCaptions.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
            [ SNew(SBox)
                .Padding_Lambda([this]{ return FMargin(12,29,12,10) * InformationScale; })
                [ SAssignNew(InformationScroll,SScrollBox)
                    .Clipping(EWidgetClipping::ClipToBoundsAlways)
                    + SScrollBox::Slot()
                    [ SAssignNew(InformationRows,SVerticalBox) ]
                ]
            ]
        ]
    ];

}
void SFighterHUDDetails::Tick(const FGeometry& G, double Now, float Delta)
{
    SCompoundWidget::Tick(G, Now, Delta);
    const FVector2D Size=G.GetLocalSize();
    InformationScale=FMath::Min(Size.X/1280.0,Size.Y/720.0);
    if (NetworkController.IsValid())
    {
        if (Now >= NextSample)
        {
            NextSample = Now + 0.1;
            Refresh();
        }
        return;
    }
    UpdateInformationMessages();
    bWarningFlash = FMath::Fmod(Now, 1.0) >= 0.5;
    if (Ship* Player = PlayerShip())
    {
        if (PanelPlayerId != Player->GetIdentity())
        {
            PanelPlayerId=Player->GetIdentity();bShowCaution=false;bShowInformation=true;
        }
        UpdateILSMode(Player->GetIdentity(), Player->GetInbound() != nullptr);
        PitchDegrees = FMath::RadiansToDegrees(Player->GetCompassPitch());
        HeadingDegrees = FMath::Fmod(FMath::RadiansToDegrees(Player->GetCompassHeading()) + 360.0, 360.0);
        const FVector Right = Player->GetCam().vrt();
        const FVector Up = Player->GetCam().vup();
        // Simulation uses Y as world up. Bank is undefined exactly at vertical.
        if (FMath::Square(Right.Y) + FMath::Square(Up.Y) > 1.e-8)
            BankRadians = FMath::Atan2(Right.Y, Up.Y);
    }
    else { PitchDegrees = BankRadians = 0; UpdateILSMode(INDEX_NONE, false); }
    if(Symbology)Symbology->Refresh(PlayerShip());
    if (Now < NextSample) return;
    NextSample = Now + 0.1;
    Refresh();
    if (!CockpitDisplays) CockpitDisplays = MakeUnique<FCockpitDisplays>();
    Ship* Current = PlayerShip();
    CockpitDisplays->Update(Current ? Cast<AFighterActor>(Current->GetNativePresentation()) : nullptr, SharedThis(this), ShowHUD.Get());
}
bool SFighterHUDDetails::IsCockpitDisplayActive() const { return CockpitDisplays && CockpitDisplays->IsActive(); }
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
    bAugmenterActive = false;
    TargetDisplayColor = FLinearColor::Yellow;
    HomeCarrierReadout.Reset();
    NavReadouts.Reset(); MissileReadouts.Reset(); DefenseReadout.Reset();
    bAutoAvailable = bGearDown = bShoot = bClosingValid = bTargetLocked = bTargetTracking = false;
    FlightSpeed = HeadingDegrees = ClosingSpeed = JumpSeconds = 0;
    ThreatLevel = 0;
    bHelmMode=false;CompactNavLabel=TEXT("FREE FLIGHT");
    if (auto* Controller = NetworkController.Get())
    {
        const auto* Snapshot = Controller->GetAssignedShipState();
        bHasPlayer = Snapshot != nullptr;
        bHasTarget = false;
        if (!Snapshot)
        {
            if (Symbology) Symbology->RefreshNetwork(Controller, nullptr, nullptr);
            return;
        }

        const FSSWReplicatedShipState* Target = nullptr;
        FlightSpeed = Snapshot->Velocity.Size();
        const FVector Heading = Snapshot->Heading;
        const FVector Right = FVector::CrossProduct(Snapshot->Up, Heading).GetSafeNormal();
        BankRadians = FMath::Atan2(Right.Y, Snapshot->Up.Y);
        HeadingDegrees = FMath::RadiansToDegrees(FMath::Atan2(Heading.X, Heading.Z));
        PitchDegrees = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Heading.Y, -1.0, 1.0)));
        PlayerHull = Snapshot->HullPercent;
        PlayerName = Snapshot->Name;
        PlayerClass = Snapshot->Design;
        if (!PlayerMFDPreview) PlayerMFDPreview = MakeUnique<FShipMFDPreview>(true);
        PlayerMFDPreview->SetDesign(ShipDesignRegistry::Find(Snapshot->Design));
        SensorRange = FMath::Max(1.0, double(Snapshot->SensorRange));
        if (const auto* Ships = Controller->GetFlightContacts())
            for (const auto& Contact : *Ships)
            {
                if (Contact.ObjectId == Snapshot->ObjectId || Contact.Region != Snapshot->Region ||
                    Contact.bDestroyed || Contact.bRetired) continue;
                const FVector Delta = Contact.Position - Snapshot->Position;
                const double Range = Delta.Size();
                if (Contact.ObjectId == Snapshot->TargetId)
                {
                    Target = &Contact;
                    bHasTarget = bTargetLocked = true;
                    bTargetTracking = Snapshot->bAutoTracking;
                    TargetName = Contact.Name;
                    TargetClass = Contact.Design;
                    TargetHull = Contact.HullPercent;
                    TargetShield = Contact.ShieldStrength;
                    TargetRange = Range / 1000.0;
                    TargetDisplayColor = Contact.IFF == Snapshot->IFF ? HUDBlue : FLinearColor::Red;
                    if (!TargetMFDPreview) TargetMFDPreview = MakeUnique<FShipMFDPreview>(true);
                    TargetMFDPreview->SetDesign(ShipDesignRegistry::Find(Contact.Design));
                    const auto ToUE = [](const FVector& V) { return FVector(V.Z, V.X, V.Y); };
                    const FQuat TargetRotation = FRotationMatrix::MakeFromXZ(
                        ToUE(Contact.Heading), ToUE(Contact.Up)).ToQuat();
                    TargetMFDPreview->SetView(TargetRotation,
                        Snapshot->VisualPosition - Contact.VisualPosition, ToUE(Snapshot->Up));
                }
                if (Range > SensorRange || Range <= UE_SMALL_NUMBER) continue;
                const double Az = FMath::Atan2(FVector::DotProduct(Delta, Right), FVector::DotProduct(Delta, Heading));
                const double El = FMath::Asin(FMath::Clamp(FVector::DotProduct(Delta, Snapshot->Up) / Range, -1.0, 1.0));
                Contacts.Add({Az, El, Range, Contact.IFF == Snapshot->IFF ? HUDBlue : FLinearColor::Red});
            }
        if (Symbology) Symbology->RefreshNetwork(Controller, Snapshot, Target);
        bShoot = Target && Snapshot->bMissileLocked;
        Primary = Snapshot->PrimaryReadout;
        Secondary = Snapshot->SecondaryReadout;
        NetworkEmcon = Snapshot->Emcon;
        CompactNavLabel = Snapshot->bDestroyed ? TEXT("DESTROYED") :
            Snapshot->bRetired ? TEXT("RETIRED") : TEXT("MULTIPLAYER");
        StatusRows.Add({TEXT("THRUST"), Snapshot->Throttle});
        StatusRows.Add({TEXT("FUEL"), double(Snapshot->FuelPercent)});
        StatusRows.Add({TEXT("HULL"), Snapshot->HullPercent});
        StatusRows.Add({TEXT("SHIELD"), double(Snapshot->ShieldStrength)});
        return;
    }
    Ship* P = PlayerShip(); bHasPlayer = P != nullptr;
    PlayerHull = Hull(P); Primary = Secondary = TEXT("NONE");
    if (P)
    {
        FlightSpeed = P->GetVelocity().Size();
        OrderedHeading=FMath::RadiansToDegrees(P->GetHelmHeading());OrderedPitch=FMath::RadiansToDegrees(P->GetHelmPitch());
        bHelmMode=P->GetFLCSMode()==EFLCSMode::HELM;
        HeadingDegrees = FMath::Fmod(FMath::RadiansToDegrees(P->GetCompassHeading()) + 360.0, 360.0);
        bAutoAvailable = P->IsAutoNavEngaged() || P->IsAutoLandEngaged() || P->IsAutoTrackEngaged();
        bGearDown = P->IsGearDown();
        bAugmenterActive = P->GetMainDrive() && P->GetMainDrive()->IsAugmenterOn();
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
        if (InboundSlot* Arrival = P->GetInbound())
        {
            CompactNavLabel=P->IsAutoLandEngaged()?TEXT("AUTO LAND"):TEXT("INBOUND");
            NavReadouts.Add(P->IsAutoLandEngaged() ? TEXT("AUTO LAND") : TEXT("INBOUND"));
            NavReadouts.Add(Arrival->Cleared() ? TEXT("CLEARED TO LAND") : TEXT("HOLD FOR CLEARANCE"));
            if (FlightDeck* Deck = Arrival->GetDeck())
                NavReadouts.Add(FString(ANSI_TO_TCHAR(Deck->GetName())));
        }
        else if (Instruction* Nav = P->GetNextNavPoint())
        {
            CompactNavLabel=FString::Printf(TEXT("NAV %02d  %hs"),P->GetNavIndex(Nav),Nav->GetRegionName());
            NavReadouts.Add(FString::Printf(TEXT("%s %d"), P->IsAutoNavEngaged() ? TEXT("AUTO NAV") : TEXT("NAV"), P->GetNavIndex(Nav)));
            const char* Action = Instruction::ActionName(Nav->GetAction());
            if (Action && *Action) NavReadouts.Add(FString(ANSI_TO_TCHAR(Action)).ToUpper());
            NavReadouts.Add(FString::Printf(TEXT("SPD %d"), Nav->GetSpeed()));
            const double Distance = FMath::Max(0.0, P->RangeToNavPoint(Nav));
            NavReadouts.Add(UFormattingUtils::FormatDistance(Distance));
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
        if (!P->IsStarship()) {
            Ship* HomeCarrier = P->GetElement() ? P->GetElement()->GetCarrier() : nullptr;
            if (!HomeCarrier) HomeCarrier = P->GetCarrier();
            HomeCarrierReadout = HomeCarrier && !HomeCarrier->IsDead()
                ? FString::Printf(TEXT("HOME %hs"), HomeCarrier->GetName()) : TEXT("HOME NONE");
        }
        PlayerName = ANSI_TO_TCHAR(P->GetName());
        PlayerClass = ANSI_TO_TCHAR(P->GetShipClassName());
        PlayerShield = P->GetShield() ? P->GetShieldStrength() : -1;
        if(!PlayerMFDPreview)PlayerMFDPreview=MakeUnique<FShipMFDPreview>(true);
        PlayerMFDPreview->SetDesign(P->Design()?ShipDesignRegistry::Find(P->Design()->name):nullptr);
        if (P->GetMainDrive()) StatusRows.Add({TEXT("THRUST"), P->GetThrottle()});
        if (P->GetReactors().size() > 0 && P->GetReactors()[0])
            StatusRows.Add({TEXT("FUEL"), double(P->GetReactors()[0]->GetCharge())});
        if (P->GetQuantumDrive()) StatusRows.Add({TEXT("QUANTUM"), Charge(P->GetQuantumDrive())});
        StatusRows.Add({TEXT("HULL"), PlayerHull});
        if (P->GetShield()) StatusRows.Add({TEXT("SHIELD"), double(P->GetShieldStrength())});
        if (P->GetPrimary()) StatusRows.Add({TEXT("GUNS"), Charge(P->GetPrimary())});
        Primary = FormatWeaponReadout(P->GetPrimaryGroup()); Secondary = FormatWeaponReadout(P->GetSecondaryGroup());
        // Legacy warning cells remain in fixed positions as damage changes.
        SSWDamagePanel::Build(P, DamageRows);
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
                if (!FMath::IsFinite(Range) || !FMath::IsFinite(Az) || !FMath::IsFinite(El) || Range <= 0) continue;
                Range = It->Range(P, SensorRange);
                // Use the sensor contact's perceived IFF, not the underlying ship's true IFF.
                // Threat status must not override identification color.
                const FLinearColor C = SensorContactColor(It.value(), P);
                Contacts.Add({Az, El, Range, C});
            }
        }
    }
    Sim* S = Sim::GetSim(); SimRegion* R = S ? S->GetActiveRegion() : nullptr;
    AShipActor* A = Cast<AShipActor>(SelectedTarget.Get().Get());
    Ship* T = IsValid(A) ? A->GetRuntimeShip() : nullptr;
    if (!SelectedTarget.Get().IsValid() && !T && P && R)
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
            Sensor* PlayerSensor = P->GetSensor();
            bTargetLocked = P->GetTarget() == T && PlayerSensor && PlayerSensor->IsTracking(T);
            bTargetTracking = P->IsAutoTrackEngaged() && P->GetAutoTrackTarget() == T;
            const FVector Separation = T->GetLocation()-P->GetLocation();
            bClosingValid = !Separation.IsNearlyZero();
            ClosingSpeed = FVector::DotProduct(P->GetVelocity()-T->GetVelocity(), Separation.GetSafeNormal());
        }
        TargetName = IsValid(A) ? SelectedName.Get().ToString() : FString(ANSI_TO_TCHAR(T->GetName()));
        TargetClass = ANSI_TO_TCHAR(T->GetShipClassName());
        // Identification is relative to the observing ship, not mission team 0.
        TargetDisplayColor=IdentificationColor(T->GetIFF(),P?P->GetIFF():-1);
        TargetHull = Hull(T); TargetShield = T->GetShield() ? T->GetShieldStrength() : -1;
        TargetRange = P ? FVector::Distance(P->GetLocation(), T->GetLocation()) / 1000.0 : -1;
        if(!TargetMFDPreview)TargetMFDPreview=MakeUnique<FShipMFDPreview>(true);
        TargetMFDPreview->SetDesign(T->Design()?ShipDesignRegistry::Find(T->Design()->name):nullptr);
        AShipActor* TargetActor=T->GetNativePresentation();
        if(!IsValid(TargetActor))TargetActor=A;
        AShipActor* PlayerActor=P?P->GetNativePresentation():nullptr;
        if(IsValid(TargetActor) && IsValid(PlayerActor))
            TargetMFDPreview->SetView(TargetActor->GetActorQuat(),
                PlayerActor->GetActorLocation()-TargetActor->GetActorLocation(),PlayerActor->GetActorUpVector());
    }
    else
    {
        AActor* SelectedObject = SelectedTarget.Get().Get();
        const bool bWaypoint = IsValid(SelectedObject) && SelectedObject->IsA<AWaypointActor>();
        FMissionObjectData ObjectData;
        const bool bObjectAvailable = ReadMissionObject(SelectedObject, P, R, ObjectData);
        if (bWaypoint && bObjectAvailable)
        {
            if (!TargetMFDPreview) TargetMFDPreview = MakeUnique<FShipMFDPreview>(true);
            TargetMFDPreview->SetWaypoint();
        }
        else if (TargetMFDPreview)
            TargetMFDPreview->SetDesign(nullptr);
        if (bObjectAvailable)
        {
            bHasTarget = true;
            TargetName = ObjectData.Name; TargetClass = ObjectData.Type;
            TargetRange = ObjectData.RangeKm; TargetHull = -1; TargetShield = -1;
            if (TargetIconPath != ObjectData.ImagePath)
            {
                TargetIconPath = ObjectData.ImagePath;
                TargetTexture.Reset(ObjectData.ImagePath.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *ObjectData.ImagePath));
                TargetBrush.SetResourceObject(TargetTexture.Get());
            }
        }
    }
}
void SFighterHUDDetails::CycleMFD(int32 Index)
{
    if (Index == 0)
    {
        Modes[0] = EMode::Ship;
        return;
    }
    Modes[1] = Modes[1] == EMode::FOV ? EMode::HSD
        : Modes[1] == EMode::HSD ? EMode::ThreeD
        : EMode::FOV;
    Invalidate(EInvalidateWidgetReason::Paint);
}

FReply SFighterHUDDetails::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FVector2D Size = MyGeometry.GetLocalSize();
    const float Scale = FMath::Min(Size.X / 1280.0f, Size.Y / 720.0f);
    const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    if(MouseEvent.GetEffectingButton()==EKeys::LeftMouseButton)
    {
        Ship* P=PlayerShip();
        const bool bPreTakeoff = P && P->GetDock() && P->GetDock()->IsLaunchDeck() && P->GetFlightPhase()<EOPSMode::ACTIVE;
        if (!bPreTakeoff && CockpitDisplays && CockpitDisplays->HandleGearClick()) return FReply::Handled();
        const bool Mini=ThirdPerson.Get() && !bPreTakeoff && !IsCockpitDisplayActive();
        if(!bPreTakeoff && !IsCockpitDisplayActive() && (Mini || Modes[0] == EMode::Ship) && P && P->GetGear() && SSWGearIndicator::Contains(Local,GearButtonOrigin(Size,Scale,Mini),FVector2D(28,26)*Scale))
        {P->ToggleGear();Refresh();return FReply::Handled();}
    }
    const FVector2D Right(Size.X - 220.f * Scale, Size.Y - 215.f * Scale);
    const FVector2D Extent(220.f * Scale, 215.f * Scale);
    // Legacy carrier launch: clickable only for an aircraft currently staged
    // in ALERT. The player controls the departure, never the mission loader.
    // The centered Launch button stays inert until the slot is ready.
    const FSlateRect LaunchRect = CarrierLaunchButtonRect(Size, Scale);
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
        Local.X >= LaunchRect.Left && Local.X <= LaunchRect.Right &&
        Local.Y >= LaunchRect.Top && Local.Y <= LaunchRect.Bottom)
    {
        Hangar* PlayerHangar = nullptr;
        int Squadron = -1, Slot = -1;
        Ship* P = PlayerShip();
        if (FindPlayerAlertSlot(P, PlayerHangar, Squadron, Slot))
        {
            if (!IsPlayerAtLaunchPosition(P, PlayerHangar, Squadron, Slot))
                return FReply::Handled(); // Visible but disabled while positioning.
            if (Sim* LiveSim = Sim::GetSim())
                LiveSim->RequestPlayerCarrierLaunch();
            return FReply::Handled();
        }
    }
    if (Local.X >= Right.X && Local.Y >= Right.Y && Local.X <= Right.X + Extent.X && Local.Y <= Right.Y + Extent.Y)
    {
        CycleMFD(1);
        return FReply::Handled();
    }
    return FReply::Unhandled();
}
void SFighterHUDDetails::CycleWeapon(bool bPrimary)
{
    if (Ship* P = PlayerShip()) { if (bPrimary) P->CyclePrimary(); else P->CycleSecondary(); Refresh(); }
}
int32 SFighterHUDDetails::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
    FSlateWindowElementList& E, int32 L, const FWidgetStyle& Style, bool Enabled) const
{
    if (!ShowHUD.Get()) return L;
    const double LowerReadoutBaseline = 30.0;
    const double LowerReadoutRowHeight = 14.0;
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
        const bool bThrust = Label == TEXT("THRUST");
        const FLinearColor C = bThrust ? HUDBlue : DamageColor(Percent);
        Text(P, Label.Left(12), C);
        Box(P+FVector2D(88,5)*Scale,FVector2D(50,6)*Scale,FLinearColor(0.05f,0.1f,0.18f,0.8f));
        if (Percent >= 0) Box(P+FVector2D(88,5)*Scale,FVector2D(50*FMath::Clamp(Percent/100.0,0.0,1.0),6)*Scale,C);
        Text(P+FVector2D(145,0)*Scale,FormatStatusReadout(Label,Percent,bThrust && bAugmenterActive),C);
    };
    auto Icon = [&](FVector2D P, const FSlateBrush& B, bool Valid, double H)
    {
        if (Valid) FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(80*Scale,80*Scale),FSlateLayoutTransform(FVector2f(P))),&B,ESlateDrawEffect::None,H < 0 ? FLinearColor::White : ShipHealthColor(H));
    };
    auto Artwork = [&](int32 Index, FVector2D P, FVector2D D)
    {
        if (PanelTextures[Index].IsValid())
            FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(D),FSlateLayoutTransform(FVector2f(P))),
                &PanelBrushes[Index],ESlateDrawEffect::None,FLinearColor(HUDBlue.R,HUDBlue.G,HUDBlue.B,0.9f));
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
    Ship* HUDPlayer=NetworkController.IsValid() ? nullptr : PlayerShip();
    const bool bPreTakeoff=HUDPlayer && HUDPlayer->GetDock() && HUDPlayer->GetDock()->IsLaunchDeck() && HUDPlayer->GetFlightPhase()<EOPSMode::ACTIVE;
    const bool bCapitalWeapons = HUDPlayer && HUDPlayer->IsStarship();
    const bool bThirdPerson = ThirdPerson.Get();
    const bool bMiniHUD = bHasPlayer && bThirdPerson && !bPreTakeoff && !IsCockpitDisplayActive();
    if (!IsCockpitDisplayActive())
    {
    if(!bPreTakeoff && bHasPlayer && (bMiniHUD || Modes[0] == EMode::Ship) && HUDPlayer && HUDPlayer->GetGear())SSWGearIndicator::Draw(E,G,L+5,GearButtonOrigin(Size,Scale,bMiniHUD),FVector2D(28,26)*Scale,HUDPlayer);
    if (bHasPlayer && !ThirdPerson.Get() && !bPreTakeoff)
    {

        // Attitude instrument in the central ring: 5 degree pitch marks, banked with the ship.
        const FVector2D Center(Size.X*0.5, Size.Y*0.5);
        const double C = FMath::Cos(BankRadians), S = FMath::Sin(BankRadians);
        auto AttitudePoint = [&](double X, double Y)
        { return Center + FVector2D(C*X-S*Y,S*X+C*Y)*Scale; };
        auto Segment = [&](FVector2D A, FVector2D B)
        {
            TArray<FVector2D> Points; Points.Add(A); Points.Add(B);
            FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,HUDBlue,true,Scale);
        };
        for (int32 Angle=-90; Angle<=90; Angle+=5)
        {
            const double Y=(PitchDegrees-Angle)*3.0;
            if (FMath::Abs(Y)>65.0) continue;
            if (Angle<0)
            {
                for (int32 X=18; X<50; X+=12)
                {
                    Segment(AttitudePoint(X,Y),AttitudePoint(X+7,Y));
                    Segment(AttitudePoint(-X,Y),AttitudePoint(-X-7,Y));
                }
            }
            else
            {
                const double Edge=Angle==0?60.0:50.0;
                Segment(AttitudePoint(-Edge,Y),AttitudePoint(-18,Y));
                Segment(AttitudePoint(18,Y),AttitudePoint(Edge,Y));
            }
            if (Angle%10==0) Text(AttitudePoint(55,Y-5),FString::FromInt(Angle),HUDBlue);
        }
        // Heading tape wraps through north without jumping.
        for (int32 Offset=-30; Offset<=30; Offset+=5)
        {
            const int32 Mark=FMath::FloorToInt(HeadingDegrees/5.0)*5+Offset;
            const double X=(Mark-HeadingDegrees)*2.0;
            if (FMath::Abs(X)>55.0) continue;
            Segment(Center+FVector2D(X,-82)*Scale,Center+FVector2D(X,-82+(Mark%10==0?6:3))*Scale);
        }
        ReticleText(-30,74,60,FString::Printf(TEXT("BANK %+.0f"),FMath::RadiansToDegrees(BankRadians)),true);
        // Shared baseline keeps speed and range level across HUD scales.
        const double SpeedRangeBaseline = -8.0;
        ReticleText(-256,SpeedRangeBaseline,108,FString::Printf(TEXT("%.0f"),FlightSpeed));
        ReticleText(-40,-100,80,FString::Printf(TEXT("%03d"),FMath::RoundToInt(HeadingDegrees)%360),true);
        ReticleText(-248,FMath::Max(88.0,LowerReadoutBaseline+(3+MissileReadouts.Num())*LowerReadoutRowHeight),108,TEXT("TAC"));
        if (bShowWeapons && !bCapitalWeapons)
        {
            ReticleText(-248,LowerReadoutBaseline+2*LowerReadoutRowHeight,108,DefenseReadout);
            for (int32 I=0; I<MissileReadouts.Num(); ++I)
                ReticleText(-248,LowerReadoutBaseline+(3+I)*LowerReadoutRowHeight,108,MissileReadouts[I]);
        }
        for (int32 I=0; I<NavReadouts.Num(); ++I)
            ReticleText(128,LowerReadoutBaseline+I*LowerReadoutRowHeight,108,NavReadouts[I]);
        if (bHasTarget && !bPreTakeoff)
        {
            ReticleText(128,SpeedRangeBaseline,108,FString(TEXT("RNG ")) + UFormattingUtils::FormatDistance(TargetRange * 1000.0));
            if (bClosingValid) ReticleText(128,SpeedRangeBaseline+14,108,FString::Printf(TEXT("CLS %+.0f M/S"),ClosingSpeed));
        }
        if (bAutoAvailable)
            Popup(-142,HUDPlayer && HUDPlayer->IsAutoTrackEngaged() ? TEXT("AUTO TRACK") : TEXT("AUTO"),HUDBlue);
        if (JumpSeconds>0) Popup(-168,FString::Printf(TEXT("QUANTUM JUMP: %d"),FMath::CeilToInt(JumpSeconds)));
        else if (ThreatLevel==2 && bWarningFlash) Popup(-168,TEXT("MISSILE WARNING"),FLinearColor::Red);
        else if (ThreatLevel==1) Popup(-168,TEXT("LOCK WARNING"),FLinearColor::Yellow);
        if (bShoot && bShowWeapons) Popup(152,TEXT("SHOOT"));

    }
    if (bHasPlayer && bMiniHUD)
    {
        // Compact flight deck: clear central view, stable alignment and restrained glass.
        const FVector2D P(Size.X*.5-330*Scale,20*Scale);
        const FVector2D Frame=P-FVector2D(220*Scale,0),D(880*Scale,112*Scale);
        static const FSlateRoundedBoxBrush Chassis(FLinearColor::White, 12.f);
        static const FSlateRoundedBoxBrush Inset(FLinearColor::White, 6.f);
        auto Surface = [&](const FSlateRoundedBoxBrush& Brush, FVector2D Position, FVector2D Extent, FLinearColor Tint)
        {
            FSlateDrawElement::MakeBox(E,L,G.ToPaintGeometry(FVector2f(Extent),FSlateLayoutTransform(FVector2f(Position))),
                &Brush,ESlateDrawEffect::None,Tint);
        };
        Surface(Chassis, Frame, D, FLinearColor(.09f,.25f,.34f,.90f));
        Surface(Chassis, Frame+FVector2D(1,1)*Scale, D-FVector2D(2,2)*Scale, FLinearColor(.008f,.019f,.032f,.96f));
        Surface(Inset, P+FVector2D(-211,7)*Scale, FVector2D(192,98)*Scale, FLinearColor(.022f,.051f,.072f,.91f));
        Surface(Inset, P+FVector2D(518,24)*Scale, FVector2D(134,80)*Scale, FLinearColor(.015f,.036f,.054f,.93f));
        for (double X : {-204.0, 120.0, 244.0, 366.0, 524.0})
            Box(P+FVector2D(X,2)*Scale,FVector2D(24*Scale,Scale),FLinearColor(.28f,.76f,.95f,.8f));

        const FLinearColor Muted(.48,.72,.82,.95),White(.88,.96,1,1),Green(.25,1,.58,1);
        const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        auto MiniText=[&](double X,double Y,FString Value,int SizeIn,FLinearColor Tint,double Width=0){
            const FSlateFontInfo Font=SizeIn>=20?FCoreStyle::GetDefaultFontStyle("Bold",FMath::Max(10,int(SizeIn*Scale))):MissionUIStyle::GetInfoLabelFont(FMath::Max(9,int(SizeIn*Scale)));
            if (Width > 0 && Measure->Measure(Value,Font).X > Width*Scale)
            {
                while (!Value.IsEmpty() && Measure->Measure(Value+TEXT("..."),Font).X > Width*Scale)
                    Value.LeftChopInline(1);
                Value += TEXT("...");
            }
            FSlateDrawElement::MakeText(E,L+2,G.ToPaintGeometry(FVector2f(430*Scale,30*Scale),FSlateLayoutTransform(FVector2f(P+FVector2D(X,Y)*Scale))),Value,Font,ESlateDrawEffect::None,Tint);
        };
        for (double X : {-12.0, 104.0, 228.0, 350.0, 510.0})
            Box(P+FVector2D(X,18)*Scale,FVector2D(Scale,76*Scale),FLinearColor(.2,.4,.5,.35));

        MiniText(-204,10,TEXT("SHIP STATUS"),10,Muted);
        if (HUDPlayer || NetworkController.IsValid())
            MiniText(-78,10,FString::Printf(TEXT("EMCON %d"),HUDPlayer ? HUDPlayer->GetEMCON() : NetworkEmcon),10,Muted);
        for (int32 I=0; I<StatusRows.Num(); ++I)
        {
            const auto& Row = StatusRows[I];
            const bool bThrust = Row.Label == TEXT("THRUST");
            const FLinearColor Tint = bThrust ? HUDBlue : DamageColor(Row.Percent);
            const double Y = 24 + I*13.0;
            MiniText(-204,Y,Row.Label,10,Muted);
            Box(P+FVector2D(-137,Y+4)*Scale,FVector2D(47,3)*Scale,FLinearColor(.07,.16,.23,.9));
            if (Row.Percent >= 0)
                Box(P+FVector2D(-137,Y+4)*Scale,FVector2D(47*FMath::Clamp(Row.Percent/100.0,0.0,1.0),3)*Scale,Tint);
            MiniText(-82,Y,FormatStatusReadout(Row.Label,Row.Percent,bThrust && bAugmenterActive),10,Tint,65);
        }

        MiniText(120,14,TEXT("VELOCITY"),10,Muted);
        const FString Speed = NetworkController.IsValid() && FlightSpeed > 0.0 && FlightSpeed < 10.0
            ? FString::Printf(TEXT("%.2f"), FlightSpeed)
            : FString::Printf(TEXT("%.0f"), FlightSpeed);
        const FLinearColor SpeedColor = bAugmenterActive ? Green : White;
        MiniText(120,37,Speed,26,SpeedColor);
        const auto SpeedFont=FCoreStyle::GetDefaultFontStyle("Bold",FMath::Max(10,int(26*Scale)));
        const FVector2D SpeedExtent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Speed,SpeedFont);
        const auto UnitsFont=MissionUIStyle::GetInfoLabelFont(FMath::Max(9,int(10*Scale)));
        const FVector2D UnitsExtent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT("m/s"),UnitsFont);
        const FVector2D UnitsPosition=P+FVector2D(120*Scale+SpeedExtent.X+7*Scale,37*Scale+SpeedExtent.Y-UnitsExtent.Y);
        FSlateDrawElement::MakeText(E,L+2,G.ToPaintGeometry(FVector2f(UnitsExtent),FSlateLayoutTransform(FVector2f(UnitsPosition))),TEXT("m/s"),UnitsFont,ESlateDrawEffect::None,bAugmenterActive ? Green : Muted);
        const auto* Controller = NetworkController.Get();
        const auto* Snapshot = Controller ? Controller->GetAssignedShipState() : nullptr;
        const bool bAtmospheric = Snapshot ? Snapshot->bAirborne : HUDPlayer && HUDPlayer->IsAirborne();
        if (bAtmospheric)
        {
            const FVector2D DialCenter = P + FVector2D(56,48)*Scale;
            constexpr double Radius = 36.0;
            const double Horizon = FMath::Clamp(PitchDegrees*1.35,-Radius,Radius);
            const double BankCos = FMath::Cos(BankRadians);
            const double BankSin = FMath::Sin(BankRadians);
            auto DialPoint = [&](double X,double Y)
            {
                return DialCenter + FVector2D(X*BankCos-Y*BankSin,X*BankSin+Y*BankCos)*Scale;
            };
            auto DialLine = [&](FVector2D A,FVector2D B,FLinearColor Color)
            {
                FSlateDrawElement::MakeLines(E,L+4,G.ToPaintGeometry(),{A,B},
                    ESlateDrawEffect::None,Color,true,1.25f);
            };
            const auto* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
            const auto Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
            auto FillArc = [&](double Start,double End,FColor Color)
            {
                TArray<FSlateVertex> Vertices;
                TArray<SlateIndex> Indices;
                const auto Transform = G.GetAccumulatedRenderTransform();
                for (int32 Index=0; Index<=32; ++Index)
                {
                    const double Angle = FMath::Lerp(Start,End,Index/32.0);
                    const FVector2D Point = DialPoint(Radius*FMath::Cos(Angle),Radius*FMath::Sin(Angle));
                    Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
                        Transform,FVector2f(Point),FVector2f(.5f,.5f),Color));
                    if (Index>=2)
                        Indices.Append({0,static_cast<SlateIndex>(Index-1),static_cast<SlateIndex>(Index)});
                }
                FSlateDrawElement::MakeCustomVerts(E,L+3,Resource,Vertices,Indices,nullptr,0,0);
            };
            const double Split = FMath::Asin(Horizon/Radius);
            FillArc(PI-Split,2*PI+Split,FColor(49,111,170));
            FillArc(Split,PI-Split,FColor(128,83,52));
            for (int32 Pitch=-20; Pitch<=20; Pitch+=5)
            {
                const double Y=Horizon-Pitch*1.35;
                if (FMath::Abs(Y)>=Radius) continue;
                const double Edge=FMath::Sqrt(Radius*Radius-Y*Y);
                const double Half=Pitch==0 ? Edge : FMath::Min(Edge,Pitch%10==0 ? 12.0 : 6.0);
                DialLine(DialPoint(-Half,Y),DialPoint(Half,Y),White);
            }
            for (int32 Degrees=-60; Degrees<=60; Degrees+=15)
            {
                const double Angle=FMath::DegreesToRadians(static_cast<double>(Degrees));
                const FVector2D Direction(FMath::Sin(Angle),-FMath::Cos(Angle));
                DialLine(DialCenter+Direction*Scale*38,DialCenter+Direction*Scale*42,White);
            }
            const FLinearColor AircraftMark(1.0f,.78f,.25f);
            for (double Side : {-1.0,1.0})
            {
                const FVector2D Inner=DialCenter+FVector2D(Side*8,0)*Scale;
                DialLine(DialCenter+FVector2D(Side*24,0)*Scale,Inner,AircraftMark);
                DialLine(Inner,Inner+FVector2D(0,4)*Scale,AircraftMark);
            }
            DialLine(DialCenter-FVector2D(2,0)*Scale,DialCenter+FVector2D(2,0)*Scale,AircraftMark);
            DialLine(DialPoint(0,-Radius),DialPoint(0,-Radius+5),AircraftMark);
        }
        const FString Attitude = FString::Printf(TEXT("%03d MARK %d"),
            (FMath::RoundToInt(HeadingDegrees) % 360 + 360) % 360, FMath::RoundToInt(PitchDegrees));
        const auto AttitudeFont = MissionUIStyle::GetInfoLabelFont(FMath::Max(9, int(10 * Scale)));
        const FVector2D AttitudeExtent = Measure->Measure(Attitude, AttitudeFont);
        constexpr double AttitudeReadoutY = 92.0;
        const FVector2D AttitudePosition(P.X + 56 * Scale - AttitudeExtent.X * .5, P.Y + AttitudeReadoutY * Scale);
        FSlateDrawElement::MakeText(E, L + 2,
            G.ToPaintGeometry(FVector2f(AttitudeExtent), FSlateLayoutTransform(FVector2f(AttitudePosition))),
            Attitude, AttitudeFont, ESlateDrawEffect::None, HUDBlue);

        const bool bAutoland = HUDPlayer && HUDPlayer->IsAutoLandEngaged();
        const bool bAutoTrack = HUDPlayer && HUDPlayer->IsAutoTrackEngaged();
        MiniText(366,14,bAutoTrack ? TEXT("AUTO TRACK") : bAutoland ? TEXT("AUTOLAND") :
            bAutoAvailable ? TEXT("AUTO PILOT") : TEXT("FLIGHT CONTROL"),
            10,(bAutoland || bAutoAvailable) ? Green : Muted);
        MiniText(366,43,CompactNavLabel,13,White,138);
        if (Snapshot)
            MiniText(366,62,FString::Printf(TEXT("RESPAWNS %d"),Snapshot->RespawnsRemaining),10,Muted,138);
        if (NavReadouts.Num() > 0)
            MiniText(366,62,NavReadouts[0],10,Muted,138);
        if (const Instruction* Waypoint = HUDPlayer && !HUDPlayer->GetInbound() ? HUDPlayer->GetNextNavPoint() : nullptr)
        {
            if (Waypoint->GetSpeed() > 0)
                MiniText(366,80,FString::Printf(TEXT("SPD %d m/s"), Waypoint->GetSpeed()),10,Muted,138);
        }
        if (HUDPlayer && HUDPlayer->IsAirborne())
            MiniText(120,AttitudeReadoutY,FString::Printf(TEXT("MSL %.0f m"),HUDPlayer->GetAltitudeMSL()),10,Muted);
        if (bShowWeapons && !bCapitalWeapons)
        {
            MiniText(244,14,TEXT("WEAPONS"),10,Muted);
            MiniText(244,43,Primary.ToUpper(),10,White,100);
            MiniText(244,62,Secondary.ToUpper(),10,Muted,100);
        }
        MiniText(524,9,PlayerName.ToUpper(),10,White,124);
        if (PlayerMFDPreview && PlayerMFDPreview->IsReady())
            FSlateDrawElement::MakeBox(E,L+2,
                G.ToPaintGeometry(FVector2f(108*Scale,54*Scale),FSlateLayoutTransform(FVector2f(P+FVector2D(531,23)*Scale))),
                &PlayerMFDPreview->GetBrush(),ESlateDrawEffect::None,ShipHealthColor(PlayerHull));
        MiniText(524,77,PlayerClass,10,White,124);
        MiniText(524,91,HomeCarrierReadout,10,Muted,124);
        if(JumpSeconds>0||ThreatLevel>0){
            const FString Alert=JumpSeconds>0?FString::Printf(TEXT("QUANTUM  %02d"),FMath::CeilToInt(JumpSeconds)):ThreatLevel==2?TEXT("MISSILE INBOUND"):TEXT("TARGET LOCK");
            const FLinearColor AlertColor=JumpSeconds>0?HUDBlue:ThreatLevel==2?FLinearColor(1,.22,.16,1):FLinearColor(1,.78,.25,1);
            const FVector2D AP(Size.X*.5-110*Scale,132*Scale),AD(220*Scale,26*Scale);
            Box(AP,AD,FLinearColor(.03,.012,.01,.75));Box(AP,FVector2D(2*Scale,AD.Y),AlertColor);
            FSlateDrawElement::MakeText(E,L+3,G.ToPaintGeometry(FVector2f(AD),FSlateLayoutTransform(FVector2f(AP+FVector2D(14,5)*Scale))),Alert,FCoreStyle::GetDefaultFontStyle("Bold",FMath::Max(9,int(11*Scale))),ESlateDrawEffect::None,AlertColor);
        }

    }
    // LAUNCH remains visible while staging but is disabled until the
    // fighter is actually spotted and the flight-deck slot becomes READY.
    if (!NetworkController.IsValid())
    {
        Hangar* PlayerHangar = nullptr;
        int Squadron = -1, Slot = -1;
        Ship* StagedPlayer = PlayerShip();
        if (FindPlayerAlertSlot(StagedPlayer, PlayerHangar, Squadron, Slot))
        {
            const bool bReady = IsPlayerAtLaunchPosition(StagedPlayer, PlayerHangar, Squadron, Slot);
            const FLinearColor FrameTint = bReady ? FLinearColor::White : FLinearColor(0.33f, 0.33f, 0.33f, 0.8f);
            const FSlateRect R = CarrierLaunchButtonRect(Size, Scale);
            const FVector2D TopLeft(R.Left, R.Top);
            const FVector2D Dimensions(R.Right - R.Left, R.Bottom - R.Top);
            // Render exactly like the existing GEAR DOWN popup, rather than
            // stretching TAC_button artwork to a non-native aspect ratio.
            const FLinearColor Border = bReady ? HUDBlue : FLinearColor(0.23f, 0.29f, 0.35f, 1.f);
            Box(TopLeft, Dimensions, FLinearColor(0, 0.015f, 0.03f, 0.75f));
            Box(TopLeft, FVector2D(Dimensions.X, Scale), Border);
            Box(TopLeft + FVector2D(0, Dimensions.Y - Scale), FVector2D(Dimensions.X, Scale), Border);
            Box(TopLeft, FVector2D(Scale, Dimensions.Y), Border);
            Box(TopLeft + FVector2D(Dimensions.X - Scale, 0), FVector2D(Scale, Dimensions.Y), Border);
            const FSlateFontInfo LaunchFont = FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(8, int32(11 * Scale)));
            const FVector2D LabelSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT("LAUNCH"), LaunchFont);
            FSlateDrawElement::MakeText(E, L + 3,
                G.ToPaintGeometry(FVector2f(LabelSize),
                    FSlateLayoutTransform(FVector2f(TopLeft + (Dimensions - LabelSize) * 0.5f))),
                TEXT("LAUNCH"), LaunchFont, ESlateDrawEffect::None, bReady ? HUDBlue : FLinearColor(0.23f, 0.29f, 0.35f, 1.f));
        }
    }
    // The fighter uses weapon readouts beside the central HUD, not the capital-ship TAC frame.
    for (int32 I=5; bShowCaution && I<7; ++I)
    {
        const FVector2D Native(256,256);
        const FVector2D D = Native * Scale;
        Artwork(I,FVector2D(Size.X*0.5 + (I==5 ? -D.X : 0.0),Size.Y-D.Y),D);
    }
    if (bShowWeapons && !bHasPlayer) Text(FVector2D(Size.X*0.5-250*Scale,Size.Y*0.5+24*Scale),TEXT("WEAPONS: NO PLAYER SHIP"));
    for (int32 Index=bMiniHUD ? 1 : 0; !bPreTakeoff && Index<2; ++Index)
    {
        const EMode M=Modes[Index]; if (M==EMode::Off) continue;
        const FVector2D P(Index==0 ? 0.0 : Size.X-220*Scale, Size.Y-215*Scale);
        const FVector2D D(220*Scale, 215*Scale);
        const FLinearColor Amber(0.55f, 0.9f, 0.35f, 0.9f);
        const FLinearColor Dim(0.2f, 0.45f, 0.15f, 0.7f);
        Box(P, D, FLinearColor(0.01f, 0.03f, 0.01f, 0.72f));
        auto Corner = [&](FVector2D At, float XSign, float YSign)
        {
            const float L = 12.f * Scale;
            Box(At, FVector2D(L, Scale), Amber);
            Box(At, FVector2D(Scale, L), Amber);
            if (XSign < 0) Box(At + FVector2D(-L + Scale, 0), FVector2D(L, Scale), Amber);
            if (YSign < 0) Box(At + FVector2D(0, -L + Scale), FVector2D(Scale, L), Amber);
        };
        Corner(P, 1.f, 1.f);
        Corner(P + FVector2D(D.X - Scale, 0), -1.f, 1.f);
        Corner(P + FVector2D(0, D.Y - Scale), 1.f, -1.f);
        Corner(P + FVector2D(D.X - Scale, D.Y - Scale), -1.f, -1.f);
        const TCHAR* Title=M==EMode::Ship?TEXT("SHIP STATUS"):M==EMode::FOV?TEXT("SNSR  FOV"):M==EMode::HSD?TEXT("SNSR  HSD"):TEXT("SNSR  3D");
        Text(P+FVector2D(10,6)*Scale, Title, Amber, 10);
        Box(P+FVector2D(8,20)*Scale, FVector2D(70*Scale, Scale), Dim);
        if (M != EMode::Ship)
        {
            const int32 ArtIndex = M == EMode::FOV ? 2 : M == EMode::HSD ? 3 : 4;
            Artwork(ArtIndex,P+FVector2D(30,32)*Scale,FVector2D(160,160)*Scale);
        }
        if (!bHasPlayer) { Text(P+FVector2D(8,36)*Scale,TEXT("NO TRACK"), Amber); continue; }
        if (M==EMode::Ship)
        {
            for (int32 I=0;I<StatusRows.Num();++I) Gauge(P+FVector2D(8,28+I*20)*Scale,StatusRows[I].Label,StatusRows[I].Percent);
            if (Ship* PShip = PlayerShip())
                Text(P+FVector2D(8,158)*Scale, FString::Printf(TEXT("EMCON %d"), PShip->GetEMCON()), Amber, 10);
            if (NavReadouts.Num() > 0)
                Text(P+FVector2D(8,174)*Scale, NavReadouts[0], Amber, 10);
            if (bAugmenterActive)
                Text(P+FVector2D(8,192)*Scale, TEXT("AUG ON"), FLinearColor(0.2f,1.0f,0.35f,1.0f));
        }
        else
        {
            const FVector2D Center=P+FVector2D(110,112)*Scale;
            const double Radius=72*Scale;
            auto Mark = [&](const FVector2D& At, const FLinearColor& Color)
            {
                FSlateDrawElement::MakeBox(E, L+6,
                    G.ToPaintGeometry(FVector2f(6.f, 6.f), FSlateLayoutTransform(FVector2f(At - FVector2D(3.f, 3.f)))),
                    FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
            };
            Mark(Center, Amber);
            for (const FBlip& B : Contacts)
            {
                FVector2D Offset;
                if (M == EMode::FOV)
                {
                    Offset = FVector2D(B.Az / (PI / 2), -B.El / (PI / 2)) * Radius;
                    const double UsableRadius = FMath::Max(0.0, Radius - 4.25); // keep 6x6 marker inside circle
                    if (Offset.SizeSquared() > UsableRadius * UsableRadius)
                        Offset = Offset.GetSafeNormal() * UsableRadius;
                }
                else
                {
                    const double R = FMath::Clamp(B.Range / SensorRange, 0.0, 1.0) * Radius;
                    Offset = FVector2D(FMath::Sin(B.Az), -FMath::Cos(B.Az)) * R;
                    if (M == EMode::ThreeD)
                        Offset.Y = Offset.Y * 0.55 - FMath::Sin(B.El) * R * 0.45;
                    if (Offset.SizeSquared() > Radius * Radius)
                        Offset = Offset.GetSafeNormal() * Radius;
                }
                Mark(Center + Offset, B.Color);
            }
            Text(P+FVector2D(8,192)*Scale,FString::Printf(TEXT("%03.0f KM  %02d TRK"),SensorRange/1000,Contacts.Num()), Amber, 9);
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
    if (bHasPlayer && !bMiniHUD && !bPreTakeoff)
    {
        // Reserve an extra HOME row for fighters while keeping the final row at the bottom edge.
        const double HomeRowHeight = HomeCarrierReadout.IsEmpty() ? 0.0 : 20.0;
        const FVector2D P(232*Scale,Size.Y-(172+HomeRowHeight)*Scale);
        ShipText(P,PlayerName);
        if(PlayerMFDPreview && PlayerMFDPreview->IsReady())
            FSlateDrawElement::MakeBox(E,L+1,
                G.ToPaintGeometry(FVector2f(176*Scale,88*Scale),FSlateLayoutTransform(FVector2f(P+FVector2D(0,22)*Scale))),
                &PlayerMFDPreview->GetBrush(),ESlateDrawEffect::None,ShipHealthColor(PlayerHull));
        ShipText(P+FVector2D(0,112)*Scale,PlayerClass);
        if (!HomeCarrierReadout.IsEmpty()) ShipText(P+FVector2D(0,132)*Scale,HomeCarrierReadout);
        ShipText(P+FVector2D(0,132+HomeRowHeight)*Scale,FString(TEXT("HULL  "))+FormatStatusReadout(TEXT("HULL"),PlayerHull),ShipHealthColor(PlayerHull));
        ShipText(P+FVector2D(0,152+HomeRowHeight)*Scale,FString(TEXT("SHIELD  "))+FormatStatusReadout(TEXT("SHIELD"),PlayerShield),ShipHealthColor(PlayerShield));
        if (bShowWeapons && !bCapitalWeapons && !ThirdPerson.Get() && !bPreTakeoff)
        {
        ReticleText(-248,LowerReadoutBaseline,108,Primary.ToUpper());
        ReticleText(-248,LowerReadoutBaseline+LowerReadoutRowHeight,108,Secondary.ToUpper());
        }
    }
    if (bHasPlayer && bShowCaution)
    {
        const int32 Rows=FMath::Min(12,DamageRows.Num());
        // Fit all visible caution labels to one shared size: the smallest required.
        const auto CautionMeasure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        int32 CautionFontSize=FMath::Max(6,int32(11*Scale));
        FSlateFontInfo CautionFont=FCoreStyle::GetDefaultFontStyle("Bold",CautionFontSize);
        for(int32 I=0;bShowCaution && I<Rows;++I)
        {
            if(DamageRows[I].Status<0) continue;
            while(CautionMeasure->Measure(DamageRows[I].Label,CautionFont).X>71*Scale && CautionFontSize>5)
                CautionFont.Size=--CautionFontSize;
        }
        for(int32 I=0;bShowCaution && I<Rows;++I)
        {
            const FVector2D D=FVector2D(Size.X*0.5-150*Scale,Size.Y-97*Scale)+FVector2D((I%4)*75,(I/4)*28)*Scale;
            // Legacy caution cells show a centered abbreviation; color conveys health.
            if (DamageRows[I].Status < 0) continue;
            const FString& Label = DamageRows[I].Label;
            const FVector2D Extent=CautionMeasure->Measure(Label,CautionFont);
            FSlateDrawElement::MakeText(E,L+2,
                G.ToPaintGeometry(FVector2f(75*Scale,20*Scale),
                    FSlateLayoutTransform(FVector2f(D+FVector2D((75*Scale-Extent.X)*0.5,0)))),
                Label,CautionFont,ESlateDrawEffect::None,
                SSWDamagePanel::Color(DamageRows[I].Status,FPlatformTime::Seconds(),HUDBlue));

        }
    }
    if (bHasTarget)
    {
        // Target has one additional range row. Keep its final row bottom-aligned too.
        const FVector2D P(Size.X-408*Scale,Size.Y-192*Scale);
        FString WaypointFirst, WaypointSecond;
        const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(8, int32(11*Scale)));
        const auto NameMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        const bool WrapWaypoint = TargetClass.StartsWith(TEXT("Waypoint")) &&
            NameMeasure->Measure(TargetName,NameFont).X > 176*Scale &&
            TargetName.Split(TEXT(" - "), &WaypointFirst, &WaypointSecond);
        if (WrapWaypoint) {
            ShipText(P-FVector2D(0,16)*Scale,WaypointFirst.TrimStartAndEnd(),TargetDisplayColor);
            ShipText(P,WaypointSecond.TrimStartAndEnd(),TargetDisplayColor);
        }
        else ShipText(P,TargetName,TargetDisplayColor);
        if(TargetMFDPreview && TargetMFDPreview->IsReady())
            FSlateDrawElement::MakeBox(E,L+1,
                G.ToPaintGeometry(FVector2f(176*Scale,88*Scale),FSlateLayoutTransform(FVector2f(P+FVector2D(0,22)*Scale))),
                &TargetMFDPreview->GetBrush(),ESlateDrawEffect::None,TargetHull < 0 ? FLinearColor::White : ShipHealthColor(TargetHull));
        else if(TargetHull<0)
            Icon(P+FVector2D(48,24)*Scale,TargetBrush,TargetTexture.IsValid(),TargetHull);
        ShipText(P+FVector2D(0,112)*Scale,
            bTargetTracking ? TargetClass + TEXT("   TRACKING") :
            bTargetLocked ? TargetClass + TEXT("   LOCKED") : TargetClass, TargetDisplayColor);
        if (TargetHull >= 0) ShipText(P+FVector2D(0,132)*Scale,FString(TEXT("HULL  "))+FormatStatusReadout(TEXT("HULL"),TargetHull),TargetDisplayColor);
        if (TargetHull >= 0) ShipText(P+FVector2D(0,152)*Scale,FString(TEXT("SHIELD  "))+FormatStatusReadout(TEXT("SHIELD"),TargetShield),TargetDisplayColor);
        ShipText(P+FVector2D(0,172)*Scale,TargetRange>=0?UFormattingUtils::FormatDistance(TargetRange * 1000.0):TEXT("RANGE --"),TargetDisplayColor);
    }
    } // External instruments; radio and information remain available in cockpit.
    if (const auto* Controller = NetworkController.Get())
    {
        const auto* Snapshot = Controller->GetAssignedShipState();
        if (Snapshot && Symbology && !bPreTakeoff)
        {
            Symbology->Paint(G,E,L+3,ThirdPerson.Get(),nullptr,bShowWeapons);
            if (!Snapshot->bAirborne)
                Symbology->PaintCompass(G,E,L+3,HeadingDegrees,OrderedHeading,PitchDegrees,OrderedPitch,
                    bHelmMode,ThirdPerson.Get(),FMath::RadiansToDegrees(BankRadians));
            if (bShoot && bShowWeapons) Popup(152,TEXT("SHOOT"));
        }
        return L + 6;
    }
    if (bCapitalWeapons && bShowWeapons && !bShowCaution &&
        (!bShowInformation || InformationCaptions.IsEmpty()))
    {
        const int32 MissileCount = FMath::Min(MissileReadouts.Num(), 6);
        const FVector2D D = FVector2D(326, 69 + 18 * ((MissileCount + 1) / 2)) * Scale;
        const FVector2D P((Size.X - D.X) * 0.5, Size.Y - D.Y - 8 * Scale);
        const FSlateBrush& Glass = FlightPanelStyle::Glass();
        FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(D),FSlateLayoutTransform(FVector2f(P))),
            &Glass,ESlateDrawEffect::None,Glass.TintColor.GetSpecifiedColor());
        Text(P+FVector2D(12,8)*Scale,TEXT("WEAPONS"),HUDBlue);
        Text(P+FVector2D(12,29)*Scale,Primary);
        Text(P+FVector2D(168,29)*Scale,Secondary);
        Text(P+FVector2D(12,47)*Scale,DefenseReadout);
        for (int32 I=0; I<MissileCount; ++I)
            Text(P+FVector2D(12+156*(I%2),65+18*(I/2))*Scale,MissileReadouts[I]);
    }
    if (bShowInformation && !InformationCaptions.IsEmpty())
    {
        const FVector2D D = FVector2D(326,124) * Scale;
        const FVector2D P((Size.X - D.X) * 0.5, Size.Y - D.Y);
        const FSlateBrush& Glass = FlightPanelStyle::Glass();
        FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(D),FSlateLayoutTransform(FVector2f(P))),
            &Glass,ESlateDrawEffect::None,Glass.TintColor.GetSpecifiedColor());
        Text(P+FVector2D(12,8)*Scale,TEXT("INFORMATION"),HUDBlue);
        Box(P+FVector2D(12,24)*Scale,FVector2D(302,1)*Scale,FlightPanelStyle::DividerColor());
    }
    if (Game::IsTimeCompressed() && !IsRadioOpen())
        Text(FVector2D(Size.X-248*Scale,34*Scale),
            FString::Printf(TEXT("TRAVEL %uX    %s 1X"), Game::GetSimulationRate(),
                *StarshatterInputBindings::ResolvedKey(TEXT("SSW_TravelSpeedReset"), EKeys::F12).GetDisplayName().ToString()));

    if (IsRadioOpen())
    {
        TArray<FString> Labels; TArray<int32> Commands; TArray<bool> Enabled; FString Title;
        BuildRadioRows(Labels,Commands,Enabled,Title);
        const FVector2D P(Size.X-260*Scale,24*Scale);
        const FVector2D D = FVector2D(244, 64 + Labels.Num() * 24) * Scale;
        const auto& Glass = FlightPanelStyle::Glass();
        FSlateDrawElement::MakeBox(E, L+1,
            G.ToPaintGeometry(FVector2f(D), FSlateLayoutTransform(FVector2f(P))),
            &Glass, ESlateDrawEffect::None, Glass.TintColor.GetSpecifiedColor());
        Text(P+FVector2D(12,10)*Scale,Title);
        for (int32 I=0; I<Labels.Num(); ++I)
            Text(P+FVector2D(12,36+24*I)*Scale,FString::Printf(TEXT("%d. %s"),I+1,*Labels[I]),Enabled[I]?HUDBlue:FLinearColor(0.12f,0.2f,0.25f,1));
        Text(P+FVector2D(12,40+24*Labels.Num())*Scale,
            IsTravelSpeedOpen() ? FString::Printf(TEXT("0 CLOSE    %s 1X"),
                *StarshatterInputBindings::ResolvedKey(TEXT("SSW_TravelSpeedReset"), EKeys::F12).GetDisplayName().ToString())
                : FString(TEXT("0 BACK    R CLOSE")));
    }
    DrawILS(G, E, L);
    if(bHasPlayer && Symbology){
        Symbology->Paint(G,E,L+3,ThirdPerson.Get(),SelectedTarget.Get().Get(),bShowWeapons && !bPreTakeoff);
        if (!bPreTakeoff && (ThirdPerson.Get() || bHelmMode) &&
            !(bMiniHUD && HUDPlayer && HUDPlayer->IsAirborne()))
            Symbology->PaintCompass(G,E,L+3,HeadingDegrees,OrderedHeading,PitchDegrees,OrderedPitch,bHelmMode,ThirdPerson.Get(),FMath::RadiansToDegrees(BankRadians));
    }
    return SCompoundWidget::OnPaint(Args,G,Cull,E,L+7,Style,Enabled);
}

namespace
{
    TArray<SimElement*> RadioElements(Ship* Player)
    {
        TArray<SimElement*> Result;
        Sim* LiveSim = Sim::GetSim();
        if (!Player || !LiveSim) return Result;
        ListIter<SimElement> Elements = LiveSim->GetElements();
        while (++Elements)
        {
            SimElement* Element = Elements.value();
            if (Element && !Element->IsFinished() && !Element->IsSquadron() && !Element->IsStatic()
                && Element != Player->GetElement() && Element->GetIFF() == Player->GetIFF()) Result.Add(Element);
        }
        return Result;
    }
}

void SFighterHUDDetails::ToggleRadio()
{
    if (IsRadioOpen()) { CloseRadio(); return; }
    Ship* P = PlayerShip();
    if (P && (P->IsDropship() || P->IsStarship())) { CloseRadio(); RadioPage = 0; }
}
void SFighterHUDDetails::BuildRadioRows(TArray<FString>& Labels, TArray<int32>& Commands, TArray<bool>& Enabled, FString& Title) const
{
    Ship* P = PlayerShip();
    if (IsTravelSpeedOpen())
    {
        const bool bThreats = USSWRuntimeSubsystem::HasTravelThreats(P);
        Title = bThreats ? TEXT("THREATS DETECTED") : FString::Printf(TEXT("TRAVEL SPEED   %uX"), Game::GetSimulationRate());
        Labels = {TEXT("1X"), TEXT("2X"), TEXT("5X"), TEXT("10X"), TEXT("20X")};
        Enabled.Init(!bThreats, Labels.Num());
        Enabled[0] = true;
        return;
    }
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
    if (RadioPage == 0 && P->IsStarship())
    {
        const auto Elements = RadioElements(P);
        const int32 Pages = FMath::Max(1, (Elements.Num() + 7) / 8);
        const int32 PageIndex = FMath::Clamp(RadioRecipientPage, 0, Pages-1);
        Title = FString::Printf(TEXT("RADIO %d/%d"), PageIndex+1, Pages);
        for (int32 Slot = 0; Slot < 8; ++Slot)
        {
            const int32 Offset = PageIndex*8 + Slot;
            if (!Elements.IsValidIndex(Offset)) break;
            SimElement* Recipient = Elements[Offset];
            const bool Active = Recipient->IsActive();
            Labels.Add(FString(ANSI_TO_TCHAR(Recipient->GetName().data())) + (Active ? TEXT("") : TEXT(" (UNAVAILABLE)")));
            Commands.Add(Recipient->GetIdentity()); Enabled.Add(CanTransmit && Active);
        }
        if (Pages > 1)
        {
            while (Labels.Num() < 8) { Labels.Add(TEXT("--")); Commands.Add(0); Enabled.Add(false); }
            Labels.Add(TEXT("NEXT PAGE")); Commands.Add(-99); Enabled.Add(true);
        }
    }
    else if (RadioPage == 0)
    {
        Page(TEXT("Wingman"),1,Wing && !Wing->IsDead());
        Page(TEXT("Element"),2,Element != nullptr);
        Page(TEXT("Control / Landing"),3,P->GetController()!=nullptr || P->GetInbound()!=nullptr);
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
        Add(TEXT("Request backup"),RadioMessageAction::REQUEST_SUPPORT);
        Add(TEXT("Call inbound"),RadioMessageAction::CALL_INBOUND);
        Add(TEXT("Request landing clearance"),RadioMessageAction::REQUEST_LANDING_CLEARANCE,P->GetInbound()!=nullptr);
        Add(TEXT("Call finals"),RadioMessageAction::CALL_FINALS,P->GetInbound() && P->GetInbound()->Cleared());
        Labels.Add(P->IsAutoLandEngaged() ? TEXT("Disengage autoland") : TEXT("Engage autoland"));
        Commands.Add(10001);
        Enabled.Add(P->IsAutoLandEngaged() ? P->GetFlightPhase() < EOPSMode::DOCKING : P->CanRequestAutoLand());
        Labels.Add(bILSMode ? TEXT("Hide ILS hoops") : TEXT("Show ILS hoops"));
        Commands.Add(10002);
        Enabled.Add(P->IsDropship());
    }
}
void SFighterHUDDetails::RadioSelect(int32 Number)
{
    if (!IsRadioOpen()) return;
    Ship* P=PlayerShip();
    if (!P) { CloseRadio(); return; }
    if (Number==0)
    {
        if (RadioHistory.IsEmpty()) CloseRadio();
        else RadioPage = RadioHistory.Pop();
        return;
    }
    TArray<FString> Labels; TArray<int32> Commands; TArray<bool> Enabled; FString Title;
    BuildRadioRows(Labels,Commands,Enabled,Title);
    const int32 I=Number-1;
    if (!Commands.IsValidIndex(I) || !Enabled[I]) return;
    if (RadioPage==0 && P->IsStarship())
    {
        if (Commands[I] == -99)
        {
            const int32 Pages = FMath::Max(1, (RadioElements(P).Num()+7)/8);
            RadioRecipientPage = (RadioRecipientPage+1)%Pages;
            return;
        }
        RadioElementId = Commands[I]; RadioRecipient = 4;
        RadioHistory.Add(RadioPage); RadioPage = 1; return;
    }
    if (RadioPage==0) { RadioHistory.Add(RadioPage); RadioRecipient=Number; RadioPage=Number==3 ? 7 : 1; return; }
    if (RadioPage==1) { RadioHistory.Add(RadioPage); RadioPage=-Commands[I]; return; }
    if (Commands[I]==10002)
    {
        bILSMode = !bILSMode;
        Invalidate(EInvalidateWidgetReason::Paint);
        CloseRadio();
        return;
    }
    if (Commands[I]==10001)
    {
        P->SetAutoLand(!P->IsAutoLandEngaged());
        CloseRadio();
        return;
    }
    const RadioMessageAction Action=static_cast<RadioMessageAction>(Commands[I]);
    SimElement* Element=P->GetElement();
    RadioMessage* Message=nullptr;
    if (RadioRecipient==4)
    {
        for (SimElement* Recipient : RadioElements(P))
            if (Recipient->GetIdentity()==RadioElementId && Recipient->IsActive())
            { Message = new RadioMessage(Recipient,P,Action); break; }
    }
    else if (RadioRecipient==2 && Element) Message=new RadioMessage(Element,P,Action);
    else
    {
        Ship* Destination=nullptr;
        if (RadioRecipient==3) {
            InboundSlot* Arrival = P->GetInbound();
            const bool RecoveryCall = Action==RadioMessageAction::CALL_INBOUND ||
                Action==RadioMessageAction::REQUEST_LANDING_CLEARANCE || Action==RadioMessageAction::CALL_FINALS;
            Destination = RecoveryCall && Arrival && Arrival->GetDeck()
                ? const_cast<Ship*>(Arrival->GetDeck()->GetCarrier()) : P->GetController();
        }
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

void SFighterHUDDetails::UpdateILSMode(int32 PlayerId, bool bInbound)
{
    if (PlayerId != ILSPlayerId)
    {
        ILSPlayerId = PlayerId;
        bILSMode = false;
        bILSInbound = false;
    }
    // Enable once on assignment; a manual Hide must survive subsequent HUD ticks.
    if (bInbound && !bILSInbound) bILSMode = true;
    bILSInbound = bInbound;
}

void SFighterHUDDetails::UpdateInformationMessages()
{
    TArray<FRadioHUDLine> Lines;RadioView::GetHUDMessages(Lines);
    bool Changed=Lines.Num()!=InformationCaptions.Num();
    for(int32 I=0;!Changed && I<Lines.Num();++I)
        Changed=Lines[I].Caption!=InformationCaptions[I] || Lines[I].ExpiresAt!=InformationExpiry[I];
    if(!Changed || !InformationRows) return;
    InformationRows->ClearChildren();InformationCaptions.Reset();InformationExpiry.Reset();
    for(const auto& Line:Lines)
    {
        InformationCaptions.Add(Line.Caption);InformationExpiry.Add(Line.ExpiresAt);
        InformationRows->AddSlot().AutoHeight().Padding(0,0,0,3)
        [ SNew(STextBlock).Text(FText::FromString(Line.Caption)).AutoWrapText(true)
            .Font(MissionUIStyle::GetInfoLabelFont(12))
            .ColorAndOpacity_Lambda([Expiry=Line.ExpiresAt,Source=Line.Source]{FLinearColor Color=Source==1?FLinearColor(0.35f,1.f,0.55f):Source==2?FLinearColor(1.f,0.78f,0.3f):HUDBlue;Color.A=FMath::Clamp(Expiry-FPlatformTime::Seconds(),0.0,1.0);return FSlateColor(Color);})
        ];
    }
    if(InformationScroll)InformationScroll->ScrollToEnd();
}
