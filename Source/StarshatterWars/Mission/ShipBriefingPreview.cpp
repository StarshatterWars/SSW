
#include "ShipBriefingPreview.h"
#include "ShipVisualCatalog.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionEvent.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Instruction.h"
#include "GameStructs_System.h"
#include "ShipActor.h"
#include "ShipWeaponMountComponent.h"
#include "WeaponBlueprintResolver.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Viewport.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "MissionUIStyle.h"

namespace
{
    void CollectTemplateParts(UClass* ActorClass, const FTransform& ParentTransform,
        TArray<FBriefingShipPreviewPart>& Parts, TSet<UClass*>& ClassStack, bool bSkipLoadoutStores = false)
    {
        if (!ActorClass || ClassStack.Contains(ActorClass) || ClassStack.Num() >= 6) return;
        const AActor* DefaultActor = ActorClass->GetDefaultObject<AActor>();
        if (!DefaultActor) return;
        ClassStack.Add(ActorClass);
        TArray<USceneComponent*> Components;
        DefaultActor->GetComponents(Components);
        TMap<FName, USceneComponent*> ByName;
        TMap<USceneComponent*, USceneComponent*> Parents;
        for (USceneComponent* Component : Components)
            if (Component) ByName.Add(Component->GetFName(), Component);

        UBlueprintGeneratedClass* ActualClass = Cast<UBlueprintGeneratedClass>(ActorClass);
        for (UClass* Class = ActorClass; Class; Class = Class->GetSuperClass())
        {
            UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class);
            if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript) continue;
            for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
            {
                USceneComponent* Component = Node ? Cast<USceneComponent>(Node->GetActualComponentTemplate(ActualClass)) : nullptr;
                if (!Component) continue;
                Components.AddUnique(Component);
                ByName.Add(Node->GetVariableName(), Component);
                for (USCS_Node* Child : Node->GetChildNodes())
                    if (Child)
                        if (USceneComponent* ChildComponent = Cast<USceneComponent>(Child->GetActualComponentTemplate(ActualClass)))
                            Parents.Add(ChildComponent, Component);
            }
        }
        for (UClass* Class = ActorClass; Class; Class = Class->GetSuperClass())
        {
            UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class);
            if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript) continue;
            for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
            {
                if (!Node || Node->ParentComponentOrVariableName.IsNone()) continue;
                USceneComponent* Component = Cast<USceneComponent>(Node->GetActualComponentTemplate(ActualClass));
                if (USceneComponent** Parent = ByName.Find(Node->ParentComponentOrVariableName))
                    if (Component && *Parent != Component && !Parents.Contains(Component)) Parents.Add(Component, *Parent);
            }
        }
        const AShipActor* ShipDefaults = Cast<AShipActor>(DefaultActor);
        auto RelativeTransform = [&Parents, ShipDefaults](const USceneComponent* Component)
        {
            FTransform Transform = FTransform::Identity;
            TSet<const USceneComponent*> Visited;
            for (const USceneComponent* Current = Component; Current && !Visited.Contains(Current);)
            {
                Visited.Add(Current);
                FTransform Local = Current->GetRelativeTransform();
                // Match AShipActor::OnConstruction without running it or modifying its CDO.
                if (ShipDefaults && Current == ShipDefaults->VisualRoot)
                    Local.SetRotation(FRotator(0.f, 90.f, 0.f).Quaternion());
                Transform = Transform * Local;
                const USceneComponent* Parent = Current->GetAttachParent();
                if (!Parent)
                    if (USceneComponent* const* SavedParent = Parents.Find(const_cast<USceneComponent*>(Current))) Parent = *SavedParent;
                Current = Parent;
            }
            return Transform;
        };
        for (USceneComponent* Source : Components)
        {
            if (!Source || !Source->GetVisibleFlag() || Source->bHiddenInGame) continue;
            const FTransform Transform = RelativeTransform(Source) * ParentTransform;
            if (const UStaticMeshComponent* MeshSource = Cast<UStaticMeshComponent>(Source))
            {
                if (!MeshSource->GetStaticMesh()) continue;
                FBriefingShipPreviewPart Part;
                Part.Mesh = MeshSource->GetStaticMesh();
                Part.Transform = Transform;
                for (UMaterialInterface* Material : MeshSource->GetMaterials()) Part.Materials.Add(Material);
                Parts.Add(MoveTemp(Part));
            }
            else if (const UChildActorComponent* Child = Cast<UChildActorComponent>(Source))
            {
                const auto* Mount = Cast<UShipWeaponMountComponent>(Child);
                if (!bSkipLoadoutStores || !Mount || !Mount->bLoadoutHardpoint)
                    CollectTemplateParts(Child->GetChildActorClass(), Transform, Parts, ClassStack, bSkipLoadoutStores);
            }
        }
        if (ShipDefaults && Parts.IsEmpty() && ShipDefaults->ShipVisualBP)
            CollectTemplateParts(ShipDefaults->ShipVisualBP, ParentTransform, Parts, ClassStack, bSkipLoadoutStores);
        ClassStack.Remove(ActorClass);
    }
}

FBox BriefingShipPreview::CalculateBounds(const TArray<FBriefingShipPreviewPart>& Parts)
{
    FBox Bounds(ForceInit);
    for (const FBriefingShipPreviewPart& Part : Parts)
        if (Part.Mesh && !Part.Transform.ContainsNaN())
            Bounds += Part.Mesh->GetBoundingBox().TransformBy(Part.Transform);
    return Bounds;
}

