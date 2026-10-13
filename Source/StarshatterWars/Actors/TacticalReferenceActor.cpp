#include "TacticalReferenceActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
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
    Arm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Arm);

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
    if (!bCaptureMouse) return;

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
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
    if (DesignName.IsEmpty() || !GetWorld()) return;

    const FString BPPath = FString::Printf(
        TEXT("/Game/Models/%s/BP_%s.BP_%s_C"), *DesignName, *DesignName, *DesignName);
    if (UClass* BPClass = LoadClass<AActor>(nullptr, *BPPath))
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        if (AActor* Visual = GetWorld()->SpawnActor<AActor>(BPClass, GetActorTransform(), Params))
        {
            Visual->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
            if (ShipMesh) ShipMesh->SetVisibility(false);
            return;
        }
    }

    const FString MeshPath = FString::Printf(
        TEXT("/Game/Models/%s/%s.%s"), *DesignName, *DesignName, *DesignName);
    if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath))
        SetDisplayMesh(Mesh);
}

void ATacticalReferenceActor::SetDisplayMesh(UStaticMesh* Mesh)
{
    if (!ShipMesh || !Mesh) return;
    ShipMesh->SetStaticMesh(Mesh);
    ShipMesh->SetVisibility(true);
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
        Arm->SetRelativeRotation(FRotator(Pitch, Yaw, 0.f));
    }
}
