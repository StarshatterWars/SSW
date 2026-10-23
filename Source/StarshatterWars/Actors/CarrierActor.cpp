#include "CarrierActor.h"

#include "Components/SceneComponent.h"
#include "Ship.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "ShipDesign.h"
#include "CarrierFlightDeckComponent.h"
#include "CarrierHangarComponent.h"

ACarrierActor::ACarrierActor()
{
    CarrierDeckRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CarrierDeckRoot"));
    CarrierDeckRoot->SetupAttachment(PointRoot);
    CarrierHangarRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CarrierHangarRoot"));
    CarrierHangarRoot->SetupAttachment(PointRoot);
    CarrierLaunchRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CarrierLaunchRoot"));
    CarrierLaunchRoot->SetupAttachment(CarrierDeckRoot);
    CarrierRecoveryRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CarrierRecoveryRoot"));
    CarrierRecoveryRoot->SetupAttachment(CarrierDeckRoot);
    CarrierCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CarrierCameraRoot"));
    CarrierCameraRoot->SetupAttachment(PointRoot);
    NativePrimaryHangar = CreateDefaultSubobject<UCarrierHangarComponent>(TEXT("Hangar_Component"));
    NativePrimaryHangar->SetupAttachment(CarrierHangarRoot);
}

UCarrierFlightDeckComponent* ACarrierActor::AddNativeFlightDeck(int32 DeckIndex, bool bRecovery)
{
    const FName Name(*FString::Printf(TEXT("FlightDeck_%d_Component"), DeckIndex + 1));
    UCarrierFlightDeckComponent* Deck = CreateDefaultSubobject<UCarrierFlightDeckComponent>(Name);
    Deck->SetupAttachment(bRecovery ? CarrierRecoveryRoot.Get() : CarrierLaunchRoot.Get());
    NativeFlightDecks.Add(Deck);
    Deck->BindFlightDeck(nullptr, DeckIndex);
    return Deck;
}

UCarrierHangarComponent* ACarrierActor::AddNativeHangarBay(int32 DeckIndex)
{
    const FName Name(*FString::Printf(TEXT("HangarBay_%d_Component"), DeckIndex + 1));
    UCarrierHangarComponent* Bay = CreateDefaultSubobject<UCarrierHangarComponent>(Name);
    Bay->SetupAttachment(NativePrimaryHangar.Get());
    NativeHangarBays.Add(Bay);
    Bay->BindHangar(nullptr, DeckIndex);
    return Bay;
}

void ACarrierActor::SynchronizeNativeDeckLocations()
{
    for (int32 Index = 0; Index < NativeFlightDecks.Num(); ++Index)
    {
        const int32 DeckNo = Index + 1;
        FVector AuthoredLocation = FVector::ZeroVector;
        bool bFound = false;
        for (const FShipPointDef& Def : DockPointDefs)
        {
            if (Def.PointName.ToString().StartsWith(FString::Printf(TEXT("Deck %d_"), DeckNo)))
            {
                AuthoredLocation = Def.LocalOffset;
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            for (const FShipPointDef& Def : LandingPointDefs)
                if (Def.PointName.ToString().StartsWith(FString::Printf(TEXT("Deck %d_"), DeckNo)))
                {
                    AuthoredLocation = Def.LocalOffset;
                    break;
                }
        }
        if (UCarrierFlightDeckComponent* Deck = NativeFlightDecks[Index].Get())
            Deck->SetRelativeLocation(AuthoredLocation);
        if (NativeHangarBays.IsValidIndex(Index) && NativeHangarBays[Index])
            NativeHangarBays[Index]->SetRelativeLocation(AuthoredLocation);
    }
}

void ACarrierActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildCarrierPresentation();
}

void ACarrierActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshRuntimeComponentBindings();
}

void ACarrierActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RefreshRuntimeComponentBindings();
}

Ship* ACarrierActor::GetCarrierSimulation() const
{
    return GetRuntimeShip();
}