bool BriefingShipPreview::ResolveParts(const FShipDesign& Design, TArray<FBriefingShipPreviewPart>& OutParts,
    const TArray<const FWeaponDesign*>& Stores)
{
    OutParts.Empty();
    UClass* VisualClass = nullptr;
    if (!Design.ShipName.IsEmpty() && (!Stores.IsEmpty() || Design.Mesh.IsNull()))
        VisualClass = ShipVisualCatalog::Resolve(Design.ShipName, Design.Model);
    if (UStaticMesh* Mesh = Design.Mesh.LoadSynchronous())
    {
        FBriefingShipPreviewPart Part;
        Part.Mesh = Mesh;
        OutParts.Add(MoveTemp(Part));
    }
    if (OutParts.IsEmpty() && !Design.ShipName.IsEmpty())
    {
        if (VisualClass)
        {
            TSet<UClass*> ClassStack;
            CollectTemplateParts(VisualClass, FTransform::Identity, OutParts, ClassStack, !Stores.IsEmpty());
        }
        if (OutParts.IsEmpty())
        {
            const FString MeshPackage = FString::Printf(TEXT("/Game/Models/%s/%s"), *Design.ShipName, *Design.ShipName);
            if (FPackageName::IsValidLongPackageName(MeshPackage) && FPackageName::DoesPackageExist(MeshPackage))
            {
                if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *(MeshPackage + TEXT(".") + Design.ShipName)))
                {
                    FBriefingShipPreviewPart Part;
                    Part.Mesh = Mesh;
                    OutParts.Add(MoveTemp(Part));
                }
            }
        }
    }
    const FBox Bounds = CalculateBounds(OutParts);
    const AShipActor* Defaults = VisualClass ? Cast<AShipActor>(VisualClass->GetDefaultObject()) : nullptr;
    const double ShipScale = FMath::Abs(Design.Scale) > SMALL_NUMBER ? Design.Scale : 1.0;
    for (int32 Index = 0; Bounds.IsValid && Index < Stores.Num() && Design.Hardpoint.IsValidIndex(Index); ++Index)
    {
        const FWeaponDesign* Store = Stores[Index];
        if (!Store) continue;
        UClass* StoreClass = WeaponBlueprintResolver::Resolve(Store->Model, Store->Name);
        if (!StoreClass) continue;

        const FShipHardPoint& Hardpoint = Design.Hardpoint[Index];
        const FVector& Location = Hardpoint.Location;
        FTransform Mount(FRotator(FMath::RadiansToDegrees(Hardpoint.ElevationRadians),
            FMath::RadiansToDegrees(Hardpoint.AzimuthRadians), 0.0),
            FVector(Location.Z, Location.X, Location.Y) / ShipScale);
        if (Defaults && Defaults->LoadoutHardpoints.IsValidIndex(Index) && Defaults->LoadoutHardpoints[Index])
        {
            Mount = FTransform::Identity;
            for (const USceneComponent* Point = Defaults->LoadoutHardpoints[Index].Get(); Point; Point = Point->GetAttachParent())
                Mount = Mount * Point->GetRelativeTransform();
        }

        TArray<FBriefingShipPreviewPart> StoreParts;
        TSet<UClass*> ClassStack;
        CollectTemplateParts(StoreClass, FTransform::Identity, StoreParts, ClassStack);
        for (FBriefingShipPreviewPart& Part : StoreParts)
        {
            const FVector Extent = Part.Mesh->GetBounds().BoxExtent;
            // Match the carried missile mesh convention used by AMissileActor.
            if (Part.Mesh->GetPathName().StartsWith(TEXT("/Game/Models/Weapons/")) &&
                Extent.Y > 2.0 * FMath::Max(Extent.X, Extent.Z))
            {
                const FVector ModelForward = Part.Transform.TransformVector(FVector::RightVector).GetSafeNormal();
                Part.Transform.SetRotation(FQuat::FindBetweenNormals(ModelForward, FVector::ForwardVector) *
                    Part.Transform.GetRotation());
            }
            Part.Transform = Part.Transform * Mount;
            OutParts.Add(MoveTemp(Part));
        }
    }

    const double Radius = Bounds.IsValid ? Bounds.GetExtent().Size() : 0.;
    if (!FMath::IsFinite(Radius) || Radius <= UE_SMALL_NUMBER)
    {
        OutParts.Empty();
        return false;
    }
    const double Scale = 100. / Radius;
    for (FBriefingShipPreviewPart& Part : OutParts)
    {
        Part.Transform.SetTranslation((Part.Transform.GetTranslation() - Bounds.GetCenter()) * Scale);
        Part.Transform.SetScale3D(Part.Transform.GetScale3D() * Scale);
    }
    return true;
}

float BriefingShipPreview::FitDistance(float Radius, const FVector2D& ViewSize)
{
    if (!FMath::IsFinite(Radius) || Radius <= 0.f) Radius = 100.f;
    const double Aspect = ViewSize.X > 1. && ViewSize.Y > 1. ? ViewSize.X / ViewSize.Y : 1.;
    // UViewport uses the default 90-degree FOV. Conservatively fit either FOV-axis mode.
    const double LimitingAspect = FMath::Clamp(FMath::Min(Aspect, 1. / Aspect), 0.05, 1.);
    return static_cast<float>(Radius / FMath::Sin(FMath::Atan(LimitingAspect)) * 1.18);
}

