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


    if (bDebugDrawRings &&
        DebugOuterRadius > DebugInnerRadius &&
        DebugOuterRadius > 0.0f)
    {
        DrawDebugRingOutline(DebugInnerRadius, DebugOuterRadius);
    }
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

    const float BaseMeshRadius = 50.0f;
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
            MeshName.Contains(TEXT("Rings"), ESearchCase::IgnoreCase);

        MeshComp->SetVisibility(true, true);
        MeshComp->SetHiddenInGame(false, true);
        MeshComp->SetCullDistance(0.0f);
        MeshComp->SetBoundsScale(10000.0f);
        MeshComp->SetMobility(EComponentMobility::Movable);

        if (bIsRing)
        {
            // Keep asset-pack ring transform for now.
            // Do not scale rings here.
            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] Ring preserved Mesh=%s Loc=%s Scale=%s Mat=%s"),
                *MeshName,
                *MeshComp->GetRelativeLocation().ToString(),
                *MeshComp->GetRelativeScale3D().ToString(),
                *GetNameSafe(MeshComp->GetMaterial(0)));

            continue;
        }

        MeshComp->SetRelativeScale3D(FVector(PlanetScale));

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Planet mesh scaled Mesh=%s Scale=%.2f"),
            *MeshName,
            PlanetScale);
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
        InnerRingRadius > 0.0f &&
        OuterRingRadius > InnerRingRadius &&
        PlanetRadiusUnits > 0.0f;

    DebugInnerRadius = bEnableRings ? InnerRingRadius : 0.0f;
    DebugOuterRadius = bEnableRings ? OuterRingRadius : 0.0f;

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

        if (!bEnableRings)
        {
            MeshComp->SetVisibility(false, true);
            MeshComp->SetHiddenInGame(true, true);

            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] RING DISABLED Actor=%s Mesh=%s"),
                *GetName(),
                *MeshName);

            continue;
        }

        MeshComp->SetVisibility(true, true);
        MeshComp->SetHiddenInGame(false, true);
        MeshComp->SetCullDistance(0.0f);
        MeshComp->SetBoundsScale(10000.0f);

        UMaterialInstanceDynamic* RingMID =
            Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(0));

        if (!RingMID)
        {
            UMaterialInterface* BaseMat =
                CurrentRingMaterial ? CurrentRingMaterial : MeshComp->GetMaterial(0);

            if (!BaseMat)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("[GasGiantActor] RING NO BASE MATERIAL Actor=%s Mesh=%s"),
                    *GetName(),
                    *MeshName);
                continue;
            }

            RingMID = UMaterialInstanceDynamic::Create(BaseMat, this);
            MeshComp->SetMaterial(0, RingMID);
        }

        const float InnerNorm =
            FMath::Clamp(InnerRingRadius / OuterRingRadius, 0.0f, 0.99f);

        const float OuterNorm = 1.0f;

        RingMID->SetScalarParameterValue(TEXT("Inner Edge"), InnerNorm);
        RingMID->SetScalarParameterValue(TEXT("Outer Edge"), OuterNorm);
        RingMID->SetScalarParameterValue(TEXT("Density"), 1.0f);

        RingMID->SetScalarParameterValue(TEXT("Rings Opacity"), 128.0f);
        RingMID->SetScalarParameterValue(TEXT("Edge Hardness"), 2.0f);
        RingMID->SetScalarParameterValue(TEXT("Frequency"), 2.0f);
        RingMID->SetScalarParameterValue(TEXT("Frequency 2"), 0.1f);
        RingMID->SetScalarParameterValue(TEXT("Position"), RingPosition);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] FINAL RING FIX Actor=%s Mesh=%s InnerNorm=%.3f OuterNorm=%.3f Mat=%s"),
            *GetName(),
            *MeshName,
            InnerNorm,
            OuterNorm,
            *GetNameSafe(RingMID));
    }
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

void AGasGiantActor::ForceDebugRingMesh()
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

        if (!bIsRing)
        {
            continue;
        }

        MeshComp->SetVisibility(true, true);
        MeshComp->SetHiddenInGame(false, true);
        MeshComp->SetRenderInMainPass(true);
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MeshComp->SetCullDistance(0.0f);
        MeshComp->SetBoundsScale(10000.0f);
        MeshComp->SetCastShadow(false);

        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
        MeshComp->SetRelativeRotation(FRotator::ZeroRotator);
        MeshComp->SetRelativeScale3D(FVector(5.0f, 5.0f, 5.0f));

        UMaterialInterface* DebugMat =
            LoadObject<UMaterialInterface>(
                nullptr,
                TEXT("/Script/Engine.Material'/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial'"));

        if (DebugMat)
        {
            MeshComp->SetMaterial(0, DebugMat);
        }

        DrawDebugBox(
            GetWorld(),
            MeshComp->Bounds.Origin,
            MeshComp->Bounds.BoxExtent,
            FColor::Cyan,
            false,
            0.0f,
            0,
            10.0f);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] FORCE RING DEBUG Actor=%s Mesh=%s Visible=%d Hidden=%d Loc=%s Scale=%s WorldScale=%s Mat=%s BoundsOrigin=%s BoundsExtent=%s"),
            *GetName(),
            *MeshName,
            MeshComp->IsVisible() ? 1 : 0,
            MeshComp->bHiddenInGame ? 1 : 0,
            *MeshComp->GetRelativeLocation().ToString(),
            *MeshComp->GetRelativeScale3D().ToString(),
            *MeshComp->GetComponentScale().ToString(),
            *GetNameSafe(MeshComp->GetMaterial(0)),
            *MeshComp->Bounds.Origin.ToString(),
            *MeshComp->Bounds.BoxExtent.ToString());

        return;
    }
}
