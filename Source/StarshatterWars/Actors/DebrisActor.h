#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DebrisActor.generated.h"

class Debris;
class UStaticMeshComponent;

UCLASS()
class STARSHATTERWARS_API ADebrisActor : public AActor
{
    GENERATED_BODY()

public:
    ADebrisActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Debris")
    TObjectPtr<UStaticMeshComponent> DebrisMesh;

    static TSubclassOf<ADebrisActor> ResolveVisualClass(FName Model);
    double GetVisualRadius() const;
    void UpdateFromSimulation(const Debris& Piece, const FVector& Position);
    void SetVisualModel(FName Model) { VisualModel = Model; }
    FName GetVisualModel() const { return VisualModel; }

private:
    FName VisualModel;
};
