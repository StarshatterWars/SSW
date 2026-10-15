
#include "TacticalReferenceActor.h"
#include "GameStructs_System.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/StaticMesh.h"

ATacticalReferenceActor::ATacticalReferenceActor()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoReceiveInput = EAutoReceiveInput::Player0;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
    ShipMesh->SetupAttachment(Root);
    ShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
    Arm->SetupAttachment(Root);
    Arm->TargetArmLength = Distance;
    CenterInPanel();
    Arm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Arm);

    MessageText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MessageText"));
    MessageText->SetupAttachment(Root);
    MessageText->SetText(FText::FromString(TEXT("Not Available")));
    MessageText->SetTextRenderColor(FColor(180, 200, 220));
    MessageText->SetHorizontalAlignment(EHTA_Center);
    MessageText->SetVerticalAlignment(EVRTA_TextCenter);
    MessageText->SetWorldSize(48.f);
    MessageText->SetVisibility(false);

    CloseText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CloseText"));
    CloseText->SetupAttachment(Root);
    CloseText->SetText(FText::FromString(TEXT("[ Close ]")));
    CloseText->SetTextRenderColor(FColor(220, 160, 120));
    CloseText->SetHorizontalAlignment(EHTA_Center);
    CloseText->SetVerticalAlignment(EVRTA_TextCenter);
    CloseText->SetWorldSize(32.f);
    CloseText->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
    CloseText->SetVisibility(false);

    KeyLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("KeyLight"));
    KeyLight->SetupAttachment(Root);
    KeyLight->SetIntensity(8.f);
    KeyLight->SetLightColor(FLinearColor(0.9f, 0.95f, 1.f));
    KeyLight->SetRelativeRotation(FRotator(-40.f, 30.f, 0.f));

    ApplyView();
}

void ATacticalReferenceActor::BeginPlay()
{
    Super::BeginPlay();
    ApplyView();
}

void ATacticalReferenceActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (MessageText && MessageText->IsVisible() && PC && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
        ShowUnavailableMessage(false);

    if (!bCaptureMouse) return;
    if (!PC) return;

    float MX = 0.f, MY = 0.f;
    PC->GetInputMouseDelta(MX, MY);
    if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
        AddOrbitDelta(MX, -MY);

    const float Wheel = PC->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
    if (!FMath::IsNearlyZero(Wheel))
        AddZoomDelta(-Wheel);
}

void ATacticalReferenceActor::ShowShip(const FString& DesignName)
{
    if (SpawnedVisual) { SpawnedVisual->Destroy(); SpawnedVisual = nullptr; }
    if (ShipMesh) { ShipMesh->SetStaticMesh(nullptr); ShipMesh->SetVisibility(false); }
    if (DesignName.IsEmpty() || !GetWorld()) return;

    const FString BPPath = FString::Printf(
        TEXT("/Game/Models/%s/BP_%s.BP_%s_C"), *DesignName, *DesignName, *DesignName);
    if (UClass* BPClass = LoadClass<AActor>(nullptr, *BPPath))
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        if (AActor* Visual = GetWorld()->SpawnActor<AActor>(BPClass, GetActorTransform(), Params))
        {
            SpawnedVisual = Visual;
            Visual->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
            if (ShipMesh) ShipMesh->SetVisibility(false);
            return;
        }
    }

    const FString MeshPath = FString::Printf(
        TEXT("/Game/Models/%s/%s.%s"), *DesignName, *DesignName, *DesignName);
    if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath))
    {
        SetDisplayMesh(Mesh);
        return;
    }
    ShowUnavailableMessage(true);
}

void ATacticalReferenceActor::SetDisplayMesh(UStaticMesh* Mesh)
{
    if (SpawnedVisual) { SpawnedVisual->Destroy(); SpawnedVisual = nullptr; }
    if (!ShipMesh) return;
    ShipMesh->SetStaticMesh(nullptr);
    ShipMesh->SetVisibility(false);
    if (!Mesh) return;
    ShipMesh->SetStaticMesh(Mesh);
    ShipMesh->SetVisibility(true);
    ShowUnavailableMessage(false);
}

void ATacticalReferenceActor::AddOrbitDelta(float YawDelta, float PitchDelta)
{
    Yaw = FMath::UnwindDegrees(Yaw + YawDelta * OrbitSpeed);
    Pitch = FMath::Clamp(Pitch + PitchDelta * OrbitSpeed, -80.f, 80.f);
    ApplyView();
}

void ATacticalReferenceActor::AddZoomDelta(float Delta)
{
    Distance = FMath::Clamp(Distance + Delta * ZoomSpeed, MinDistance, MaxDistance);
    ApplyView();
}

void ATacticalReferenceActor::ResetView()
{
    Yaw = 180.f;
    Pitch = 15.f;
    Distance = 800.f;
    ApplyView();
}

void ATacticalReferenceActor::ApplyView()
{
    if (Arm)
    {
        Arm->TargetArmLength = Distance;
    CenterInPanel();
        Arm->SetRelativeRotation(FRotator(Pitch, Yaw, 0.f));
    }
}

void ATacticalReferenceActor::ShowShipDesign(const FShipDesign& Design)
{
    if (Design.Mesh.IsNull())
    {
        ShowShip(Design.ShipName);
        return;
    }
    if (UStaticMesh* Mesh = Design.Mesh.LoadSynchronous())
        SetDisplayMesh(Mesh);
    else
        ShowShip(Design.ShipName);
}

void ATacticalReferenceActor::CenterInPanel()
{
    if (!Camera) return;
    // Panel cutout center as a fraction of a 1920x1080 frame.
    const float CutoutCenterX = (32.f + 1024.f * 0.5f) / 1920.f; // 0.2833
    const float CutoutCenterY = (285.f + 640.f * 0.5f) / 1080.f; // 0.5602
    const float FOV = Camera->FieldOfView;
    const float HalfW = Distance * FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f));
    const float HalfH = HalfW * (1080.f / 1920.f);
    // Offset the camera so the ship (at origin) lands at the cutout center.
    const float OffX = (0.5f - CutoutCenterX) * 2.f * HalfW;
    const float OffY = (CutoutCenterY - 0.5f) * 2.f * HalfH;
    Camera->SetRelativeLocation(FVector(0.f, OffX, OffY));
}

void ATacticalReferenceActor::ShowUnavailableMessage(bool bShow)
{
    if (MessageText) MessageText->SetVisibility(bShow);
    if (CloseText)
    {
        CloseText->SetVisibility(bShow);
        if (bShow) CloseText->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
    }
    if (!MessageText) return;
    MessageText->SetVisibility(bShow);
    if (bShow)
        MessageText->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
}
