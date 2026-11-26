#pragma once
#include "CoreMinimal.h"
#include "GameStructs_System.h"
#include "GameStructs_insurance.h"
#include "SimComponent.h"
class Ship;
class SimSystem;
struct FShipDesign;

// Owned by every native Ship. Actors only display the resulting simulation.
// Level III preserves the authored simulation baseline. I-II are inferior,
// IV-V superior; issuing standard Level I never recalibrates it to military III.
class FShipEquipment
{
public:
 explicit FShipEquipment(Ship&);
 Ship& GetOwner() const {return Owner;}
 int32 GetStandardLevel() const {return StandardLevel;}
 void InitializeStandard(int32 Level);
 void Capture(FPlayerHangarShip&) const;
 void Apply(const FPlayerHangarShip&);
 static const TMap<FName,FSystemDesign>& GetCatalog();
 static void PublishCatalog(const TMap<FName,FSystemDesign>&);
 static TOptional<ComponentDesign> GradeDesign(FName System,FName Part,int32 Level);
 static int64 ComponentValue(FName System,FName Part,int32 Level,int64 Fallback=0);
 static void InitializePurchase(FPlayerHangarShip&,const FShipDesign&);
 static void SeparateValue(FPlayerHangarShip&);
 static void Revalue(FPlayerHangarShip&);
private:
 void Bind();
 Ship& Owner;
 int32 StandardLevel=3;
};
