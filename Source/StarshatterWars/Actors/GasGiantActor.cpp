#include "GasGiantActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

#include "UObject/UnrealType.h"
#include "DrawDebugHelpers.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

static void SetBPObjectPropertyByName(UObject* Object, const FString& WantedName, UObject* Value)
{
    if (!Object)
    {
        return;
    }

    const FString WantedCompact =
        WantedName.Replace(TEXT(" "), TEXT(""))
        .Replace(TEXT("_"), TEXT(""));

    for (TFieldIterator<FObjectProperty> It(Object->GetClass()); It; ++It)
    {
        FObjectProperty* Prop = *It;

        if (!Prop)
        {
            continue;
        }

        const FString InternalName = Prop->GetName();
        const FString DisplayName = Prop->GetMetaData(TEXT("DisplayName"));

        const FString InternalCompact =
            InternalName.Replace(TEXT(" "), TEXT(""))
            .Replace(TEXT("_"), TEXT(""));

        const FString DisplayCompact =
            DisplayName.Replace(TEXT(" "), TEXT(""))
            .Replace(TEXT("_"), TEXT(""));

        if (InternalName.Equals(WantedName, ESearchCase::IgnoreCase) ||
            DisplayName.Equals(WantedName, ESearchCase::IgnoreCase) ||
            InternalCompact.Equals(WantedCompact, ESearchCase::IgnoreCase) ||
            DisplayCompact.Equals(WantedCompact, ESearchCase::IgnoreCase))
        {
            Prop->SetObjectPropertyValue_InContainer(Object, Value);

            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] Set BP object Wanted='%s' Internal='%s' Display='%s' Value=%s"),
                *WantedName,
                *InternalName,
                *DisplayName,
                *GetNameSafe(Value));

            return;
        }
    }

    UE_LOG(LogTemp, Error,
        TEXT("[GasGiantActor] BP object property not found: %s"),
        *WantedName);
}

static void SetBPFloatProperty(UObject* Object, const FName PropertyName, float Value)
{
    if (!Object)
    {
        return;
    }

    FFloatProperty* FloatProp =
        FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName);

    if (!FloatProp)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] BP float property not found: %s"),
            *PropertyName.ToString());
        return;
    }

    FloatProp->SetPropertyValue_InContainer(Object, Value);

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] Set BP float %s = %.3f"),
        *PropertyName.ToString(),
        Value);
}

AGasGiantActor::AGasGiantActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
    CoreMesh->SetupAttachment(SceneRoot);
    CoreMesh->SetMobility(EComponentMobility::Movable);
    CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CoreMesh->SetGenerateOverlapEvents(false);
    CoreMesh->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    if (SphereMeshFinder.Succeeded())
    {
        CoreMesh->SetStaticMesh(SphereMeshFinder.Object);
    }
}

void AGasGiantActor::BeginPlay()
{
    Super::BeginPlay();
    InitialActorScale = GetActorScale3D();
    CreateMID();
    if (bForceRings || !RingMaterialName.IsEmpty()) SetRingMaterialByName(RingMaterialName);

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] BeginPlay Actor=%s"),
        *GetName());
}

void AGasGiantActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    /*if (bDebugDrawRings &&
        DebugOuterRadius > DebugInnerRadius &&
        DebugOuterRadius > 0.0f)
    {
        DrawDebugRingOutline(DebugInnerRadius, DebugOuterRadius);
    }*/
}

