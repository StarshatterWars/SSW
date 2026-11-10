#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaypointActor.generated.h"

// Noncolliding 3D navigation marker. Legacy instructions own arrival and progress.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AWaypointActor : public AActor
{
    GENERATED_BODY()
public:
    AWaypointActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    void RefreshAppearance(bool bVisible);
    FString CompactLabel() const;
    static void BuildPlumbob(TArray<FVector>& Vertices,TArray<int32>& Triangles,TArray<FVector>& Normals,TArray<FVector2D>& UVs,TArray<FLinearColor>& Colors,double Height,double Width);
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Navigation|Appearance") TObjectPtr<class UProceduralMeshComponent> WaypointMarker;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Navigation|Appearance") TObjectPtr<class UTextRenderComponent> WaypointLabel;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Navigation|Appearance") bool bShowWorldLabel=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Navigation|Appearance",meta=(ClampMin="10")) float MarkerHeight=240;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Navigation|Appearance",meta=(ClampMin="5")) float MarkerWidth=100;
private:
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> MarkerMaterial;
    void RebuildMarker();
public:
    // Fixed legacy rule, not a BP-adjustable arrival distance.
    static double LegacyArrivalRadius(double HullRadius);
    void SetArrivalHullRadius(double HullRadius);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") double ArrivalRadiusMeters = 0.0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") TObjectPtr<class USphereComponent> ArrivalSphere;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation|Appearance")
    TObjectPtr<class UStaticMeshComponent> WaypointMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") int32 ElementIdentity = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") int32 WaypointIndex = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FString SectorName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FString Description;
    FString NamedLocationType;
    bool bAsteroidFieldContact = false;
    static constexpr const TCHAR* AsteroidContactIcon = TEXT("/Game/UI/HUD/AsteroidHud.AsteroidHud");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FVector LegacyLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") bool bNextWaypoint = false;
};