AActor* BriefingShipPreview::CreateVisual(UWorld* PreviewWorld, const TArray<FBriefingShipPreviewPart>& Parts, bool bAddLighting)
{
    if (!PreviewWorld || (PreviewWorld->WorldType != EWorldType::GamePreview &&
        PreviewWorld->WorldType != EWorldType::EditorPreview) || Parts.IsEmpty()) return nullptr;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    AActor* Visual = PreviewWorld->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
    if (!Visual) return nullptr;
    USceneComponent* Root = NewObject<USceneComponent>(Visual);
    Visual->SetRootComponent(Root);
    Root->RegisterComponent();
    for (const FBriefingShipPreviewPart& Part : Parts)
    {
        if (!Part.Mesh) continue;
        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Visual);
        Visual->AddInstanceComponent(Mesh);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetupAttachment(Root);
        Mesh->SetStaticMesh(Part.Mesh);
        Mesh->SetRelativeTransform(Part.Transform);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetComponentTickEnabled(false);
        for (int32 Index = 0; Index < Part.Materials.Num(); ++Index)
            if (Part.Materials[Index]) Mesh->SetMaterial(Index, Part.Materials[Index]);
        Mesh->RegisterComponent();
    }
    Visual->SetActorEnableCollision(false);
    Visual->SetActorTickEnabled(false);
    if (!bAddLighting) return Visual;

    UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(Visual);
    Visual->AddInstanceComponent(Fill);
    Fill->SetupAttachment(Root);
    Fill->SetRelativeRotation(FRotator(-25.f, 140.f, 0.f));
    Fill->SetIntensity(1.5f);
    Fill->SetMobility(EComponentMobility::Movable);
    Fill->SetForwardShadingPriority(1);
    Fill->SetLightColor(FLinearColor(0.65f, 0.8f, 1.f));
    Fill->SetCastShadows(false);
    Fill->RegisterComponent();
    if (UTextureCube* Ambient = LoadObject<UTextureCube>(nullptr,
        TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")))
    {
        USkyLightComponent* Sky = NewObject<USkyLightComponent>(Visual);
        Visual->AddInstanceComponent(Sky);
        Sky->SetupAttachment(Root);
        Sky->SetMobility(EComponentMobility::Movable);
        Sky->SourceType = SLS_SpecifiedCubemap;
        Sky->SetCubemap(Ambient);
        Sky->SetIntensity(0.8f);
        Sky->RegisterComponent();
    }
    return Visual;
}

TSharedRef<SWidget> UShipBriefingPreview::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    if (!PreviewViewport)
    {
        UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
        WidgetTree->RootWidget = Overlay;
        PreviewViewport = WidgetTree->ConstructWidget<UViewport>();
        PreviewViewport->SetBackgroundColor(FLinearColor(0.018f, 0.025f, 0.035f, 1.f));
        PreviewViewport->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* ViewSlot = Overlay->AddChildToOverlay(PreviewViewport);
        ViewSlot->SetHorizontalAlignment(HAlign_Fill);
        ViewSlot->SetVerticalAlignment(VAlign_Fill);
        UnavailableText = WidgetTree->ConstructWidget<UTextBlock>();
        UnavailableText->SetText(NSLOCTEXT("MissionBriefing", "3DUnavailable", "3D PREVIEW UNAVAILABLE"));
        UnavailableText->SetFont(MissionUIStyle::GetInfoValueFont(16));
        UnavailableText->SetAutoWrapText(true);
        UnavailableText->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* Message = Overlay->AddChildToOverlay(UnavailableText);
        Message->SetHorizontalAlignment(HAlign_Center);
        Message->SetVerticalAlignment(VAlign_Center);
        RouteCaption = WidgetTree->ConstructWidget<UTextBlock>();
        RouteCaption->SetFont(MissionUIStyle::GetInfoValueFont(16));
        RouteCaption->SetColorAndOpacity(FSlateColor(FLinearColor(.55f, .9f, 1.f)));
        RouteCaption->SetVisibility(ESlateVisibility::Collapsed);
        UOverlaySlot* CaptionSlot = Overlay->AddChildToOverlay(RouteCaption);
        CaptionSlot->SetPadding(FMargin(14.f));
        CaptionSlot->SetHorizontalAlignment(HAlign_Left);
        CaptionSlot->SetVerticalAlignment(VAlign_Bottom);
        CaptionSlot->SetPadding(FMargin(14.f, 14.f, 14.f, 62.f));
        RouteCaption->SetAutoWrapText(true);

        EventPopup = WidgetTree->ConstructWidget<UBorder>();
        EventPopup->SetBrushColor(MissionUIStyle::PanelBG);
        EventPopup->SetPadding(FMargin(18.f, 10.f));
        EventPopup->SetVisibility(ESlateVisibility::Collapsed);
        EventPopupText = WidgetTree->ConstructWidget<UTextBlock>();
        EventPopupText->SetFont(MissionUIStyle::GetInfoValueFont(18));
        EventPopupText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        EventPopupText->SetWrapTextAt(540.f);
        EventPopup->SetContent(EventPopupText);
        UOverlaySlot* EventSlot = Overlay->AddChildToOverlay(EventPopup);
        EventSlot->SetHorizontalAlignment(HAlign_Center);
        EventSlot->SetVerticalAlignment(VAlign_Top);
        EventSlot->SetPadding(FMargin(12.f, 24.f));

        auto* ChecklistPanel = WidgetTree->ConstructWidget<UBorder>();
        ChecklistPanel->SetBrushColor(MissionUIStyle::PanelBG);
        ChecklistPanel->SetPadding(FMargin(14.f, 10.f));
        ChecklistPanel->SetVisibility(ESlateVisibility::Collapsed);
        auto* ChecklistSlot = Overlay->AddChildToOverlay(ChecklistPanel);
        ChecklistSlot->SetHorizontalAlignment(HAlign_Right);
        ChecklistSlot->SetVerticalAlignment(VAlign_Bottom);
        ChecklistSlot->SetPadding(FMargin(12.f, 12.f, 16.f, 62.f));
        RouteChecklist = WidgetTree->ConstructWidget<UVerticalBox>();
        ChecklistPanel->SetContent(RouteChecklist);

        PlaybackControls = WidgetTree->ConstructWidget<UHorizontalBox>();
        PlaybackControls->SetVisibility(ESlateVisibility::Collapsed);
        UOverlaySlot* ControlsSlot = Overlay->AddChildToOverlay(PlaybackControls);
        ControlsSlot->SetHorizontalAlignment(HAlign_Center);
        ControlsSlot->SetVerticalAlignment(VAlign_Bottom);
        ControlsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
        auto AddControl = [this](const FText& Label, const TCHAR* IconName, const TCHAR* Color) {
            auto* Button = WidgetTree->ConstructWidget<UButton>();
            auto* Icon = WidgetTree->ConstructWidget<UImage>();
            const FString Path = FString::Printf(
                TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/%s.%s"), IconName, IconName);
            MissionUIStyle::SetActionIcon(Icon, *Path, FLinearColor::White);
            Icon->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
            Button->SetContent(Icon);
            Button->SetToolTipText(Label);
            MissionUIStyle::ApplyMenuButtonStyle(Button, GetGameInstance());
            MissionUIStyle::ApplyActionButtonStyle(Button, Color);
            if (auto* IconSlot = Cast<UButtonSlot>(Icon->Slot))
            {
                IconSlot->SetHorizontalAlignment(HAlign_Center);
                IconSlot->SetVerticalAlignment(VAlign_Center);
            }
            auto* Size = WidgetTree->ConstructWidget<USizeBox>();
            Size->SetWidthOverride(80.f);
            Size->SetHeightOverride(42.f);
            Size->SetContent(Button);
            PlaybackControls->AddChildToHorizontalBox(Size)->SetPadding(FMargin(5.f, 0.f));
            return Button;
        };
        AddControl(NSLOCTEXT("MissionRehearsal", "Play", "Play"), TEXT("T_Play_256x256"), TEXT("Green"))
            ->OnClicked.AddDynamic(this, &UShipBriefingPreview::PlayRehearsal);
        AddControl(NSLOCTEXT("MissionRehearsal", "Rewind", "Rewind"), TEXT("T_Play_Rev_256x256"), TEXT("Cyan"))
            ->OnClicked.AddDynamic(this, &UShipBriefingPreview::RewindRehearsal);
        AddControl(NSLOCTEXT("MissionRehearsal", "Pause", "Pause"), TEXT("T_Pause_256x256"), TEXT("Blue"))
            ->OnClicked.AddDynamic(this, &UShipBriefingPreview::PauseRehearsal);
        AddControl(NSLOCTEXT("MissionRehearsal", "Stop", "Stop"), TEXT("T_Stop_256x256"), TEXT("Red"))
            ->OnClicked.AddDynamic(this, &UShipBriefingPreview::StopRehearsal);
    }
    return Super::RebuildWidget();
}

void UShipBriefingPreview::NativeConstruct()
{
    Super::NativeConstruct();
    // UViewport's Slate resources now exist. Never create scene actors during RebuildWidget.
    if (!IsDesignTime() && PreviewViewport && PreviewViewport->GetViewportWorld())
    {
        PreviewViewport->SetEnableAdvancedFeatures(false);
        PreviewViewport->SetLightIntensity(3.f);
        PreviewViewport->SetSkyIntensity(0.f);
        PreviewViewport->SetShowFlag(TEXT("Fog"), false);
        UpdateVisual();
    }
}

void UShipBriefingPreview::ClearVisual()
{
    bDragging = false;
    if (HasMouseCapture() && FSlateApplication::IsInitialized()) FSlateApplication::Get().ReleaseMouseCapture();
    if (AActor* Visual = PreviewVisual.Get()) Visual->Destroy();
    PreviewVisual.Reset();
    if (AActor* Visual = RouteShip.Get()) Visual->Destroy();
    RouteShip.Reset();
}

void UShipBriefingPreview::NativeDestruct()
{
    ClearVisual();
    Parts.Empty();
    DesignKey.Empty();
    RouteShipParts.Reset();
    Route.Reset();
    RouteLabels.Reset();
    bMissionScene = false;
    Super::NativeDestruct();
}

bool UShipBriefingPreview::SetShipDesign(const FShipDesign* Design, const TArray<const FWeaponDesign*>& Stores)
{
    if (bMissionScene)
    {
        ClearVisual();
        Parts.Reset();
        RouteShipParts.Reset();
        Route.Reset();
        RouteLabels.Reset();
        bMissionScene = false;
        if (RouteCaption) RouteCaption->SetVisibility(ESlateVisibility::Collapsed);
        if (PlaybackControls) PlaybackControls->SetVisibility(ESlateVisibility::Collapsed);
        bRehearsalPlaying = false;
    }
    FString NewKey = Design ? Design->ShipName + TEXT("|") + Design->Mesh.ToSoftObjectPath().ToString() : FString();
    const FString ShipKey = NewKey;
    for (const FWeaponDesign* Store : Stores)
        NewKey += TEXT("|") + (Store ? Store->Name : FString());
    const bool bSameShip = !ShipKey.IsEmpty() && (DesignKey == ShipKey || DesignKey.StartsWith(ShipKey + TEXT("|")));
    if (DesignKey == NewKey && !Parts.IsEmpty()) { UpdateVisual(); return true; }
    ClearVisual();
    Parts.Empty();
    DesignKey = NewKey;
    if (!Design || !BriefingShipPreview::ResolveParts(*Design, Parts, Stores)) return false;
    if (!bSameShip)
    {
        ResetView();
        if (!Stores.IsEmpty())
            OrbitPitch = -25.f;
    }
    UpdateVisual();
    return true;
}

bool UShipBriefingPreview::SetMissionScene(Mission* MissionDef)
{
    MissionElement* Player = MissionDef ? MissionDef->GetPlayer() : nullptr;
    if (!Player || !Player->GetShipDesign()) return false;

    auto ResolveRehearsalParts = [](const FShipDesign& Design, TArray<FBriefingShipPreviewPart>& Model) {
        if (UClass* VisualClass = ShipVisualCatalog::Resolve(Design.ShipName, Design.Model))
        {
            TSet<UClass*> ClassStack;
            CollectTemplateParts(VisualClass, FTransform::Identity, Model, ClassStack, false);
        }
        return !Model.IsEmpty() || BriefingShipPreview::ResolveParts(Design, Model);
    };
    TArray<FBriefingShipPreviewPart> PlayerParts;
    if (!ResolveRehearsalParts(*Player->GetShipDesign(), PlayerParts)) return false;

    ClearVisual();
    Parts.Reset();
    DesignKey.Reset();
    Route.Reset();
    RouteLabels.Reset();
    RouteTime = 0.0;
    RehearsalDuration = 0.0;
    LastCaptionSecond = INDEX_NONE;
    bRehearsalPlaying = false;
    RouteArrivals.Reset();
    RouteDepartures.Reset();
    TimedCues.Reset();
    bMissionScene = true;

    // Use authored coordinates within each sector. Separate sectors in the
    // display so astronomical distances do not flatten the tactical route.
    TMap<FString, int32> SectorIndices;
    const FVector Origin = Player->GetLocation();
    const FString PlayerSector = ANSI_TO_TCHAR(Player->GetRegion().data());
    SectorIndices.Add(PlayerSector, 0);
    auto ScenePosition = [&](FVector Position, const char* Region) {
        const FString Sector = Region && *Region ? FString(ANSI_TO_TCHAR(Region)) : PlayerSector;
        int32* Existing = SectorIndices.Find(Sector);
        const int32 Index = Existing ? *Existing : SectorIndices.Add(Sector, SectorIndices.Num());
        const FVector Local = Index == 0 ? Position - Origin : Position;
        return FVector(Local.Z, Local.X, Local.Y) + FVector(Index * 120000.0, 0.0, 0.0);
    };

    Route.Add(FVector::ZeroVector);
    RouteArrivals.Add(0.0);
    RouteDepartures.Add(0.0);
    FVector PreviousLocation = Origin;
    bool bReturnsToBase = false;
    Text RecoveryTarget;
    for (ListIter<Instruction> Point = Player->NavList(); ++Point;)
    {
        const double Speed = Point->GetSpeed() > 0 ? Point->GetSpeed() : 250.0;
        const double Travel = FVector::Distance(PreviousLocation, Point->GetLocation()) / Speed;
        const double Arrival = RouteDepartures.Last() + FMath::Max(Travel, 1.0);
        RouteArrivals.Add(Arrival);
        RouteDepartures.Add(Arrival + FMath::Max(Point->GetHoldTime(), 0.0));
        PreviousLocation = Point->GetLocation();
        Route.Add(ScenePosition(Point->GetLocation(), Point->GetRegionName()));
        bReturnsToBase = Point->GetAction() == EInstruction::RTB;
        if (bReturnsToBase) RecoveryTarget = Point->GetTargetName();
        const int32 Waypoint = RouteLabels.Num() + 1;
        if (!bReturnsToBase)
        {
            const bool bSweep = Point->GetAction() == EInstruction::Sweep;
            const FString Cue = bSweep
                ? FString::Printf(TEXT("Proceed to NAV %02d for combat patrol"), Waypoint)
                : FString::Printf(TEXT("Proceed to waypoint %02d"), Waypoint);
            TimedCues.Add({RouteArrivals[Waypoint - 1], Cue});
        }
        RouteLabels.Add(FString::Printf(TEXT("NAV %02d  %s"), RouteLabels.Num() + 1,
            ANSI_TO_TCHAR(Point->GetRegionName())));
    }

    Text CarrierName = RecoveryTarget.length() ? RecoveryTarget : Player->GetCarrier();
    // Training flights can inherit their home carrier from the parent squadron.
    if (bReturnsToBase && !CarrierName.length() && Player->GetSquadron().length())
    {
        for (ListIter<MissionElement> Entry = MissionDef->GetElements(); ++Entry;)
        {
            const MissionElement* Squadron = Entry.value();
            if (Squadron && Squadron->GetName() == Player->GetSquadron())
            {
                CarrierName = Squadron->GetCarrier();
                break;
            }
        }
    }

    if (bReturnsToBase && CarrierName.length())
    {
        for (ListIter<MissionElement> Entry = MissionDef->GetElements(); ++Entry;)
        {
            const MissionElement* Base = Entry.value();
            if (!Base || Base->GetName() != CarrierName) continue;
            TimedCues.Add({RouteArrivals.Last(), FString::Printf(
                TEXT("Return to %s"), UTF8_TO_TCHAR(CarrierName.data()))});
            const FVector Destination = ScenePosition(Base->GetLocation(), Base->GetRegion().data());
            if (!Destination.Equals(Route.Last(), 1.0))
            {
                const double Arrival = RouteDepartures.Last() + FMath::Max(FVector::Dist(Route.Last(), Destination) / 250.0, 1.0);
                Route.Add(Destination);
                RouteArrivals.Add(Arrival);
                RouteDepartures.Add(Arrival);
                TimedCues.Add({Arrival, FString::Printf(
                    TEXT("Arrived at %s"), UTF8_TO_TCHAR(CarrierName.data()))});
                RouteLabels.Add(FString::Printf(TEXT("RETURN TO %s"), UTF8_TO_TCHAR(CarrierName.data())));
            }
            break;
        }
    }

    if (RouteChecklist)
    {
        RouteChecklist->ClearChildren();
        auto* Title = WidgetTree->ConstructWidget<UTextBlock>();
        Title->SetText(NSLOCTEXT("MissionRehearsal", "RouteObjectives", "ROUTE OBJECTIVES"));
        Title->SetFont(MissionUIStyle::GetHeaderFont(14));
        RouteChecklist->AddChildToVerticalBox(Title);
        for (const FString& Label : RouteLabels)
        {
            auto* Row = WidgetTree->ConstructWidget<UTextBlock>();
            Row->SetFont(MissionUIStyle::GetInfoValueFont(14));
            RouteChecklist->AddChildToVerticalBox(Row);
        }
    }

    RehearsalDuration = RouteDepartures.Last();
    // In the rehearsal, anchor scripted spawns to their navigation milestone.
    // Combat prerequisites remain enforced by the live mission.
    TMap<int32, double> NavigationEventTimes;
    for (ListIter<MissionEvent> Events = MissionDef->GetEvents(); ++Events;)
    {
        const MissionEvent* Event = Events.value();
        if (!Event) continue;
        double At = -1.0;
        if (Event->Trigger() == MissionEvent::TRIGGER_NAVPT &&
            RouteArrivals.IsValidIndex(Event->TriggerParam()) &&
            Event->TriggerShip().indexOf(Player->GetName()) == 0)
        {
            At = RouteArrivals[Event->TriggerParam()];
        }
        else if (Event->Trigger() == MissionEvent::TRIGGER_EVENT ||
                 Event->Trigger() == MissionEvent::TRIGGER_EVENT_ALL)
        {
            for (int32 Index = 0; Index < Event->NumTriggerParams(); ++Index)
                if (const double* Time = NavigationEventTimes.Find(Event->TriggerParam(Index)))
                    At = FMath::Max(At, *Time);
        }
        if (At < 0.0) continue;
        if (Event->EventID() > 0) NavigationEventTimes.Add(Event->EventID(), At);
        if (Event->Event() == MissionEvent::SPAWN && Event->EventShip().length())
            TimedCues.Add({At, FString::Printf(TEXT("Contacts arriving: %s"),
                UTF8_TO_TCHAR(Event->EventShip().data()))});
    }
    TimedCues.StableSort([](const FTimedCue& A, const FTimedCue& B) { return A.Time < B.Time; });

    struct FPlacement { MissionElement* Element; FVector Position; };
    TArray<FPlacement> Ships;
    FBox Layout(ForceInit);
    for (const FVector& Point : Route) Layout += Point;
    for (ListIter<MissionElement> Entry = MissionDef->GetElements(); ++Entry;)
    {
        MissionElement* Element = Entry.value();
        if (!Element || Element->IsSquadron() || !Element->GetShipDesign() || Element->Count() <= 0)
            continue;
        const FVector Position = ScenePosition(Element->GetLocation(), Element->GetRegion().data());
        Ships.Add({Element, Position});
        Layout += Position;
    }
    const double Scale = 650.0 / FMath::Max(Layout.GetExtent().Size(), 1000.0);
    const FVector Center = Layout.GetCenter();
    for (FVector& Point : Route) Point = (Point - Center) * Scale;

    auto AddParts = [](TArray<FBriefingShipPreviewPart>& Destination,
        const TArray<FBriefingShipPreviewPart>& Source, const FTransform& Placement) {
        for (FBriefingShipPreviewPart Part : Source)
        {
            Part.Transform = Part.Transform * Placement;
            Destination.Add(MoveTemp(Part));
        }
    };
    RouteShipParts.Reset();
    AddParts(RouteShipParts, PlayerParts, FTransform(FQuat::Identity, FVector::ZeroVector, FVector(.42)));

    TMap<const FShipDesign*, TArray<FBriefingShipPreviewPart>> Models;
    Models.Add(Player->GetShipDesign(), MoveTemp(PlayerParts));
    for (const FPlacement& Placement : Ships)
    {
        MissionElement* Element = Placement.Element;
        const FShipDesign* Design = Element->GetShipDesign();
        auto* Model = Models.Find(Design);
        if (!Model)
        {
            Model = &Models.Add(Design);
            ResolveRehearsalParts(*Design, *Model);
        }
        const double Size = Element->IsStarship() ? .82 : .32;
        const FQuat Heading(FVector::UpVector, Element->GetHeading() + PI);
        const FVector Position = (Placement.Position - Center) * Scale;
        for (int32 ShipIndex = 0; ShipIndex < FMath::Min(Element->Count(), 8); ++ShipIndex)
        {
            if (Element == Player && ShipIndex + 1 == Player->GetPlayerIndex()) continue;
            const FVector Offset = Heading.RotateVector(FVector(-30.0 * ShipIndex, 32.0 * ShipIndex, 0.0));
            AddParts(Parts, *Model, FTransform(Heading, Position + Offset, FVector(Size)));
        }
    }

    UStaticMesh* Line = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Emissive = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materials/M_NavlightEmissive.M_NavlightEmissive"));
    UMaterialInstanceDynamic* RouteGlow = Emissive ? UMaterialInstanceDynamic::Create(Emissive, this) : nullptr;
    if (RouteGlow)
    {
        RouteGlow->SetVectorParameterValue(TEXT("GlowColor"), FLinearColor(.08f, .65f, 1.f));
        RouteGlow->SetScalarParameterValue(TEXT("GlowIntensity"), 4.f);
    }
    UMaterialInterface* MarkerBase = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Models/Waypoint/M_WaypointMarker.M_WaypointMarker"));
    UMaterialInstanceDynamic* MarkerGlow = MarkerBase ? UMaterialInstanceDynamic::Create(MarkerBase, this) : nullptr;
    if (MarkerGlow)
    {
        MarkerGlow->SetVectorParameterValue(TEXT("MarkerColor"), FLinearColor(.08f, .65f, 1.f));
        MarkerGlow->SetScalarParameterValue(TEXT("Intensity"), 2.f);
        MarkerGlow->SetScalarParameterValue(TEXT("Opacity"), .55f);
    }
    auto AddLine = [&](const FVector& Start, const FVector& End, double Width, UMaterialInterface* Material) {
        const FVector Direction = End - Start;
        if (Direction.IsNearlyZero()) return;
        FBriefingShipPreviewPart& Part = Parts.AddDefaulted_GetRef();
        Part.Mesh = Line;
        if (Material) Part.Materials.Add(Material);
        Part.Transform = FTransform(Direction.Rotation(), (Start + End) * .5,
            FVector(Direction.Size() / 100.0, Width, Width));
    };
    constexpr int32 RingSegments = 16;
    constexpr double MarkerRadius = 13.0;
    const FVector Axes[] = {FVector::ForwardVector, FVector::RightVector, FVector::UpVector};
    for (int32 Index = 1; Index < Route.Num(); ++Index)
    {
        AddLine(Route[Index - 1], Route[Index], .032, RouteGlow);
        for (int32 Plane = 0; Plane < 3; ++Plane)
        {
            auto RingPoint = [&](int32 Step) {
                const double Angle = 2.0 * PI * Step / RingSegments;
                return Route[Index] + MarkerRadius * (Axes[Plane] * FMath::Cos(Angle)
                    + Axes[(Plane + 1) % 3] * FMath::Sin(Angle));
            };
            for (int32 Step = 0; Step < RingSegments; ++Step)
                AddLine(RingPoint(Step), RingPoint(Step + 1), .014, MarkerGlow ? MarkerGlow : RouteGlow);
        }
    }

    MissionBounds = BriefingShipPreview::CalculateBounds(Parts);
    for (const FVector& Point : Route)
    {
        MissionBounds += Point + FVector(26.0);
        MissionBounds += Point - FVector(26.0);
    }
    ResetView();
    UpdateVisual();
    return true;
}

void UShipBriefingPreview::SetPreviewActive(bool bActive)
{
    bPreviewActive = bActive;
    SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bActive) UpdateVisual();
    else { PauseRehearsal(); ClearVisual(); }
}