Hangar* ACarrierActor::GetCarrierHangar() const
{
    Ship* SimShip = GetCarrierSimulation();
    return SimShip ? SimShip->GetHangar() : nullptr;
}

FlightDeck* ACarrierActor::GetCarrierFlightDeck(int32 Index) const
{
    Ship* SimShip = GetCarrierSimulation();
    return SimShip && Index >= 0 && Index < SimShip->NumFlightDecks()
        ? SimShip->GetFlightDeck(Index) : nullptr;
}

FVector ACarrierActor::ConvertDeckWorldToUE(const FVector& LegacyWorld) const
{
    const Ship* Carrier = GetCarrierSimulation();
    if (!Carrier) return LegacyWorld;

    // Legacy simulation basis is right/up/forward. UE actor local axes are
    // forward/right/up; actor transform supplies actual BP/model scale ONCE.
    const FVector Delta = LegacyWorld - Carrier->GetLocation();
    const FVector Local(
        FVector::DotProduct(Delta, Carrier->GetCam().vpn().GetSafeNormal()),
        FVector::DotProduct(Delta, Carrier->GetCam().vrt().GetSafeNormal()),
        FVector::DotProduct(Delta, Carrier->GetCam().vup().GetSafeNormal()));
    return GetActorTransform().TransformPosition(Local);
}

FVector ACarrierActor::ConvertDeckFighterToUE(Ship* Fighter)
{
    Ship* Carrier = GetCarrierSimulation();
    if (!Carrier || !Fighter || Fighter->GetCarrier() != Carrier)
        return Fighter ? ConvertDeckWorldToUE(Fighter->GetLocation()) : GetActorLocation();

    const EOPSMode Phase = Fighter->GetFlightPhase();
    if (Phase < EOPSMode::ALERT || Phase >= EOPSMode::ACTIVE)
    {
        CapturedLaunchLanes.Remove(Fighter);
        return ConvertDeckWorldToUE(Fighter->GetLocation());
    }

    const FVector Right = Carrier->GetCam().vrt().GetSafeNormal();
    const FVector Relative = Fighter->GetLocation() - Carrier->GetLocation();
    const double ActualLane = FVector::DotProduct(Relative, Right);
    double* SavedLane = CapturedLaunchLanes.Find(Fighter);
    if (!SavedLane || Phase == EOPSMode::ALERT)
    {
        CapturedLaunchLanes.Add(Fighter, ActualLane);
        SavedLane = CapturedLaunchLanes.Find(Fighter);
    }
    const FVector LanePosition = Fighter->GetLocation() + Right * (*SavedLane - ActualLane);
    return ConvertDeckWorldToUE(LanePosition);
}

FVector ACarrierActor::ConvertDeckWorldToLegacyCamera(const FVector& LegacyWorld) const
{
    const Ship* Carrier = GetCarrierSimulation();
    if (!Carrier) return LegacyWorld;

    // The legacy camera renderer consumes simulation axes, whereas the hull
    // and fighter components use UE axes. Preserve the carrier's sim origin.
    const FVector UEOffset = ConvertDeckWorldToUE(LegacyWorld) - GetActorLocation();
    return Carrier->GetLocation() + FVector(UEOffset.Y, UEOffset.Z, UEOffset.X);
}

FVector ACarrierActor::ConvertDeckFighterToLegacyCamera(Ship* Fighter)
{
    Ship* Carrier = GetCarrierSimulation();
    if (!Carrier || !Fighter) return Fighter ? Fighter->GetLocation() : FVector::ZeroVector;
    const FVector UEOffset = ConvertDeckFighterToUE(Fighter) - GetActorLocation();
    return Carrier->GetLocation() + FVector(UEOffset.Y, UEOffset.Z, UEOffset.X);
}

int32 ACarrierActor::GetRuntimeFlightDeckCount() const
{
    Ship* SimShip = GetCarrierSimulation();
    return SimShip ? SimShip->NumFlightDecks() : 0;
}

bool ACarrierActor::HasRuntimeHangar() const
{
    return GetCarrierHangar() != nullptr;
}

