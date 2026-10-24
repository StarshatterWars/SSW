#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CarrierInteriorBuilderComponent.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** Dimensions are unscaled ship-local units: X forward, Y right, Z up. */
USTRUCT(BlueprintType)
struct FCarrierInteriorBay
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name = TEXT("Hangar");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
    /** Origin is the center of the FINISHED floor surface, not its underside. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MakeEditWidget=true)) FTransform FloorTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) FVector Size = FVector(400,120,70);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1")) float Thickness = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCeiling = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bLeftWall = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRightWall = true;
    /** Openings are centered in each end wall; zero width creates a closed wall. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) FVector2D FrontOpening = FVector2D(80,55);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) FVector2D RearOpening = FVector2D(80,55);
};

/** A custom box or replacement mesh, e.g. a catwalk, column, bulkhead, or service platform. */
USTRUCT(BlueprintType)
struct FCarrierInteriorModule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name = TEXT("Module");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MakeEditWidget=true)) FTransform Transform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1")) FVector Size = FVector(100,100,10);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UMaterialInterface> Material;
    /** Complete room/corridor assemblies. When supplied, this takes priority over Mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AActor> RoomBlueprint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bFitMeshToSize = true;
    /** An empty anchor has a viewport arrow instead of a solid placeholder box. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAnchorOnly = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bStarterAnchor = false;
};

/** Presentation-only modular interior. No gameplay, deck queues, or simulation transforms are owned here. */
UCLASS(ClassGroup=(Starshatter, Carrier), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UCarrierInteriorBuilderComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UCarrierInteriorBuilderComponent();

    /** Enable after placing Orion. Defaults are a blockout, not final hull-fitted architecture. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Build") bool bBuildInterior = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Build") bool bAutoRebuild = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Build") bool bShowCeilings = true;
    /** Optional simple collision for future walking. Legacy spacecraft remain simulation-driven. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Build") bool bWalkingCollision = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Layout") TArray<FCarrierInteriorBay> Bays;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Layout") TArray<FCarrierInteriorModule> Modules;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Materials") TObjectPtr<UMaterialInterface> FloorMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Materials") TObjectPtr<UMaterialInterface> WallMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Materials") TObjectPtr<UMaterialInterface> CeilingMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Meshes") TObjectPtr<UStaticMesh> FloorMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Meshes") TObjectPtr<UStaticMesh> WallMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interior|Meshes") TObjectPtr<UStaticMesh> CeilingMesh;

    /** Explicitly replaces bay settings with four Orion-aligned starter bays; custom modules are retained. */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Interior|Build") void LoadOrionStarterLayout();
    /** Four flight bays plus editable whole-ship asset placement anchors. Preserves custom modules. */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Interior|Build") void LoadOrionFullInteriorLayout();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Interior|Build") void RebuildInterior();
    /** Disables preview generation and removes only this builder's generated components. */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Interior|Build") void ClearInterior();
    /** Creates independently editable static-mesh actors attached to Orion. Preview is then disabled. */
    UFUNCTION(CallInEditor, Category="Interior|Bake") void BakeToEditableActors();
    UFUNCTION(BlueprintPure, Category="Interior|Build") int32 GetGeneratedPieceCount() const { return GeneratedPieces.Num(); }

    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
private:
    UPROPERTY() TObjectPtr<UStaticMesh> BlockMesh;
    UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> GeneratedPieces;
    bool bRebuilding = false;
    void RemoveGeneratedPieces();
    void AddPiece(const FString& Label, const FTransform& Frame, const FVector& Center,
        const FVector& Size, UMaterialInterface* Material, UStaticMesh* Mesh = nullptr);
    void BuildEndWall(const FCarrierInteriorBay& Bay, float Direction, const FVector2D& Opening);
};