void AGasGiantActor::CreateMID()
{
    if (!CoreMesh)
    {
        return;
    }

    UMaterialInterface* BaseMat = CoreMesh->GetMaterial(0);

    if (BaseMat)
    {
        DynamicMaterial = CoreMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
}

void AGasGiantActor::SetPlanetRadius(float InRadiusUnits)
{
    PlanetRadiusUnits = FMath::Max(InRadiusUnits, 1.0f);

    const float BaseMeshRadius =
        GasGiantBaseMeshRadius > KINDA_SMALL_NUMBER
        ? GasGiantBaseMeshRadius
        : 50.0f;

    const float PlanetScale = PlanetRadiusUnits / BaseMeshRadius;

    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        const FString MeshName = MeshComp->GetName();

        const bool bIsRing =
            MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase) ||
            MeshName.Contains(TEXT("Rings"), ESearchCase::IgnoreCase) ||
            MeshComp->ComponentHasTag(TEXT("Ring"));

        const bool bIsBPRoot =
            MeshName.Equals(TEXT("root"), ESearchCase::IgnoreCase) ||
            MeshName.Equals(TEXT("SceneRoot"), ESearchCase::IgnoreCase);

        MeshComp->SetCullDistance(0.0f);
        MeshComp->SetBoundsScale(10000.0f);
        MeshComp->SetMobility(EComponentMobility::Movable);

        if (bIsBPRoot)
        {
            MeshComp->SetRelativeScale3D(FVector::OneVector);

            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] BP root NOT scaled Mesh=%s Scale=%s"),
                *MeshName,
                *MeshComp->GetRelativeScale3D().ToString());

            continue;
        }

        if (bIsRing)
        {
            MeshComp->SetVisibility(true, true);
            MeshComp->SetHiddenInGame(false, true);

            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] Ring NOT scaled Mesh=%s RelScale=%s WorldScale=%s Mat=%s"),
                *MeshName,
                *MeshComp->GetRelativeScale3D().ToString(),
                *MeshComp->GetComponentScale().ToString(),
                *GetNameSafe(MeshComp->GetMaterial(0)));

            continue;
        }

        const bool bIsPlanetVisual =
            MeshName.Equals(TEXT("CoreMesh"), ESearchCase::IgnoreCase) ||
            MeshName.Equals(TEXT("Planet"), ESearchCase::IgnoreCase) ||
            MeshName.Equals(TEXT("Atmosphere"), ESearchCase::IgnoreCase);

        if (!bIsPlanetVisual)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] Mesh skipped Mesh=%s Scale=%s"),
                *MeshName,
                *MeshComp->GetRelativeScale3D().ToString());

            continue;
        }

        MeshComp->SetVisibility(true, true);
        MeshComp->SetHiddenInGame(false, true);
        MeshComp->SetRelativeScale3D(FVector(PlanetScale));

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Planet visual scaled Mesh=%s PlanetScale=%.2f RelScale=%s WorldScale=%s"),
            *MeshName,
            PlanetScale,
            *MeshComp->GetRelativeScale3D().ToString(),
            *MeshComp->GetComponentScale().ToString());
    }

    OnGasGiantRadiusChanged();
    if (!RingMaterialName.IsEmpty()) ApplyRingRadiusSettings();
}
float AGasGiantActor::GetPlanetRadius() const
{
    return PlanetRadiusUnits;
}

void AGasGiantActor::SetLightDirection(const FVector& InDirection)
{
    LightDirection = InDirection.IsNearlyZero()
        ? FVector(1, 0, 0)
        : InDirection.GetSafeNormal();

    if (DynamicMaterial)
    {
        DynamicMaterial->SetVectorParameterValue(
            TEXT("LightDirection"),
            FLinearColor(
                LightDirection.X,
                LightDirection.Y,
                LightDirection.Z));
    }
}

