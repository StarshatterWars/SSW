#include "ShipEquipment.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "SimSystem.h"
#include "Misc/SecureHash.h"

namespace
{
TMap<FName,FSystemDesign> EquipmentCatalog;
FGuid PartIdentity(FGuid Hull,int32 System,int32 Index)
{
 FGuid Id;const FString Hash=FMD5::HashAnsiString(*FString::Printf(TEXT("%s/System/%d/Part/%d"),*Hull.ToString(),System,Index));
 FGuid::ParseExact(Hash,EGuidFormats::Digits,Id);return Id;
}
const FComponentDesign* FindPart(FName System,FName Part)
{
 const auto* Record=EquipmentCatalog.Find(System);
 return Record?Record->Components.FindByPredicate([Part](const auto& Row){return FName(*Row.Name)==Part;}):nullptr;
}
const FSystemComponentGrade* FindGrade(FName System,int32 Level)
{
 const auto* Record=EquipmentCatalog.Find(System);
 return Record?Record->ComponentGrades.FindByPredicate([Level](const auto& Row){return Row.Level==Level;}):nullptr;
}
int64 SumValue(const TArray<FHangarComponent>& Components)
{
 int64 Sum=0;for(const auto& Part:Components)Sum=FMath::Min(1000000000000000LL,Sum+FMath::Clamp(Part.InstalledValue,0LL,1000000000000000LL));return Sum;
}
}
const TMap<FName,FSystemDesign>& FShipEquipment::GetCatalog(){return EquipmentCatalog;}
void FShipEquipment::PublishCatalog(const TMap<FName,FSystemDesign>& Rows){EquipmentCatalog=Rows;}
FShipEquipment::FShipEquipment(Ship& Vessel):Owner(Vessel){Bind();}
void FShipEquipment::Bind()
{
 auto& Systems=Owner.GetSystems();
 for(int32 S=0;S<Systems.size();++S)if(auto* System=Systems[S])
  for(int32 I=0;I<System->GetComponents().size();++I)if(auto* Part=System->GetComponents()[I])
  {System->SetShip(&Owner);Part->SetSystem(System);if(Part->OwnerHullId!=Owner.GetHullId() || !Part->ItemId.IsValid())Part->ItemId=PartIdentity(Owner.GetHullId(),S,I);Part->OwnerHullId=Owner.GetHullId();if(Part->BaseName.IsNone())Part->BaseName=FName(UTF8_TO_TCHAR(Part->Name()));}
}
TOptional<ComponentDesign> FShipEquipment::GradeDesign(FName System,FName Part,int32 Level)
{
 const auto* Base=FindPart(System,Part);const auto* Grade=FindGrade(System,Level);const auto* Calibration=FindGrade(System,3);
 if(!Base || !Grade || !Calibration || Calibration->PerformanceMultiplier<=0 || Calibration->PowerDemandMultiplier<=0 || Calibration->RepairScale<=0 || Calibration->ReplacementScale<=0)return {};
 ComponentDesign Native;Native.name=TCHAR_TO_UTF8(*Base->Name);Native.abrv=TCHAR_TO_UTF8(*Base->Abbrev);
 Native.base_cost=ComponentValue(System,Part,Level);
 Native.performance_multiplier=Base->PerformanceMultiplier*Grade->PerformanceMultiplier/Calibration->PerformanceMultiplier;
 Native.power_demand_multiplier=Base->PowerDemandMultiplier*Grade->PowerDemandMultiplier/Calibration->PowerDemandMultiplier;
 Native.repair_time=Base->RepairTime*Grade->RepairScale/Calibration->RepairScale;Native.replace_time=Base->ReplaceTime*Grade->ReplacementScale/Calibration->ReplacementScale;
 Native.spares=Base->Spares;Native.affects=Base->Affects;return Native;
}
int64 FShipEquipment::ComponentValue(FName System,FName Part,int32 Level,int64 Fallback)
{
 const auto* Base=FindPart(System,Part);const auto* Grade=FindGrade(System,Level);
 return Base && Grade?FMath::CeilToInt64(FMath::Clamp(double(Base->BaseCost)*Grade->PriceMultiplier,0.,1.e15)):FMath::Clamp(Fallback,0LL,1000000000000000LL);
}
void FShipEquipment::InitializeStandard(int32 Level)
{
 Bind();
 StandardLevel=FMath::Clamp(Level,1,5);auto& Systems=Owner.GetSystems();
 for(int32 S=0;S<Systems.size();++S)if(auto* System=Systems[S])
 {
  for(int32 I=0;I<System->GetComponents().size();++I)if(auto* Part=System->GetComponents()[I])
  {Part->InstalledLevel=StandardLevel;if(const auto Design=GradeDesign(System->GetDesignKey(),Part->BaseName,StandardLevel))Part->ApplyGradeDesign(*Design);}
  System->CalcStatus();
 }
}
void FShipEquipment::Capture(FPlayerHangarShip& Row) const
{
 const_cast<FShipEquipment*>(this)->Bind();
 Row.StandardEquipmentLevel=StandardLevel;auto& Systems=Owner.GetSystems();
 for(int32 S=0;S<Systems.size();++S)if(auto* System=Systems[S])
  for(int32 I=0;I<System->GetComponents().size();++I)if(const auto* Part=System->GetComponents()[I])
  {
   auto* Slot=Row.Components.FindByPredicate([&](const auto& C){return C.SystemIndex==S && C.ComponentIndex==I;});
   if(!Slot){Slot=&Row.Components.AddDefaulted_GetRef();Slot->SystemIndex=S;Slot->ComponentIndex=I;Slot->SystemName=UTF8_TO_TCHAR(System->GetName());}
   Slot->ItemId=Part->ItemId;Slot->SystemDesign=System->GetDesignKey();Slot->Component=Part->BaseName;Slot->Level=Part->InstalledLevel;
   if(Slot->InstalledValue<0)Slot->InstalledValue=ComponentValue(Slot->SystemDesign,Slot->Component,Slot->Level,Part->Cost());
  }
}
void FShipEquipment::Apply(const FPlayerHangarShip& Row)
{
 Bind();
 StandardLevel=FMath::Clamp(Row.StandardEquipmentLevel,1,5);auto& Systems=Owner.GetSystems();
 if(Row.Components.IsEmpty()){InitializeStandard(StandardLevel);return;}
 for(const auto& Slot:Row.Components)
 {
  if(Slot.SystemIndex<0 || Slot.SystemIndex>=Systems.size())continue;
  auto* System=Systems[Slot.SystemIndex];if(!System || Slot.ComponentIndex<0 || Slot.ComponentIndex>=System->GetComponents().size())continue;
  auto* Part=System->GetComponents()[Slot.ComponentIndex];if(!Part)continue;
  const FName Base=Part->BaseName.IsNone()?FName(UTF8_TO_TCHAR(Part->Name())):Part->BaseName;
  static const TCHAR* Roman[]={TEXT("I"),TEXT("II"),TEXT("III"),TEXT("IV"),TEXT("V")};
  const FName OldGradedName(*(Base.ToString()+TEXT(" ")+Roman[FMath::Clamp(Slot.Level,1,5)-1]));
  if((Base!=Slot.Component && OldGradedName!=Slot.Component) || (!Slot.SystemDesign.IsNone() && Slot.SystemDesign!=System->GetDesignKey()))continue;
  Part->InstalledLevel=FMath::Clamp(Slot.Level,1,5);if(Slot.ItemId.IsValid())Part->ItemId=Slot.ItemId;
  if(const auto Design=GradeDesign(System->GetDesignKey(),Base,Part->InstalledLevel))Part->ApplyGradeDesign(*Design);
 }
}
void FShipEquipment::Revalue(FPlayerHangarShip& Row)
{
 for(auto& Part:Row.Components)if(Part.InstalledValue<0)Part.InstalledValue=ComponentValue(Part.SystemDesign,Part.Component,Part.Level);
 Row.InstalledComponentValue=SumValue(Row.Components);
}
void FShipEquipment::SeparateValue(FPlayerHangarShip& Row)
{
 Revalue(Row);if(Row.bEquipmentValueSeparated)return;
 int64 Standard=0;
 for(const auto& Part:Row.Components)Standard=FMath::Min(1000000000000000LL,Standard+ComponentValue(Part.SystemDesign,Part.Component,Row.StandardEquipmentLevel,Part.InstalledValue));
 // Older prices already included standard equipment. Do not add it a second time.
 Row.HullValue=FMath::Max(0LL,Row.HullValue-Standard);Row.bEquipmentValueSeparated=true;
}
void FShipEquipment::InitializePurchase(FPlayerHangarShip& Row,const FShipDesign& Design)
{
 Row.StandardEquipmentLevel=1;
 if(auto* Native=ShipDesignRegistry::FindLegacy(FName(*Design.ShipName)))
 {
  Ship Preview("Equipment inventory","",Native,0,0,nullptr,false);
  if(auto* Equipment=Preview.GetEquipment()){Equipment->InitializeStandard(1);Equipment->Capture(Row);}
 }
 for(auto& Part:Row.Components)Part.ItemId=PartIdentity(Row.HullId,Part.SystemIndex,Part.ComponentIndex);
 const int64 Price=FMath::Max(0LL,Row.HullValue);Revalue(Row);
 if(Row.InstalledComponentValue>Price && Row.InstalledComponentValue>0)
 {
  const double Scale=double(Price)/Row.InstalledComponentValue;
  for(auto& Part:Row.Components)Part.InstalledValue=FMath::FloorToInt64(Part.InstalledValue*Scale);
  Revalue(Row);
 }
 Row.HullValue=Price-Row.InstalledComponentValue;Row.bEquipmentValueSeparated=!Row.Components.IsEmpty();
}
