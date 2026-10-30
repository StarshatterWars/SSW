#include "ShipBriefingPreview.h"
#include "GameStructs_System.h"
#include "ShipActor.h"
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
#include "Misc/PackageName.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "MissionUIStyle.h"

namespace
{
    void CollectTemplateParts(UClass* ActorClass, const FTransform& ParentTransform,
        TArray<FBriefingShipPreviewPart>& Parts, TSet<UClass*>& ClassStack)
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
                CollectTemplateParts(Child->GetChildActorClass(), Transform, Parts, ClassStack);
            }
        }
        if (ShipDefaults && Parts.IsEmpty() && ShipDefaults->ShipVisualBP)
            CollectTemplateParts(ShipDefaults->ShipVisualBP, ParentTransform, Parts, ClassStack);
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

bool BriefingShipPreview::ResolveParts(const FShipDesign& Design, TArray<FBriefingShipPreviewPart>& OutParts)
{
    OutParts.Empty();
    if (UStaticMesh* Mesh = Design.Mesh.LoadSynchronous())
    {
        FBriefingShipPreviewPart Part;
        Part.Mesh = Mesh;
        OutParts.Add(MoveTemp(Part));
    }
    if (OutParts.IsEmpty() && !Design.ShipName.IsEmpty())
    {
        const FString Package = FString::Printf(TEXT("/Game/Models/%s/BP_%s"), *Design.ShipName, *Design.ShipName);
        if (FPackageName::IsValidLongPackageName(Package) && FPackageName::DoesPackageExist(Package))
        {
            const FString ClassPath = Package + TEXT(".BP_") + Design.ShipName + TEXT("_C");
            if (UClass* VisualClass = LoadClass<AActor>(nullptr, *ClassPath))
            {
                TSet<UClass*> ClassStack;
                CollectTemplateParts(VisualClass, FTransform::Identity, OutParts, ClassStack);
            }
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

AActor* BriefingShipPreview::CreateVisual(UWorld* PreviewWorld, const TArray<FBriefingShipPreviewPart>& Parts)
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
    Visual->SetActorEnableCollision(false);
    Visual->SetActorTickEnabled(false);
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
}

void UShipBriefingPreview::NativeDestruct()
{
    ClearVisual();
    Parts.Empty();
    DesignKey.Empty();
    Super::NativeDestruct();
}

bool UShipBriefingPreview::SetShipDesign(const FShipDesign* Design)
{
    const FString NewKey = Design ? Design->ShipName + TEXT("|") + Design->Mesh.ToSoftObjectPath().ToString() : FString();
    if (DesignKey == NewKey && !Parts.IsEmpty()) { UpdateVisual(); return true; }
    ClearVisual();
    Parts.Empty();
    DesignKey = NewKey;
    if (!Design || !BriefingShipPreview::ResolveParts(*Design, Parts)) return false;
    ResetView();
    UpdateVisual();
    return true;
}

void UShipBriefingPreview::SetPreviewActive(bool bActive)
{
    bPreviewActive = bActive;
    SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bActive) UpdateVisual();
    else ClearVisual();
}

void UShipBriefingPreview::UpdateVisual()
{
    if (IsDesignTime() || !bPreviewActive || !PreviewViewport || !PreviewViewport->GetViewportWorld()) return;
    if (PreviewVisual.IsValid() && PreviewVisual->GetWorld() == PreviewViewport->GetViewportWorld()) return;
    ClearVisual();
    PreviewVisual = BriefingShipPreview::CreateVisual(PreviewViewport->GetViewportWorld(), Parts);
    if (UnavailableText) UnavailableText->SetVisibility(PreviewVisual.IsValid()
        ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    UpdateCamera();
}

void UShipBriefingPreview::ResetView()
{
    OrbitYaw = -130.f;
    OrbitPitch = 20.f;
    Zoom = 1.f;
    UpdateCamera();
}

void UShipBriefingPreview::UpdateCamera()
{
    if (!PreviewViewport || !PreviewViewport->GetViewportWorld() || Parts.IsEmpty()) return;
    FVector2D Size = PreviewViewport->GetCachedGeometry().GetLocalSize();
    if (Size.X <= 1.f || Size.Y <= 1.f) Size = GetCachedGeometry().GetLocalSize();
    LastViewSize = Size;
    const FBox Bounds = BriefingShipPreview::CalculateBounds(Parts);
    const FVector Center = Bounds.IsValid ? Bounds.GetCenter() : FVector::ZeroVector;
    const float Radius = Bounds.IsValid ? static_cast<float>(Bounds.GetExtent().Size()) : 100.f;
    const FVector Direction = FRotator(OrbitPitch, OrbitYaw, 0.f).Vector();
    PreviewViewport->SetViewLocation(Center + Direction * BriefingShipPreview::FitDistance(Radius, Size) * Zoom);
    PreviewViewport->SetViewRotation((-Direction).Rotation());
}

void UShipBriefingPreview::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (bPreviewActive && PreviewVisual.IsValid() && PreviewViewport &&
        !LastViewSize.Equals(PreviewViewport->GetCachedGeometry().GetLocalSize(), 1.f)) UpdateCamera();
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
        Zoom = FMath::Clamp(Zoom * FMath::Pow(0.85f, Event.GetWheelDelta()), 0.7f, 3.f);
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