void AGasGiantActor::SetGasGiantMaterialByName(const FString& MaterialName)
{
    const FString CleanName = MaterialName.TrimStartAndEnd();

    if (CleanName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Empty material name Actor=%s"),
            *GetName());
        return;
    }

    // ----------------------------------------------------
    // BUILD MATERIAL PATH
    // ----------------------------------------------------
    const FString AssetName = CleanName.StartsWith(TEXT("MI_"))
        ? CleanName
        : FString(TEXT("MI_")) + CleanName;

    const FString Path = FString::Printf(
        TEXT("/Script/Engine.MaterialInterface'%s%s.%s'"),
        *MaterialBasePath,
        *AssetName,
        *AssetName);

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] Loading Material Path=%s"),
        *Path);

    UMaterialInterface* Mat =
        LoadObject<UMaterialInterface>(nullptr, *Path);

    if (!Mat)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] FAILED to load material '%s' Actor=%s"),
            *AssetName,
            *GetName());
        return;
    }

    // ----------------------------------------------------
    // SET BP VARIABLE (safe to keep)
    // ----------------------------------------------------
    FObjectProperty* MaterialProperty =
        FindFProperty<FObjectProperty>(GetClass(), TEXT("Planet Material"));

    if (MaterialProperty)
    {
        MaterialProperty->SetObjectPropertyValue_InContainer(this, Mat);
    }

    // ----------------------------------------------------
    // COMMON VALUES
    // ----------------------------------------------------
    const float BaseMeshRadius = 50.0f;
    const float NormalizedRadius = PlanetRadiusUnits / BaseMeshRadius;
    const FVector PlanetCenter = GetActorLocation();

    // ----------------------------------------------------
    // APPLY MATERIALS
    // ----------------------------------------------------
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        const FString MeshName = MeshComp->GetName();

        const bool bIsRing =
            MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase) ||
            MeshName.Contains(TEXT("Rings"), ESearchCase::IgnoreCase);

        // ==================================================
        // RINGS (FIXED: no MID stacking)
        // ==================================================
        if (bIsRing)
        {
            MeshComp->SetVisibility(true, true);
            MeshComp->SetHiddenInGame(false, true);
            MeshComp->SetCullDistance(0.0f);
            MeshComp->SetBoundsScale(10000.0f);

            // DO NOT touch material here
            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] Ring preserved Mesh=%s Mat=%s"),
                *MeshName,
                *GetNameSafe(MeshComp->GetMaterial(0)));

            continue;
        }

        // ==================================================
        // PLANET
        // ==================================================
        UMaterialInstanceDynamic* MID =
            UMaterialInstanceDynamic::Create(Mat, this);

        MeshComp->SetMaterial(0, MID);

        if (MID)
        {
            MID->SetScalarParameterValue(
                TEXT("PlanetRadius"),
                NormalizedRadius);

            MID->SetVectorParameterValue(
                TEXT("PlanetCenterWS"),
                FLinearColor(
                    PlanetCenter.X,
                    PlanetCenter.Y,
                    PlanetCenter.Z,
                    0.0f));

            MID->SetVectorParameterValue(
                TEXT("LightDirection"),
                FLinearColor(
                    LightDirection.X,
                    LightDirection.Y,
                    LightDirection.Z,
                    0.0f));

            MID->SetScalarParameterValue(TEXT("LightIntensity"), 1.0f);

            DynamicMaterial = MID;

            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] PlanetMID applied Mesh=%s"),
                *MeshName);
        }
    }

    OnGasGiantMaterialChanged(DynamicMaterial);
    if (!RingMaterialName.IsEmpty()) ApplyRingRadiusSettings();
}

void AGasGiantActor::RebuildRuntimeRingMesh(float InnerRadius, float OuterRadius)
{
    EnsureRuntimeRingMesh();

    if (!RuntimeRingMesh)
    {
        return;
    }

    RuntimeRingMesh->ClearAllMeshSections();

    if (InnerRadius <= 0.0f || OuterRadius <= InnerRadius)
    {
        RuntimeRingMesh->SetVisibility(false, true);
        RuntimeRingMesh->SetHiddenInGame(true, true);
        return;
    }

    const int32 Segments = 192;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<FLinearColor> Colors;

    Vertices.Reserve((Segments + 1) * 2);
    Normals.Reserve((Segments + 1) * 2);
    UVs.Reserve((Segments + 1) * 2);
    Tangents.Reserve((Segments + 1) * 2);
    Colors.Reserve((Segments + 1) * 2);
    Triangles.Reserve(Segments * 12);

    for (int32 i = 0; i <= Segments; ++i)
    {
        const float Alpha = (float)i / (float)Segments;
        const float Angle = Alpha * TWO_PI;

        const float CosA = FMath::Cos(Angle);
        const float SinA = FMath::Sin(Angle);

        const FVector InnerPos(InnerRadius * CosA, InnerRadius * SinA, 0.0f);
        const FVector OuterPos(OuterRadius * CosA, OuterRadius * SinA, 0.0f);

        Vertices.Add(InnerPos);
        Vertices.Add(OuterPos);

        Normals.Add(FVector::UpVector);
        Normals.Add(FVector::UpVector);

        // CORRECT RADIAL UV MAPPING (THIS WAS THE PROBLEM)
        const float OuterU = 0.5f + 0.5f * CosA;
        const float OuterV = 0.5f + 0.5f * SinA;

        const float InnerRatio = InnerRadius / OuterRadius;

        const float InnerU = 0.5f + 0.5f * InnerRatio * CosA;
        const float InnerV = 0.5f + 0.5f * InnerRatio * SinA;

        UVs.Add(FVector2D(InnerU, InnerV));
        UVs.Add(FVector2D(OuterU, OuterV));

        Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

        Colors.Add(FLinearColor::White);
        Colors.Add(FLinearColor::White);
    }

    for (int32 i = 0; i < Segments; ++i)
    {
        const int32 Inner0 = i * 2;
        const int32 Outer0 = Inner0 + 1;
        const int32 Inner1 = Inner0 + 2;
        const int32 Outer1 = Inner0 + 3;

        Triangles.Add(Inner0);
        Triangles.Add(Outer1);
        Triangles.Add(Outer0);

        Triangles.Add(Inner0);
        Triangles.Add(Inner1);
        Triangles.Add(Outer1);

        // Reverse winding gives the flat annulus a visible underside even
        // when the material itself is one-sided.
        Triangles.Add(Outer0);
        Triangles.Add(Outer1);
        Triangles.Add(Inner0);
        Triangles.Add(Outer1);
        Triangles.Add(Inner1);
        Triangles.Add(Inner0);
    }

    RuntimeRingMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        Colors,
        Tangents,
        false);

    //  NO SCALING � geometry already matches radii
    RuntimeRingMesh->SetRelativeLocation(FVector::ZeroVector);
    RuntimeRingMesh->SetRelativeRotation(FRotator::ZeroRotator);
    RuntimeRingMesh->SetRelativeScale3D(FVector(1.0f));

    RuntimeRingMesh->SetVisibility(true, true);
    RuntimeRingMesh->SetHiddenInGame(false, true);

    if (RingDynamicMaterial)
    {
        RuntimeRingMesh->SetMaterial(0, RingDynamicMaterial);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] Runtime annulus rebuilt Actor=%s Inner=%.2f Outer=%.2f Segments=%d"),
        *GetName(),
        InnerRadius,
        OuterRadius,
        Segments);
}