bool ACarrierActor::IsRuntimeLaunchDeck(int32 Index) const
{
    const FlightDeck* Deck = GetCarrierFlightDeck(Index);
    return Deck && Deck->IsLaunchDeck();
}

bool ACarrierActor::IsRuntimeRecoveryDeck(int32 Index) const
{
    const FlightDeck* Deck = GetCarrierFlightDeck(Index);
    return Deck && Deck->IsRecoveryDeck();
}

USceneComponent* ACarrierActor::CreateCarrierMarker(USceneComponent* Parent, const FString& Name,
    const FVector& LocalPosition, const FRotator& LocalRotation)
{
    if (!Parent || !GetWorld()) return nullptr;
    const FName UniqueName = MakeUniqueObjectName(this, USceneComponent::StaticClass(), FName(*Name));
    USceneComponent* Marker = NewObject<USceneComponent>(this, UniqueName, RF_Transient);
    if (!Marker) return nullptr;
    Marker->SetupAttachment(Parent);
    Marker->SetRelativeLocation(LocalPosition);
    Marker->SetRelativeRotation(LocalRotation);
    Marker->RegisterComponent();
    return Marker;
}

void ACarrierActor::ClearCarrierMarkers()
{
    const auto DestroyAll = [](TArray<TObjectPtr<USceneComponent>>& Items)
    {
        for (TObjectPtr<USceneComponent>& Item : Items)
            if (IsValid(Item.Get())) Item->DestroyComponent();
        Items.Empty();
    };
    // Native flight deck components must survive editor reconstruction.
    DeckMarkers.Empty();
    DestroyAll(LaunchSpotMarkers);
    DestroyAll(RecoveryMarkers);
    DestroyAll(BayCameraMarkers);
    for (UCarrierFlightDeckComponent* Deck : FlightDeckComponents)
        if (IsValid(Deck) && !NativeFlightDecks.Contains(TObjectPtr<UCarrierFlightDeckComponent>(Deck))) Deck->DestroyComponent();
    for (UCarrierHangarComponent* Bay : HangarBayComponents)
        if (IsValid(Bay) && !NativeHangarBays.Contains(TObjectPtr<UCarrierHangarComponent>(Bay))) Bay->DestroyComponent();
    if (IsValid(HangarComponent.Get()) && HangarComponent != NativePrimaryHangar) HangarComponent->DestroyComponent();
    FlightDeckComponents.Empty();
    HangarBayComponents.Empty();
    HangarComponent = nullptr;
    LaunchMarkerDeckIndices.Empty();
    LaunchMarkerSpotIndices.Empty();
    RecoveryMarkerDeckIndices.Empty();
    LastBoundCarrier = nullptr;
    LastBoundDeckCount = INDEX_NONE;
}

void ACarrierActor::RefreshRuntimeComponentBindings()
{
    const Ship* Carrier = GetCarrierSimulation();
    const int32 RuntimeCount = Carrier ? Carrier->NumFlightDecks() : 0;
    if (Carrier != LastBoundCarrier || (Carrier && RuntimeCount != LastBoundDeckCount))
    {
        // Late spawner binding must generate the actual deck count, including
        // decks without named visual markers. Rebuild is NOT a simulation write.
        RebuildCarrierPresentation();
        return;
    }
    // Hangar may be created later than the Ship. Refresh non-owning bindings.
    Hangar* ExistingHangar = GetCarrierHangar();
    if (HangarComponent) HangarComponent->BindHangar(ExistingHangar, INDEX_NONE);
    for (int32 i = 0; i < HangarBayComponents.Num(); ++i)
        if (HangarBayComponents[i]) HangarBayComponents[i]->BindHangar(ExistingHangar, i);
    for (int32 i = 0; i < FlightDeckComponents.Num(); ++i)
        if (FlightDeckComponents[i]) FlightDeckComponents[i]->BindFlightDeck(GetCarrierFlightDeck(i), i);
}

