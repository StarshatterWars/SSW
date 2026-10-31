#include "StarshatterInputBindings.h"
#include "StarshatterKeyboardSettings.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "UObject/UnrealType.h"
#include "Misc/SecureHash.h"
#include "GameFramework/PlayerController.h"

namespace
{
    struct FRuntimeBindings
    {
        TWeakObjectPtr<UInputMappingContext> Context;
        TWeakObjectPtr<UInputMappingContext> Source;
        TArray<FString> Ids;
        TArray<FKey> Defaults;
    };
    TArray<FRuntimeBindings> RuntimeBindings;

    FString Signature(const UObject* Object)
    {
        if (!Object) return TEXT("None");
        FString Result=Object->GetClass()->GetPathName();
        for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
        {
            if (!It->HasAnyPropertyFlags(CPF_Edit) || It->HasAnyPropertyFlags(CPF_Transient)) continue;
            FString Value;
            It->ExportText_InContainer(0,Value,Object,nullptr,nullptr,PPF_None);
            Result+=TEXT("|")+It->GetName()+TEXT("=")+Value;
        }
        return Result;
    }
}

UInputMappingContext* StarshatterInputBindings::SupplementalContext(UObject* Owner)
{
    auto* Context=NewObject<UInputMappingContext>(Owner,NAME_None,RF_Transient);
    auto Add=[&](const TCHAR* Name,const FString& Label,FKey Key,EInputActionValueType Type=EInputActionValueType::Boolean)
    {
        auto* Action=NewObject<UInputAction>(Context,Name,RF_Transient);
        Action->ActionDescription=FText::FromString(Label);Action->ValueType=Type;
        Context->MapKey(Action,Key);
    };
    Add(TEXT("SSW_TargetZoom"),TEXT("Target View Zoom"),EKeys::MouseWheelAxis,EInputActionValueType::Axis1D);
    Add(TEXT("SSW_HUDVisibility"),TEXT("Show / Hide HUD"),EKeys::H);
    Add(TEXT("SSW_WeaponCycle"),TEXT("Cycle Active Weapon"),EKeys::BackSpace);
    Add(TEXT("SSW_SensorRangeUp"),TEXT("Increase Sensor Range (Alternate)"),EKeys::RightBracket);
    Add(TEXT("SSW_SensorRangeDown"),TEXT("Decrease Sensor Range (Alternate)"),EKeys::LeftBracket);
    Add(TEXT("SSW_ReferenceCameraOrbit"),TEXT("Orbit Reference Preview"),EKeys::LeftMouseButton);
    Add(TEXT("SSW_ReferenceCameraZoom"),TEXT("Zoom Reference Preview"),EKeys::MouseWheelAxis,EInputActionValueType::Axis1D);
    Add(TEXT("SSW_BriefingAccept"),TEXT("Accept Mission Briefing"),EKeys::Enter);
    const FKey Digits[]={EKeys::Zero,EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
    for (int32 Index=0;Index<10;++Index)
        Add(*FString::Printf(TEXT("SSW_RadioChoice%d"),Index),FString::Printf(TEXT("Radio Menu Choice %d"),Index),Digits[Index]);
    return Context;
}

bool StarshatterInputBindings::IsModifierDown(APlayerController* Controller,const FString& Modifier)
{
    if (!Controller) return false;
    bool bFound=false;
    for (const auto& Item:RuntimeBindings)
        if (Item.Context.IsValid() && Item.Context->GetWorld()==Controller->GetWorld())
            for (const auto& Mapping:Item.Context->GetMappings())
                if (Mapping.Action && Mapping.Action->GetName().Contains(TEXT("Modifier")) && Mapping.Action->GetName().Contains(Modifier))
                { bFound=true;if(Controller->IsInputKeyDown(Mapping.Key)) return true; }
    if (bFound) return false;
    if(Modifier==TEXT("Control")) return Controller->IsInputKeyDown(EKeys::LeftControl) || Controller->IsInputKeyDown(EKeys::RightControl);
    return Controller->IsInputKeyDown(EKeys::LeftShift) || Controller->IsInputKeyDown(EKeys::RightShift);
}

FString StarshatterInputBindings::Id(const FEnhancedActionKeyMapping& Mapping)
{
    FString Settings;
    for (const auto& Modifier:Mapping.Modifiers) Settings+=Signature(Modifier);
    for (const auto& Trigger:Mapping.Triggers) Settings+=Signature(Trigger);
    const FString ActionId=Mapping.Action && Mapping.Action->GetName().StartsWith(TEXT("SSW_"))?Mapping.Action->GetName():GetPathNameSafe(Mapping.Action.Get());
    return ActionId+TEXT("|")+Mapping.Key.ToString()+TEXT("|")+FMD5::HashAnsiString(*Settings);
}

UInputMappingContext* StarshatterInputBindings::RuntimeContext(UInputMappingContext* Source,UObject* Owner)
{
    if (!Source || !Owner) return Source;
    RuntimeBindings.RemoveAll([](const FRuntimeBindings& Item){return !Item.Context.IsValid();});
    for (const auto& Item:RuntimeBindings)
        if (Item.Source==Source && Item.Context->GetOuter()==Owner) return Item.Context.Get();
    auto* Copy=DuplicateObject<UInputMappingContext>(Source,Owner,NAME_None);
    Copy->SetFlags(RF_Transient); Copy->ClearFlags(RF_Public|RF_Standalone);
    FRuntimeBindings Item; Item.Context=Copy; Item.Source=Source;
    for (const auto& Mapping:Source->GetMappings()) {Item.Ids.Add(Id(Mapping));Item.Defaults.Add(Mapping.Key);}
    RuntimeBindings.Add(MoveTemp(Item));
    Refresh();
    return Copy;
}

void StarshatterInputBindings::RegisterContext(UInputMappingContext* Context)
{
    if (!Context) return;
    for (const auto& Item:RuntimeBindings) if (Item.Context==Context) return;
    FRuntimeBindings Item;Item.Context=Context;
    for (const auto& Mapping:Context->GetMappings())
    {
        FString BindingId=Id(Mapping);FKey Default=Mapping.Key;
        // Higher-priority mission contexts copy authored mappings. Keep their original IDs,
        // even when a saved override was already applied before the copy was made.
        bool bFound=false;
        for (const auto& Other:RuntimeBindings)
        {
            if (!Other.Context.IsValid()) continue;
            const auto& Mappings=Other.Context->GetMappings();
            for (int32 Index=0;Index<Mappings.Num();++Index)
                if (Mappings[Index]==Mapping)
                { BindingId=Other.Ids[Index];Default=Other.Defaults[Index];bFound=true;break; }
            if (bFound) break;
        }
        Item.Ids.Add(BindingId);Item.Defaults.Add(Default);
    }
    RuntimeBindings.Add(MoveTemp(Item));Refresh();
}

void StarshatterInputBindings::Refresh()
{
    const auto& Config=UStarshatterKeyboardSettings::Get()->GetKeyboardConfig();
    RuntimeBindings.RemoveAll([](const FRuntimeBindings& Item){return !Item.Context.IsValid();});
    for (auto& Item:RuntimeBindings)
        for (int32 Index=0;Index<Item.Ids.Num() && Index<Item.Context->GetMappings().Num();++Index)
        {
            const FKey* Key=Config.EnhancedBindings.Find(Item.Ids[Index]);
            Item.Context->GetMapping(Index).Key=Key && Key->IsValid()?*Key:Item.Defaults[Index];
        }
}

FString StarshatterInputBindings::Category(const FString& Name)
{
    if (Name.Contains(TEXT("Modifier"))) return TEXT("MODIFIER KEYS");
    if (Name.Contains(TEXT("Cam")) || Name.Contains(TEXT("View")) || Name.Contains(TEXT("Zoom")) || Name.Contains(TEXT("Padlock"))) return TEXT("CAMERA & VIEWS");
    if (Name.Contains(TEXT("Comm")) || Name.Contains(TEXT("Radio")) || Name.Contains(TEXT("Chat"))) return TEXT("COMMUNICATIONS");
    if (Name.Contains(TEXT("Sensor")) || Name.Contains(TEXT("Emcon")) || Name.Contains(TEXT("EMCON")) || Name.Contains(TEXT("Probe")) || Name.Contains(TEXT("GroundMode"))) return TEXT("SENSORS");
    if (Name.Contains(TEXT("Target")) || Name.Contains(TEXT("Subtarget"))) return TEXT("TARGETING");
    if (Name.Contains(TEXT("Fire")) || Name.Contains(TEXT("CyclePrimary")) || Name.Contains(TEXT("CycleSecondary")) || Name.Contains(TEXT("Decoy")) || (Name.Contains(TEXT("Weapon")) && !Name.Contains(TEXT("Panel")) && !Name.Contains(TEXT("Dialog")))) return TEXT("WEAPONS");
    if (Name.Contains(TEXT("Pitch")) || Name.Contains(TEXT("Yaw")) || Name.Contains(TEXT("Roll")) || Name.Contains(TEXT("Throttle")) || Name.Contains(TEXT("Augmenter")) || Name.Contains(TEXT("Plus")) || Name.Contains(TEXT("Minus")) || Name.Contains(TEXT("FLCS")) || Name.Contains(TEXT("Flcs"))) return TEXT("MOVEMENT & FLIGHT");
    if (Name.Contains(TEXT("HUD")) || Name.Contains(TEXT("Hud")) || Name.Contains(TEXT("MFD")) || Name.Contains(TEXT("Panel")) || Name.Contains(TEXT("Dialog"))) return TEXT("HUD & PANELS");
    if (Name.Contains(TEXT("Gear")) || Name.Contains(TEXT("Nav")) || Name.Contains(TEXT("Shield")) || Name.Contains(TEXT("Orbit")) || Name.Contains(TEXT("Quantum"))) return TEXT("NAVIGATION & SHIP SYSTEMS");
    return TEXT("GENERAL");
}


FKey StarshatterInputBindings::ResolvedKey(const FString& Name,FKey Default)
{
    const FString BindingId=Name+TEXT("|")+Default.ToString()+TEXT("|")+FMD5::HashAnsiString(TEXT(""));
    const auto& Config=UStarshatterKeyboardSettings::Get()->GetKeyboardConfig();
    const FKey* Saved=Config.EnhancedBindings.Find(BindingId);return Saved && Saved->IsValid()?*Saved:Default;
}

void StarshatterInputBindings::ConfigureAugmenterHold(FEnhancedActionKeyMapping& Mapping,UObject* Owner)
{
    if(!Owner || !Mapping.Action || Mapping.Action->GetName()!=TEXT("IA_Augmenter"))return;
    for(auto& Trigger:Mapping.Triggers)
        if(const auto* Hold=Cast<UInputTriggerHold>(Trigger))
        {
            auto* Continuous=DuplicateObject<UInputTriggerHold>(Hold,Owner);
            Continuous->HoldTimeThreshold=UE_SMALL_NUMBER;
            Continuous->bIsOneShot=false;
            Trigger=Continuous;
        }
}
