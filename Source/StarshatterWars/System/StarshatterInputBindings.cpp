#include "StarshatterInputBindings.h"
#include "StarshatterKeyboardSettings.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "UObject/UnrealType.h"
#include "Misc/SecureHash.h"
#include "KeyMap.h"
#include "GameFramework/PlayerController.h"
#include "GameStructs_InputDefaults.h"

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

void StarshatterInputBindings::ApplyLegacy(KeyMap& Mapping,const FStarshatterKeyboardConfig& Config,UObject* Owner)
{
    static TMap<TWeakObjectPtr<UObject>,TMap<int32,int32>> OriginalKeys;
    for(auto It=OriginalKeys.CreateIterator();It;++It) if(!It.Key().IsValid()) It.RemoveCurrent();
    TMap<int32,int32>* Original=Owner?&OriginalKeys.FindOrAdd(Owner):nullptr;
    if(Original) for(const auto& Pair:*Original)
    {const int32 Index=Mapping.FindMapIndex(Pair.Key);if(Index>=0 && Index<Mapping.GetNumKeys()) Mapping.GetKeyMap(Index)->key=Pair.Value;}
    // Filled by the explicit legacy action bridge below. Retains each default chord.
    auto Apply=[&](EStarshatterInputAction Action,int32 Legacy)
    {
        const FKey* Key=Config.RemappedKeys.Find(Action);
        if (!Key) return;
        const uint32* Code=nullptr;const uint32* Character=nullptr;
        FInputKeyManager::Get().GetCodesFromKey(*Key,Code,Character);
        if (!Code) return;
        const int32 Index=Mapping.FindMapIndex(Legacy);
        if (Index>=0 && Index<Mapping.GetNumKeys())
        {
            if(Original && !Original->Contains(Legacy)) Original->Add(Legacy,Mapping.GetKeyMap(Index)->key);
            Mapping.GetKeyMap(Index)->key=static_cast<int32>(*Code);
        }
    };
    Apply(EStarshatterInputAction::ExitGame,KEY_EXIT);
Apply(EStarshatterInputAction::Pause,KEY_PAUSE);
Apply(EStarshatterInputAction::TimeCompress,KEY_TIME_COMPRESS);
Apply(EStarshatterInputAction::TimeExpand,KEY_TIME_EXPAND);
Apply(EStarshatterInputAction::TimeSkip,KEY_TIME_SKIP);
Apply(EStarshatterInputAction::PitchUp,KEY_PITCH_UP);
Apply(EStarshatterInputAction::PitchDown,KEY_PITCH_DOWN);
Apply(EStarshatterInputAction::YawLeft,KEY_YAW_LEFT);
Apply(EStarshatterInputAction::YawRight,KEY_YAW_RIGHT);
Apply(EStarshatterInputAction::RollLeft,KEY_ROLL_LEFT);
Apply(EStarshatterInputAction::RollRight,KEY_ROLL_RIGHT);
Apply(EStarshatterInputAction::PlusX,KEY_PLUS_X);
Apply(EStarshatterInputAction::MinusX,KEY_MINUS_X);
Apply(EStarshatterInputAction::PlusY,KEY_PLUS_Y);
Apply(EStarshatterInputAction::MinusY,KEY_MINUS_Y);
Apply(EStarshatterInputAction::PlusZ,KEY_PLUS_Z);
Apply(EStarshatterInputAction::MinusZ,KEY_MINUS_Z);
Apply(EStarshatterInputAction::ThrottleUp,KEY_THROTTLE_UP);
Apply(EStarshatterInputAction::ThrottleDown,KEY_THROTTLE_DOWN);
Apply(EStarshatterInputAction::ThrottleZero,KEY_THROTTLE_ZERO);
Apply(EStarshatterInputAction::ThrottleFull,KEY_THROTTLE_FULL);
Apply(EStarshatterInputAction::Augmenter,KEY_AUGMENTER);
Apply(EStarshatterInputAction::FlcsModeAuto,KEY_FLCS_MODE_AUTO);
Apply(EStarshatterInputAction::CommandMode,KEY_COMMAND_MODE);
Apply(EStarshatterInputAction::CyclePrimary,KEY_CYCLE_PRIMARY);
Apply(EStarshatterInputAction::CycleSecondary,KEY_CYCLE_SECONDARY);
Apply(EStarshatterInputAction::Decoy,KEY_DECOY);
Apply(EStarshatterInputAction::LockTarget,KEY_LOCK_TARGET);
Apply(EStarshatterInputAction::LockThreat,KEY_LOCK_THREAT);
Apply(EStarshatterInputAction::LockClosestShip,KEY_LOCK_CLOSEST_SHIP);
Apply(EStarshatterInputAction::LockClosestThreat,KEY_LOCK_CLOSEST_THREAT);
Apply(EStarshatterInputAction::LockHostileShip,KEY_LOCK_HOSTILE_SHIP);
Apply(EStarshatterInputAction::LockHostileThreat,KEY_LOCK_HOSTILE_THREAT);
Apply(EStarshatterInputAction::CycleSubtarget,KEY_CYCLE_SUBTARGET);
Apply(EStarshatterInputAction::PrevSubtarget,KEY_PREV_SUBTARGET);
Apply(EStarshatterInputAction::TargetPadlock,KEY_TARGET_PADLOCK);
Apply(EStarshatterInputAction::GearToggle,KEY_GEAR_TOGGLE);
Apply(EStarshatterInputAction::NavlightToggle,KEY_NAVLIGHT_TOGGLE);
Apply(EStarshatterInputAction::AutoNav,KEY_AUTO_NAV);
Apply(EStarshatterInputAction::DropOrbit,KEY_DROP_ORBIT);
Apply(EStarshatterInputAction::ShieldsUp,KEY_SHIELDS_UP);
Apply(EStarshatterInputAction::ShieldsDown,KEY_SHIELDS_DOWN);
Apply(EStarshatterInputAction::ShieldsFull,KEY_SHIELDS_FULL);
Apply(EStarshatterInputAction::ShieldsZero,KEY_SHIELDS_ZERO);
Apply(EStarshatterInputAction::SensorMode,KEY_SENSOR_MODE);
Apply(EStarshatterInputAction::SensorGroundMode,KEY_SENSOR_GROUND_MODE);
Apply(EStarshatterInputAction::LaunchProbe,KEY_LAUNCH_PROBE);
Apply(EStarshatterInputAction::SensorRangeMinus,KEY_SENSOR_RANGE_MINUS);
Apply(EStarshatterInputAction::SensorRangePlus,KEY_SENSOR_RANGE_PLUS);
Apply(EStarshatterInputAction::EmconMinus,KEY_EMCON_MINUS);
Apply(EStarshatterInputAction::EmconPlus,KEY_EMCON_PLUS);
Apply(EStarshatterInputAction::CamBridge,KEY_CAM_BRIDGE);
Apply(EStarshatterInputAction::CamVirt,KEY_CAM_VIRT);
Apply(EStarshatterInputAction::CamChase,KEY_CAM_CHASE);
Apply(EStarshatterInputAction::CamDrop,KEY_CAM_DROP);
Apply(EStarshatterInputAction::CamExtern,KEY_CAM_EXTERN);
Apply(EStarshatterInputAction::ZoomWide,KEY_ZOOM_WIDE);
Apply(EStarshatterInputAction::CamCycleObject,KEY_CAM_CYCLE_OBJECT);
Apply(EStarshatterInputAction::CamExtPlusAz,KEY_CAM_EXT_PLUS_AZ);
Apply(EStarshatterInputAction::CamExtMinusAz,KEY_CAM_EXT_MINUS_AZ);
Apply(EStarshatterInputAction::CamExtPlusEl,KEY_CAM_EXT_PLUS_EL);
Apply(EStarshatterInputAction::CamExtMinusEl,KEY_CAM_EXT_MINUS_EL);
Apply(EStarshatterInputAction::CamExtPlusRange,KEY_CAM_EXT_PLUS_RANGE);
Apply(EStarshatterInputAction::CamExtMinusRange,KEY_CAM_EXT_MINUS_RANGE);
Apply(EStarshatterInputAction::CamViewSelection,KEY_CAM_VIEW_SELECTION);
Apply(EStarshatterInputAction::CamVirtPlusAz,KEY_CAM_VIRT_PLUS_AZ);
Apply(EStarshatterInputAction::CamVirtMinusAz,KEY_CAM_VIRT_MINUS_AZ);
Apply(EStarshatterInputAction::CamVirtPlusEl,KEY_CAM_VIRT_PLUS_EL);
Apply(EStarshatterInputAction::CamVirtMinusEl,KEY_CAM_VIRT_MINUS_EL);
Apply(EStarshatterInputAction::HudMode,KEY_HUD_MODE);
Apply(EStarshatterInputAction::HudColor,KEY_HUD_COLOR);
Apply(EStarshatterInputAction::HudWarn,KEY_HUD_WARN);
Apply(EStarshatterInputAction::HudInst,KEY_HUD_INST);
Apply(EStarshatterInputAction::NavDialog,KEY_NAV_DLG);
Apply(EStarshatterInputAction::WeaponDialog,KEY_WEP_DLG);
Apply(EStarshatterInputAction::FlightDialog,KEY_FLT_DLG);
Apply(EStarshatterInputAction::EngineDialog,KEY_ENG_DLG);
Apply(EStarshatterInputAction::RadioMenu,KEY_RADIO_MENU);
Apply(EStarshatterInputAction::QuantumMenu,KEY_QUANTUM_MENU);
Apply(EStarshatterInputAction::MFD1,KEY_MFD1);
Apply(EStarshatterInputAction::MFD2,KEY_MFD2);
Apply(EStarshatterInputAction::SelfDestruct,KEY_SELF_DESTRUCT);
Apply(EStarshatterInputAction::SwapRollYaw,KEY_SWAP_ROLL_YAW);
Apply(EStarshatterInputAction::CommAttackTgt,KEY_COMM_ATTACK_TGT);
Apply(EStarshatterInputAction::CommEscortTgt,KEY_COMM_ESCORT_TGT);
Apply(EStarshatterInputAction::CommWepFree,KEY_COMM_WEP_FREE);
Apply(EStarshatterInputAction::CommWepHold,KEY_COMM_WEP_HOLD);
Apply(EStarshatterInputAction::CommCoverMe,KEY_COMM_COVER_ME);
Apply(EStarshatterInputAction::CommSkipNav,KEY_COMM_SKIP_NAV);
Apply(EStarshatterInputAction::CommReturnToBase,KEY_COMM_RETURN_TO_BASE);
Apply(EStarshatterInputAction::CommCallInbound,KEY_COMM_CALL_INBOUND);
Apply(EStarshatterInputAction::CommRequestPicture,KEY_COMM_REQUEST_PICTURE);
Apply(EStarshatterInputAction::CommRequestSupport,KEY_COMM_REQUEST_SUPPORT);
Apply(EStarshatterInputAction::ChatBroadcast,KEY_CHAT_BROADCAST);
Apply(EStarshatterInputAction::ChatTeam,KEY_CHAT_TEAM);
Apply(EStarshatterInputAction::ChatWing,KEY_CHAT_WING);
Apply(EStarshatterInputAction::ChatUnit,KEY_CHAT_UNIT);
Apply(EStarshatterInputAction::IncStardate,KEY_INC_STARDATE);
Apply(EStarshatterInputAction::DecStardate,KEY_DEC_STARDATE);
Apply(EStarshatterInputAction::Action0,KEY_ACTION_0);
Apply(EStarshatterInputAction::Action1,KEY_ACTION_1);
Apply(EStarshatterInputAction::FirePrimary,KEY_ACTION_0);
Apply(EStarshatterInputAction::FireSecondary,KEY_ACTION_1);
Apply(EStarshatterInputAction::CameraNextView,KEY_NEXT_VIEW);
Apply(EStarshatterInputAction::CameraChase,KEY_CAM_CHASE);
Apply(EStarshatterInputAction::CameraExternal,KEY_CAM_EXTERN);
Apply(EStarshatterInputAction::CameraZoomIn,KEY_ZOOM_IN);
Apply(EStarshatterInputAction::CameraZoomOut,KEY_ZOOM_OUT);
}