void ACarrierActor::RebuildCarrierPresentation()
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || !GetWorld()) return;
    ClearCarrierMarkers();
    SynchronizeNativeDeckLocations();

    // The model's points are already in UE local axes and the component tree
    // applies actual BP/actor scale. Never multiply values or create offsets.
    // Build distinct components from the union of authored and runtime decks.
    TMap<int32, FVector> DeckAnchors;
    TMap<int32, bool> AuthoredRecovery;
    for (const FShipPointDef& Def : DockPointDefs)
    {
        const FString PointName = Def.PointName.ToString();
        if (!PointName.StartsWith(TEXT("Deck "))) continue;
        const int32 DeckNo = FCString::Atoi(*PointName.RightChop(5));
        if (DeckNo > 0 && !DeckAnchors.Contains(DeckNo))
            DeckAnchors.Add(DeckNo, Def.LocalOffset);
    }
    for (const FShipPointDef& Def : LandingPointDefs)
    {
        const FString PointName = Def.PointName.ToString();
        if (!PointName.StartsWith(TEXT("Deck "))) continue;
        const int32 DeckNo = FCString::Atoi(*PointName.RightChop(5));
        if (DeckNo > 0)
        {
            AuthoredRecovery.Add(DeckNo, true);
            if (!DeckAnchors.Contains(DeckNo)) DeckAnchors.Add(DeckNo, Def.LocalOffset);
        }
    }

    int32 HighestAuthoredDeck = 0;
    for (const TPair<int32, FVector>& Pair : DeckAnchors)
        HighestAuthoredDeck = FMath::Max(HighestAuthoredDeck, Pair.Key);
    const int32 TotalDecks = FMath::Max(HighestAuthoredDeck, GetRuntimeFlightDeckCount());

    // A carrier has one simulation Hangar. The bay components below represent
    // distinct physical deck spaces but all bind to that same Hangar instance.
    HangarComponent = NativePrimaryHangar.Get();
    if (HangarComponent)
        HangarComponent->BindHangar(GetCarrierHangar(), INDEX_NONE);

    for (int32 DeckIndex = 0; DeckIndex < TotalDecks; ++DeckIndex)
    {
        const int32 DeckNo = DeckIndex + 1;
        FlightDeck* RuntimeDeck = GetCarrierFlightDeck(DeckIndex);
        const bool bRecovery = RuntimeDeck ? RuntimeDeck->IsRecoveryDeck()
            : AuthoredRecovery.Contains(DeckNo);
        const FVector* DeckPosition = DeckAnchors.Find(DeckNo);
        UCarrierFlightDeckComponent* DeckComp = NativeFlightDecks.IsValidIndex(DeckIndex)
            ? NativeFlightDecks[DeckIndex].Get() : nullptr;
        if (DeckComp)
            DeckComp->BindFlightDeck(RuntimeDeck, DeckIndex);

        UCarrierHangarComponent* Bay = NativeHangarBays.IsValidIndex(DeckIndex)
            ? NativeHangarBays[DeckIndex].Get() : nullptr;
        if (Bay)
            Bay->BindHangar(GetCarrierHangar(), DeckIndex);
        HangarBayComponents.Add(Bay);
        FlightDeckComponents.Add(DeckComp);

        if (DeckComp) DeckMarkers.Add(DeckComp);

        // Camera reference is an authored anchor, not a fabricated camera pose.
        if (DeckComp)
        {
            if (USceneComponent* Ref = CreateCarrierMarker(DeckComp,
                FString::Printf(TEXT("Deck_%d_CameraReference"), DeckNo),
                FVector::ZeroVector, FRotator::ZeroRotator))
                BayCameraMarkers.Add(Ref);
        }
    }

    // Authored dock/landing locations become children of their own decks, so
    // moving a deck presentation component moves its entire visual hierarchy.
    for (const FShipPointDef& Def : DockPointDefs)
    {
        const FString PointName = Def.PointName.ToString();
        if (!PointName.StartsWith(TEXT("Deck "))) continue;
        const FString AfterDeck = PointName.RightChop(5);
        FString DeckText, Tail;
        if (!AfterDeck.Split(TEXT("_"), &DeckText, &Tail) || !Tail.StartsWith(TEXT("Spot_"))) continue;
        const int32 DeckNo = FCString::Atoi(*DeckText);
        const int32 SpotNo = FCString::Atoi(*Tail.RightChop(5));
        if (DeckNo <= 0 || SpotNo <= 0 || !FlightDeckComponents.IsValidIndex(DeckNo - 1)) continue;
        UCarrierFlightDeckComponent* Parent = FlightDeckComponents[DeckNo - 1];
        if (!Parent) continue;
        if (USceneComponent* Spot = CreateCarrierMarker(Parent,
            FString::Printf(TEXT("Deck_%d_Spot_%d"), DeckNo, SpotNo),
            Def.LocalOffset - Parent->GetRelativeLocation(), Def.LocalRotation))
        {
            LaunchSpotMarkers.Add(Spot);
            LaunchMarkerDeckIndices.Add(DeckNo - 1);
            LaunchMarkerSpotIndices.Add(SpotNo - 1);
        }
    }
    for (const FShipPointDef& Def : LandingPointDefs)
    {
        const FString PointName = Def.PointName.ToString();
        if (!PointName.StartsWith(TEXT("Deck "))) continue;
        const int32 DeckNo = FCString::Atoi(*PointName.RightChop(5));
        if (DeckNo <= 0 || !FlightDeckComponents.IsValidIndex(DeckNo - 1)) continue;
        UCarrierFlightDeckComponent* Parent = FlightDeckComponents[DeckNo - 1];
        if (!Parent) continue;
        if (USceneComponent* Point = CreateCarrierMarker(Parent,
            FString::Printf(TEXT("Deck_%d_Recovery"), DeckNo),
            Def.LocalOffset - Parent->GetRelativeLocation(), Def.LocalRotation))
        {
            RecoveryMarkers.Add(Point);
            RecoveryMarkerDeckIndices.Add(DeckNo - 1);
        }
    }

    LastBoundCarrier = GetCarrierSimulation();
    LastBoundDeckCount = LastBoundCarrier ? LastBoundCarrier->NumFlightDecks() : 0;
}