void AGasGiantActor::HideLegacyBPRings()
{
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        const FString MeshName = MeshComp->GetName();

        const bool bIsLegacyRing =
            MeshComp->ComponentHasTag(TEXT("Ring")) ||
            MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase) ||
            (MeshComp->GetStaticMesh() && MeshComp->GetStaticMesh()->GetName().Equals(TEXT("Plane")));

        if (bIsLegacyRing)
        {
            MeshComp->SetVisibility(false, true);
            MeshComp->SetHiddenInGame(true, true);

            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] Legacy BP ring hidden Mesh=%s"),
                *MeshName);
        }
    }
}

void AGasGiantActor::SetBandSpeed(float InSpeed)
{
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("BandSpeed"), InSpeed);
    }
}

void AGasGiantActor::SetStormIntensity(float InIntensity)
{
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("StormIntensity"), InIntensity);
    }
}

void AGasGiantActor::UpdateRingParameters(float PlanetScale)
{
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        if (!MeshComp->GetName().Contains(TEXT("Ring"), ESearchCase::IgnoreCase))
        {
            continue;
        }

        UMaterialInstanceDynamic* MID =
            MeshComp->CreateAndSetMaterialInstanceDynamic(0);

        if (!MID)
        {
            continue;
        }

        const float Inner = PlanetScale * 1.2f;
        const float Outer = PlanetScale * 2.0f;

        MID->SetScalarParameterValue(TEXT("Inner_Radius"), Inner);
        MID->SetScalarParameterValue(TEXT("Outer_Radius"), Outer);

        MID->SetScalarParameterValue(TEXT("Position"), 0.0f);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] RingParams Inner=%.2f Outer=%.2f Scale=%.2f"),
            Inner, Outer, PlanetScale);
    }
}

