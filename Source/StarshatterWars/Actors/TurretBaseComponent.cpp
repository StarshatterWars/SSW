#include "TurretBaseComponent.h"
#include "TurretBaseCatalog.h"
UTurretBaseComponent::UTurretBaseComponent(){bEditableWhenInherited=true;PrimaryComponentTick.bCanEverTick=false;}
void UTurretBaseComponent::Configure(FName Type)
{
    WeaponType=Type;TurretBaseCatalog::Definition(Type,LegacyBaseModel,ModelScale);
    UClass* Class=TurretBaseCatalog::VisualClass(LegacyBaseModel).Get();
    if(!GetChildActorClass() || GetChildActorClass()==AutoBaseClass.Get())SetChildActorClass(Class);
    AutoBaseClass=Class;ApplyVisualDefinition();
}
void UTurretBaseComponent::ApplyVisualDefinition()
{
    if(auto* Base=Cast<ATurretBaseActor>(GetChildActor()))Base->ApplyDefinition(LegacyBaseModel,ModelScale);
}
void UTurretBaseComponent::OnRegister(){Super::OnRegister();ApplyVisualDefinition();}