UCarrierFlightDeckComponent* ACarrierActor::GetFlightDeckComponent(int32 DeckIndex) const
{
    return FlightDeckComponents.IsValidIndex(DeckIndex) ? FlightDeckComponents[DeckIndex].Get() : nullptr;
}

UCarrierHangarComponent* ACarrierActor::GetHangarComponent() const
{
    return HangarComponent.Get();
}

UCarrierHangarComponent* ACarrierActor::GetHangarBayComponent(int32 DeckIndex) const
{
    return HangarBayComponents.IsValidIndex(DeckIndex) ? HangarBayComponents[DeckIndex].Get() : nullptr;
}

USceneComponent* ACarrierActor::GetDeckMarker(int32 DeckIndex) const
{
    return GetFlightDeckComponent(DeckIndex);
}

USceneComponent* ACarrierActor::GetLaunchSpotMarker(int32 DeckIndex, int32 SpotIndex) const
{
    for (int32 i = 0; i < LaunchSpotMarkers.Num(); ++i)
        if (LaunchMarkerDeckIndices.IsValidIndex(i) && LaunchMarkerSpotIndices.IsValidIndex(i) &&
            LaunchMarkerDeckIndices[i] == DeckIndex && LaunchMarkerSpotIndices[i] == SpotIndex)
            return LaunchSpotMarkers[i];
    return nullptr;
}

USceneComponent* ACarrierActor::GetRecoveryMarker(int32 DeckIndex) const
{
    for (int32 i = 0; i < RecoveryMarkers.Num(); ++i)
        if (RecoveryMarkerDeckIndices.IsValidIndex(i) && RecoveryMarkerDeckIndices[i] == DeckIndex)
            return RecoveryMarkers[i];
    return nullptr;
}