void AGasGiantActor::ApplyRingRadiusSettings()
{
    if (bForceRings)
    {
        if (RingMaterialName.TrimStartAndEnd().IsEmpty()) RingMaterialName = TEXT("Ring2");
        if (!CurrentRingMaterial)
            CurrentRingMaterial = Cast<UMaterialInstance>(LoadRingMaterialByName(RingMaterialName));
        const float InnerRatio = FMath::IsFinite(ForcedInnerRingRatio) ? FMath::Max(1.01f, ForcedInnerRingRatio) : 1.2f;
        const float OuterRatio = FMath::IsFinite(ForcedOuterRingRatio) ? FMath::Max(InnerRatio + 0.01f, ForcedOuterRingRatio) : InnerRatio + 0.7f;
        InnerRingRadius = FMath::Max(PlanetRadiusUnits, 1.0f) * InnerRatio;
        OuterRingRadius = FMath::Max(PlanetRadiusUnits, 1.0f) * OuterRatio;
    }

    const bool bEnabled = !RingMaterialName.TrimStartAndEnd().IsEmpty() &&
        FMath::IsFinite(InnerRingRadius) && FMath::IsFinite(OuterRingRadius) &&
        InnerRingRadius > 0 && OuterRingRadius > InnerRingRadius && PlanetRadiusUnits > 0;
    DebugInnerRadius = bEnabled ? InnerRingRadius : 0;
    DebugOuterRadius = bEnabled ? OuterRingRadius : 0;
    if (!bEnabled)
    {
        HideLegacyBPRings();
        if (RuntimeRingMesh)
        {
            RuntimeRingMesh->ClearAllMeshSections();
            RuntimeRingMesh->SetVisibility(false);
            RuntimeRingMesh->SetHiddenInGame(true);
        }
        return;
    }

    // DEF radii are scene units. BP_GasGiant can apply additional scale to its
    // Planet component. Measure geometry, not inflated culling bounds.
    UStaticMeshComponent* PlanetVisual = CoreMesh;
    FQuat RingRotation = GetActorQuat();
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        if (!IsValid(Mesh) || !Mesh->GetStaticMesh()) continue;
        if (Mesh->GetName().Equals(TEXT("Planet"), ESearchCase::IgnoreCase)) PlanetVisual = Mesh;
        if (Mesh->ComponentHasTag(TEXT("Ring")) || Mesh->GetName().Contains(TEXT("Ring")))
            RingRotation = Mesh->GetComponentQuat();
    }
    double VisualRadius = PlanetRadiusUnits;
    FVector Center = GetActorLocation();
    if (PlanetVisual && PlanetVisual->GetStaticMesh())
    {
        const FBoxSphereBounds LocalBounds = PlanetVisual->GetStaticMesh()->GetBounds();
        VisualRadius = FMath::Max(VisualRadius,
            LocalBounds.SphereRadius * PlanetVisual->GetComponentScale().GetAbsMax());
        Center = PlanetVisual->GetComponentTransform().TransformPosition(LocalBounds.Origin);
    }
    const double RadiusScale = VisualRadius / PlanetRadiusUnits;
    const float Inner = float(InnerRingRadius * RadiusScale);
    const float Outer = float(OuterRingRadius * RadiusScale);
    RebuildRuntimeRingMesh(Inner, Outer);
    if (!RuntimeRingMesh) return;

    // Cancel the CURRENT parent scale via SetWorldScale3D, then inherit future
    // changes. SystemSceneBuilder rescales the entire planet for mission and
    // cutscene projection; an absolute-scale ring would remain inside its hull.
    RuntimeRingMesh->SetAbsolute(false, false, false);
    RuntimeRingMesh->SetWorldScale3D(FVector::OneVector);
    RuntimeRingMesh->SetWorldRotation(RingRotation);
    RuntimeRingMesh->SetWorldLocation(Center + RingRotation.RotateVector(FVector(0, 0, RingPosition)));
    HideLegacyBPRings();

    UMaterialInterface* Material = RingBaseMaterial ? RingBaseMaterial.Get() : CurrentRingMaterial;
    if (Material)
    {
        if (!RingDynamicMaterial || RingDynamicMaterial->Parent != Material)
            RingDynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
        if (RingDynamicMaterial)
        {
            // M_Rings_Master uses round(radialUV * Edge), not world radii.
            // Radial UV is 0.5 at the outer edge, Inner/(2*Outer) at the inner.
            // Slightly extend the shader mask; geometry defines the exact edges.
            RingDynamicMaterial->SetScalarParameterValue(TEXT("Outer Edge"), 0.99f);
            RingDynamicMaterial->SetScalarParameterValue(TEXT("Inner Edge"), (Outer / Inner) * 1.01f);
            if (bForceRings)
                RingDynamicMaterial->SetScalarParameterValue(TEXT("RingsDensity"), 1.0f);
            RuntimeRingMesh->SetMaterial(0, RingDynamicMaterial);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GasGiantRing] Missing material '%s' on %s"), *RingMaterialName, *GetName());
    }
    RuntimeRingMesh->SetBoundsScale(1.0f);
    RuntimeRingMesh->SetVisibility(true);
    RuntimeRingMesh->SetHiddenInGame(false);
    UE_LOG(LogTemp, Display,
        TEXT("[GasGiantRing] Actor=%s PlanetRadius=%.1f Inner=%.1f Outer=%.1f Material=%s WorldScale=%s"),
        *GetName(), VisualRadius, Inner, Outer, *GetNameSafe(Material), *RuntimeRingMesh->GetComponentScale().ToString());
}