void UShipBriefingPreview::UpdateVisual()
{
    if (IsDesignTime() || !bPreviewActive || !PreviewViewport || !PreviewViewport->GetViewportWorld()) return;
    if (PreviewVisual.IsValid() && PreviewVisual->GetWorld() == PreviewViewport->GetViewportWorld()) return;
    ClearVisual();
    PreviewViewport->SetShowFlag(TEXT("EyeAdaptation"), !bMissionScene);
    PreviewViewport->SetBackgroundColor(bMissionScene ? FLinearColor(.002f, .004f, .009f, 1.f)
        : FLinearColor(.018f, .025f, .035f, 1.f));
    PreviewVisual = BriefingShipPreview::CreateVisual(PreviewViewport->GetViewportWorld(), Parts);
    if (bMissionScene && PreviewVisual.IsValid())
    {
        RouteShip = BriefingShipPreview::CreateVisual(PreviewViewport->GetViewportWorld(), RouteShipParts, false);
        if (AActor* Visual = RouteShip.Get()) Visual->SetActorLocation(Route[0]);
    }
    if (EventPopup) EventPopup->SetVisibility(ESlateVisibility::Collapsed);
    if (RouteChecklist && RouteChecklist->GetParent())
        RouteChecklist->GetParent()->SetVisibility(bMissionScene && RouteShip.IsValid() && !RouteLabels.IsEmpty()
            ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (RouteCaption) RouteCaption->SetVisibility(bMissionScene && RouteShip.IsValid()
        ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (PlaybackControls) PlaybackControls->SetVisibility(bMissionScene && RouteShip.IsValid()
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    LastCaptionSecond = INDEX_NONE;
    if (bMissionScene) UpdateRehearsal();
    if (UnavailableText) UnavailableText->SetVisibility(PreviewVisual.IsValid()
        ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    UpdateCamera();
}

void UShipBriefingPreview::ResetView()
{
    OrbitYaw = -130.f;
    OrbitPitch = bMissionScene ? 48.f : 20.f;
    Zoom = bMissionScene ? .43f : 1.f;
    UpdateCamera();
}

void UShipBriefingPreview::UpdateCamera()
{
    if (!PreviewViewport || !PreviewViewport->GetViewportWorld() || Parts.IsEmpty()) return;
    FVector2D Size = PreviewViewport->GetCachedGeometry().GetLocalSize();
    if (Size.X <= 1.f || Size.Y <= 1.f) Size = GetCachedGeometry().GetLocalSize();
    LastViewSize = Size;
    const FBox Bounds = bMissionScene ? MissionBounds : BriefingShipPreview::CalculateBounds(Parts);
    const FVector Center = Bounds.IsValid ? Bounds.GetCenter() : FVector::ZeroVector;
    const float Radius = Bounds.IsValid ? static_cast<float>(Bounds.GetExtent().Size()) : 100.f;
    const FVector Direction = FRotator(OrbitPitch, OrbitYaw, 0.f).Vector();
    PreviewViewport->SetViewLocation(Center + Direction * BriefingShipPreview::FitDistance(Radius, Size) * Zoom);
    PreviewViewport->SetViewRotation((-Direction).Rotation());
}

void UShipBriefingPreview::PlayRehearsal()
{
    if (RouteTime >= RehearsalDuration) RewindRehearsal();
    bRehearsalPlaying = RehearsalDuration > 0.0;
}

void UShipBriefingPreview::RewindRehearsal()
{
    RouteTime = 0.0;
    LastCaptionSecond = INDEX_NONE;
    UpdateRehearsal();
}

void UShipBriefingPreview::PauseRehearsal()
{
    bRehearsalPlaying = false;
}

void UShipBriefingPreview::StopRehearsal()
{
    PauseRehearsal();
    RewindRehearsal();
    ResetView();
}

void UShipBriefingPreview::UpdateRehearsal()
{
    AActor* Visual = RouteShip.Get();
    if (!Visual || Route.IsEmpty()) return;

    int32 Leg = 0;
    if (Route.Num() > 1)
    {
        while (Leg + 2 < Route.Num() && RouteTime >= RouteDepartures[Leg + 1]) ++Leg;
        const double Travel = RouteArrivals[Leg + 1] - RouteDepartures[Leg];
        const double Along = FMath::Clamp((RouteTime - RouteDepartures[Leg]) / Travel, 0.0, 1.0);
        Visual->SetActorLocationAndRotation(FMath::Lerp(Route[Leg], Route[Leg + 1], Along),
            (Route[Leg + 1] - Route[Leg]).Rotation());
    }

    const int32 Second = FMath::FloorToInt(RouteTime);
    if (RouteCaption && LastCaptionSecond != Second)
    {
        LastCaptionSecond = Second;
        const int32 LabelIndex = RouteArrivals.IsValidIndex(Leg + 1) && RouteTime >= RouteArrivals[Leg + 1]
            ? Leg + 1 : Leg;
        const TCHAR* Label = RouteLabels.IsValidIndex(LabelIndex) ? *RouteLabels[LabelIndex] : TEXT("MISSION COMPLETE");
        RouteCaption->SetText(FText::FromString(FString::Printf(TEXT("MISSION TIME %02d:%02d  |  %s"),
            Second / 60, Second % 60, Label)));
    }

    if (RouteChecklist)
    {
        for (int32 Index = 0; Index < RouteLabels.Num(); ++Index)
        {
            auto* Row = Cast<UTextBlock>(RouteChecklist->GetChildAt(Index + 1));
            if (!Row || !RouteArrivals.IsValidIndex(Index + 1)) continue;
            const bool bPassed = RouteTime >= RouteArrivals[Index + 1];
            Row->SetText(FText::FromString(FString(bPassed ? TEXT("\u2713  ") : TEXT("    ")) + RouteLabels[Index]));
            Row->SetColorAndOpacity(FSlateColor(bPassed ? FLinearColor(.28f, 1.f, .46f) : FLinearColor::White));
        }
    }

    if (EventPopup && EventPopupText)
    {
        const double PlaybackSeconds = FMath::Clamp(RehearsalDuration / 4.0, 15.0, 30.0);
        const double CueWindow = 2.5 * RehearsalDuration / PlaybackSeconds;
        FString Message;
        for (const auto& Cue : TimedCues)
        {
            const double Age = RouteTime - Cue.Time;
            if (Age < 0.0 || Age >= CueWindow) continue;
            if (!Message.IsEmpty()) Message += TEXT("\n");
            Message += Cue.Text;
        }
        EventPopupText->SetText(FText::FromString(Message));
        EventPopup->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
}

void UShipBriefingPreview::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (!bPreviewActive || !PreviewVisual.IsValid() || !PreviewViewport) return;
    if (bMissionScene && bRehearsalPlaying)
    {
        // Keep authored timing ratios while fitting the whole briefing into a short viewing window.
        const double PlaybackSeconds = FMath::Clamp(RehearsalDuration / 4.0, 15.0, 30.0);
        RouteTime = FMath::Min(RouteTime + DeltaTime * RehearsalDuration / PlaybackSeconds, RehearsalDuration);
        UpdateRehearsal();
        if (RouteTime >= RehearsalDuration) PauseRehearsal();
        if (!bDragging)
        {
            OrbitYaw = FMath::UnwindDegrees(OrbitYaw + DeltaTime * 3.f);
            UpdateCamera();
        }
    }
    else if (!LastViewSize.Equals(PreviewViewport->GetCachedGeometry().GetLocalSize(), 1.f))
        UpdateCamera();
}

FReply UShipBriefingPreview::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (bPreviewActive && PreviewVisual.IsValid() && Event.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bDragging = true;
        return FReply::Handled().CaptureMouse(TakeWidget());
    }
    return Super::NativeOnMouseButtonDown(Geometry, Event);
}

FReply UShipBriefingPreview::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (bDragging && Event.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(Geometry, Event);
}

FReply UShipBriefingPreview::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (bDragging && HasMouseCapture())
    {
        const FVector2D Delta = Event.GetCursorDelta();
        OrbitYaw = FMath::UnwindDegrees(OrbitYaw + Delta.X * 0.35f);
        OrbitPitch = FMath::Clamp(OrbitPitch + Delta.Y * 0.35f, -80.f, 80.f);
        UpdateCamera();
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(Geometry, Event);
}

FReply UShipBriefingPreview::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (bPreviewActive && PreviewVisual.IsValid())
    {
        Zoom = FMath::Clamp(Zoom * FMath::Pow(0.85f, Event.GetWheelDelta()), bMissionScene ? .30f : .70f, 3.f);
        UpdateCamera();
        return FReply::Handled();
    }
    return Super::NativeOnMouseWheel(Geometry, Event);
}

FReply UShipBriefingPreview::NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::LeftMouseButton && PreviewVisual.IsValid())
    {
        bDragging = false;
        ResetView();
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonDoubleClick(Geometry, Event);
}

void UShipBriefingPreview::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    bDragging = false;
    Super::NativeOnMouseCaptureLost(Event);
}
