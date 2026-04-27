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
    Triangles.Reserve(Segments * 6);

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

    //  NO SCALING — geometry already matches radii
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
            MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase) &&
            !MeshName.Equals(TEXT("RuntimeRingDisc"), ESearchCase::IgnoreCase);

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
    const bool bEnableRings =
        !RingMaterialName.IsEmpty() &&
        InnerRingRadius > 0.0f &&
        OuterRingRadius > InnerRingRadius &&
        PlanetRadiusUnits > 0.0f;

    DebugInnerRadius = bEnableRings ? InnerRingRadius : 0.0f;
    DebugOuterRadius = bEnableRings ? OuterRingRadius : 0.0f;

    if (!bEnableRings)
    {
        if (RuntimeRingMesh)
        {
            RuntimeRingMesh->SetVisibility(false, true);
            RuntimeRingMesh->SetHiddenInGame(true, true);
            RuntimeRingMesh->ClearAllMeshSections();
        }

        HideLegacyBPRings();

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] RING DISABLED Actor=%s Inner=%.2f Outer=%.2f PlanetRadius=%.2f"),
            *GetName(),
            InnerRingRadius,
            OuterRingRadius,
            PlanetRadiusUnits);

        return;
    }

    HideLegacyBPRings();
    RebuildRuntimeRingMesh(InnerRingRadius, OuterRingRadius);

    if (RingDynamicMaterial)
    {
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Rings Opacity"), 255.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Density"), 6.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Dark Side Brightness"), 1.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Rings Shaded Brightness"), 1.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Rings Scattering Strength"), 3.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Scattering Size"), 3.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Scattering Power"), 3.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Shadow Strength"), 0.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Shadow Hardness"), 0.0f);
        RingDynamicMaterial->SetScalarParameterValue(TEXT("Position"), RingPosition);

        RingDynamicMaterial->SetVectorParameterValue(TEXT("Rings Color 1"), FLinearColor(1.0f, 0.85f, 0.55f, 1.0f));
        RingDynamicMaterial->SetVectorParameterValue(TEXT("Rings Color 2"), FLinearColor(1.0f, 0.75f, 0.40f, 1.0f));
        RingDynamicMaterial->SetVectorParameterValue(TEXT("Rings Color 3"), FLinearColor(1.0f, 0.95f, 0.70f, 1.0f));
        RingDynamicMaterial->SetVectorParameterValue(TEXT("Rings Scattering Color"), FLinearColor(1.0f, 0.85f, 0.55f, 1.0f));
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] ApplyRingRadiusSettings annulus Actor=%s Inner=%.2f Outer=%.2f"),
        *GetName(),
        InnerRingRadius,
        OuterRingRadius);
}

void AGasGiantActor::SetRingMaterialByName(const FString& InRingName)
{
    RingMaterialName = InRingName.TrimStartAndEnd();
    CurrentRingMaterial = nullptr;

    if (RingMaterialName.IsEmpty())
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

            const bool bIsRing =
                MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase) ||
                MeshName.Contains(TEXT("Rings"), ESearchCase::IgnoreCase) ||
                MeshComp->ComponentHasTag(TEXT("Ring"));

            if (bIsRing)
            {
                MeshComp->SetVisibility(false, true);
                MeshComp->SetHiddenInGame(true, true);
            }
        }

        OnRingMaterialChanged(nullptr);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Ring material cleared Actor=%s"),
            *GetName());

        return;
    }

    FString CleanRingName = RingMaterialName;

    const FString AssetName = CleanRingName.StartsWith(TEXT("MI_"))
        ? CleanRingName
        : FString(TEXT("MI_")) + CleanRingName;

    const FString Path = FString::Printf(
        TEXT("/Script/Engine.MaterialInstanceConstant'/Game/GameData/Galaxy/GasGiants/%s.%s'"),
        *AssetName,
        *AssetName);

    UMaterialInstance* LoadedMat =
        LoadObject<UMaterialInstance>(nullptr, *Path);

    if (!LoadedMat)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] FAILED Ring Material Actor=%s DEF='%s' Asset='%s' Path=%s"),
            *GetName(),
            *RingMaterialName,
            *AssetName,
            *Path);

        OnRingMaterialChanged(nullptr);
        return;
    }

    CurrentRingMaterial = LoadedMat;

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

        if (!bIsRing)
        {
            continue;
        }

        MeshComp->SetMaterial(0, CurrentRingMaterial);
        MeshComp->SetVisibility(true, true);
        MeshComp->SetHiddenInGame(false, true);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Ring mesh material SET Actor=%s Mesh=%s DEF='%s' Asset='%s' Mat=%s"),
            *GetName(),
            *MeshName,
            *RingMaterialName,
            *AssetName,
            *GetNameSafe(CurrentRingMaterial));
    }

    OnRingMaterialChanged(CurrentRingMaterial);
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

    UMaterialInterface* RingMat = CurrentRingMaterial;

    if (!RingMat)
    {
        RingMat = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Script/Engine.MaterialInstanceConstant'/Game/GameData/Galaxy/GasGiants/MI_Ring2.MI_Ring2'"));
    }

    if (RingMat)
    {
        RingDynamicMaterial = UMaterialInstanceDynamic::Create(RingMat, this);

        if (RingDynamicMaterial)
        {
            RuntimeRingMesh->SetMaterial(0, RingDynamicMaterial);
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] RuntimeRingMesh created Actor=%s Mat=%s MID=%s"),
        *GetName(),
        *GetNameSafe(RingMat),
        *GetNameSafe(RingDynamicMaterial));
}