void AGasGiantActor::SetRingMaterialByName(const FString& InRingName)
{
    RingMaterialName = InRingName.TrimStartAndEnd();
    if (bForceRings && RingMaterialName.IsEmpty()) RingMaterialName = TEXT("Ring2");
    CurrentRingMaterial = Cast<UMaterialInstance>(LoadRingMaterialByName(RingMaterialName));
    RingDynamicMaterial = nullptr;
    // Allow BP notifications first. The C++ annulus is the final ring state.
    OnRingMaterialChanged(CurrentRingMaterial);
    ApplyRingRadiusSettings();
}

UMaterialInterface* AGasGiantActor::LoadRingMaterialByName(const FString& RingName)
{
    const FString CleanName = RingName.TrimStartAndEnd();

    if (CleanName.IsEmpty())
    {
        return nullptr;
    }

    const FString AssetName = CleanName.StartsWith(TEXT("MI_"))
        ? CleanName
        : RingMaterialPrefix + CleanName;

    const FString Path = FString::Printf(
        TEXT("/Script/Engine.MaterialInterface'%s%s.%s'"),
        *RingMaterialBasePath,
        *AssetName,
        *AssetName);

    return LoadObject<UMaterialInterface>(nullptr, *Path);
}

void AGasGiantActor::DrawDebugRingOutline(float InnerRadius, float OuterRadius) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (InnerRadius <= 0.0f || OuterRadius <= InnerRadius)
    {
        return;
    }

    const FVector Center = GetActorLocation();

    const FVector AxisX = GetActorRightVector();
    const FVector AxisY = GetActorForwardVector();

    const int32 Segments = 128;
    const float Lifetime = 0.0f;
    const float Thickness = 8.0f;

    DrawDebugSphere(
        World,
        Center,
        50.0f,
        16,
        FColor::Yellow,
        false,
        Lifetime,
        0,
        4.0f);

    DrawDebugCircle(
        World,
        Center,
        OuterRadius,
        Segments,
        FColor::Red,
        false,
        Lifetime,
        0,
        Thickness,
        AxisX,
        AxisY,
        false);

    DrawDebugCircle(
        World,
        Center,
        InnerRadius,
        Segments,
        FColor::Green,
        false,
        Lifetime,
        0,
        Thickness,
        AxisX,
        AxisY,
        false);
}

void AGasGiantActor::EnsureRuntimeRingMesh()
{
    if (RuntimeRingMesh)
    {
        return;
    }

    USceneComponent* AttachParent = GetRootComponent();

    if (!AttachParent)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] EnsureRuntimeRingMesh: no root Actor=%s"),
            *GetName());
        return;
    }

    RuntimeRingMesh = NewObject<UProceduralMeshComponent>(
        this,
        UProceduralMeshComponent::StaticClass(),
        TEXT("RuntimeRingMesh"));

    if (!RuntimeRingMesh)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] EnsureRuntimeRingMesh: NewObject failed Actor=%s"),
            *GetName());
        return;
    }

    RuntimeRingMesh->SetupAttachment(AttachParent);
    RuntimeRingMesh->RegisterComponent();

    RuntimeRingMesh->SetMobility(EComponentMobility::Movable);
    RuntimeRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RuntimeRingMesh->SetGenerateOverlapEvents(false);
    RuntimeRingMesh->SetCastShadow(false);
    RuntimeRingMesh->SetVisibility(false, true);
    RuntimeRingMesh->SetHiddenInGame(true, true);
    RuntimeRingMesh->bUseAsyncCooking = true;

    RuntimeRingMesh->SetAbsolute(false, false, false);
    RuntimeRingMesh->SetCanEverAffectNavigation(false);
    RuntimeRingMesh->SetCullDistance(0.0f);
}
