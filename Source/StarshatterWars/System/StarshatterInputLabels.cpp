#include "StarshatterInputLabels.h"
#include "InputAction.h"
#include "InputTriggers.h"

FText UStarshatterInputLabels::GetActionLabel(const UInputAction* Action)
{
    if (!Action) return NSLOCTEXT("SSW.InputLabels","Unassigned","Unassigned Action");
    const FString Description=Action->ActionDescription.ToString().TrimStartAndEnd();
    if (!Description.IsEmpty() && !Description.StartsWith(TEXT("IA_"),ESearchCase::IgnoreCase)) return Action->ActionDescription;
    return GetDefaultActionLabel(Action->GetFName());
}
FText UStarshatterInputLabels::GetActionTypeLabel(const UInputAction* Action)
{
    return Action ? GetValueTypeLabel(Action->ValueType) : NSLOCTEXT("SSW.InputLabels","UnknownType","Unknown Input Type");
}
FText UStarshatterInputLabels::GetValueTypeLabel(EInputActionValueType ValueType)
{
    switch(ValueType)
    {
    case EInputActionValueType::Boolean: return NSLOCTEXT("SSW.InputLabels","Button","Button");
    case EInputActionValueType::Axis1D: return NSLOCTEXT("SSW.InputLabels","Axis1D","1D Axis");
    case EInputActionValueType::Axis2D: return NSLOCTEXT("SSW.InputLabels","Axis2D","2D Axis");
    case EInputActionValueType::Axis3D: return NSLOCTEXT("SSW.InputLabels","Axis3D","3D Axis");
    default: return NSLOCTEXT("SSW.InputLabels","UnknownType","Unknown Input Type");
    }
}
FText UStarshatterInputLabels::GetDefaultActionLabel(FName ActionName)
{
    const FString Name=ActionName.ToString();
    if (ActionName==TEXT("IA_Augmenter")) return NSLOCTEXT("SSW.InputActions","IA_Augmenter","Toggle Afterburner");
    if (ActionName==TEXT("IA_AutoNav")) return NSLOCTEXT("SSW.InputActions","IA_AutoNav","Toggle Autonav");
    if (ActionName==TEXT("IA_CameraChase")) return NSLOCTEXT("SSW.InputActions","IA_CameraChase","Chase View");
    if (ActionName==TEXT("IA_CameraCockpit")) return NSLOCTEXT("SSW.InputActions","IA_CameraCockpit","Cockpit View");
    if (ActionName==TEXT("IA_CameraDrop")) return NSLOCTEXT("SSW.InputActions","IA_CameraDrop","Free Camera");
    if (ActionName==TEXT("IA_CameraLookX")) return NSLOCTEXT("SSW.InputActions","IA_CameraLookX","Camera Look Horizontal");
    if (ActionName==TEXT("IA_CameraLookY")) return NSLOCTEXT("SSW.InputActions","IA_CameraLookY","Camera Look Vertical");
    if (ActionName==TEXT("IA_CameraNextObject")) return NSLOCTEXT("SSW.InputActions","IA_CameraNextObject","View Next Object");
    if (ActionName==TEXT("IA_CameraOrbit")) return NSLOCTEXT("SSW.InputActions","IA_CameraOrbit","Orbit Camera");
    if (ActionName==TEXT("IA_CameraOverview")) return NSLOCTEXT("SSW.InputActions","IA_CameraOverview","Overview Camera");
    if (ActionName==TEXT("IA_CameraRange")) return NSLOCTEXT("SSW.InputActions","IA_CameraRange","Camera Distance");
    if (ActionName==TEXT("IA_CameraTarget")) return NSLOCTEXT("SSW.InputActions","IA_CameraTarget","Target Camera");
    if (ActionName==TEXT("IA_CameraThreat")) return NSLOCTEXT("SSW.InputActions","IA_CameraThreat","Threat Camera");
    if (ActionName==TEXT("IA_CameraVirtual")) return NSLOCTEXT("SSW.InputActions","IA_CameraVirtual","Virtual Cockpit View");
    if (ActionName==TEXT("IA_CameraWide")) return NSLOCTEXT("SSW.InputActions","IA_CameraWide","Wide Camera View");
    if (ActionName==TEXT("IA_ContactsPanel")) return NSLOCTEXT("SSW.InputActions","IA_ContactsPanel","Open Contacts Panel");
    if (ActionName==TEXT("IA_Decoy")) return NSLOCTEXT("SSW.InputActions","IA_Decoy","Deploy Countermeasure");
    if (ActionName==TEXT("IA_DropWeaponTarget")) return NSLOCTEXT("SSW.InputActions","IA_DropWeaponTarget","Clear Weapon Target");
    if (ActionName==TEXT("IA_EMCONDown")) return NSLOCTEXT("SSW.InputActions","IA_EMCONDown","Decrease EMCON");
    if (ActionName==TEXT("IA_EMCONUp")) return NSLOCTEXT("SSW.InputActions","IA_EMCONUp","Increase EMCON");
    if (ActionName==TEXT("IA_EngineeringPanel")) return NSLOCTEXT("SSW.InputActions","IA_EngineeringPanel","Open Engineering Panel");
    if (ActionName==TEXT("IA_FLCSMode")) return NSLOCTEXT("SSW.InputActions","IA_FLCSMode","Cycle Flight Control Mode");
    if (ActionName==TEXT("IA_FirePrimary")) return NSLOCTEXT("SSW.InputActions","IA_FirePrimary","Fire Primary Weapon");
    if (ActionName==TEXT("IA_FireSecondary")) return NSLOCTEXT("SSW.InputActions","IA_FireSecondary","Fire Secondary Weapon");
    if (ActionName==TEXT("IA_FlightOpsPanel")) return NSLOCTEXT("SSW.InputActions","IA_FlightOpsPanel","Open Flight Operations Panel");
    if (ActionName==TEXT("IA_GearToggle")) return NSLOCTEXT("SSW.InputActions","IA_GearToggle","Raise / Lower Landing Gear");
    if (ActionName==TEXT("IA_GroundMode")) return NSLOCTEXT("SSW.InputActions","IA_GroundMode","Toggle Ground Targeting");
    if (ActionName==TEXT("IA_HUDColor")) return NSLOCTEXT("SSW.InputActions","IA_HUDColor","Cycle HUD Color");
    if (ActionName==TEXT("IA_HUDInstruments")) return NSLOCTEXT("SSW.InputActions","IA_HUDInstruments","Toggle HUD Instruments");
    if (ActionName==TEXT("IA_HUDWarnings")) return NSLOCTEXT("SSW.InputActions","IA_HUDWarnings","Toggle HUD Warnings");
    if (ActionName==TEXT("IA_HudMode")) return NSLOCTEXT("SSW.InputActions","IA_HudMode","Cycle HUD Mode");
    if (ActionName==TEXT("IA_LaunchProbe")) return NSLOCTEXT("SSW.InputActions","IA_LaunchProbe","Launch Sensor Probe");
    if (ActionName==TEXT("IA_MFDLeftCycle")) return NSLOCTEXT("SSW.InputActions","IA_MFDLeftCycle","Cycle Left MFD");
    if (ActionName==TEXT("IA_MFDRightCycle")) return NSLOCTEXT("SSW.InputActions","IA_MFDRightCycle","Cycle Right MFD");
    if (ActionName==TEXT("IA_ModifierAlt")) return NSLOCTEXT("SSW.InputActions","IA_ModifierAlt","Alt Modifier");
    if (ActionName==TEXT("IA_ModifierControl")) return NSLOCTEXT("SSW.InputActions","IA_ModifierControl","Control Modifier");
    if (ActionName==TEXT("IA_ModifierLShift")) return NSLOCTEXT("SSW.InputActions","IA_ModifierLShift","Left Shift Modifier");
    if (ActionName==TEXT("IA_ModifierRShift")) return NSLOCTEXT("SSW.InputActions","IA_ModifierRShift","Right Shift Modifier");
    if (ActionName==TEXT("IA_MouseZoom")) return NSLOCTEXT("SSW.InputActions","IA_MouseZoom","Zoom View");
    if (ActionName==TEXT("IA_NavLightsToggle")) return NSLOCTEXT("SSW.InputActions","IA_NavLightsToggle","Toggle Navigation Lights");
    if (ActionName==TEXT("IA_NavMap")) return NSLOCTEXT("SSW.InputActions","IA_NavMap","Open Navigation Map");
    if (ActionName==TEXT("IA_NavigationPanel")) return NSLOCTEXT("SSW.InputActions","IA_NavigationPanel","Open Navigation Panel");
    if (ActionName==TEXT("IA_NextTarget")) return NSLOCTEXT("SSW.InputActions","IA_NextTarget","Next Target");
    if (ActionName==TEXT("IA_ObjectivesPanel")) return NSLOCTEXT("SSW.InputActions","IA_ObjectivesPanel","Open Mission Objectives");
    if (ActionName==TEXT("IA_Pitch")) return NSLOCTEXT("SSW.InputActions","IA_Pitch","Pitch");
    if (ActionName==TEXT("IA_PreviousTarget")) return NSLOCTEXT("SSW.InputActions","IA_PreviousTarget","Previous Target");
    if (ActionName==TEXT("IA_PrimaryWeaponCycle")) return NSLOCTEXT("SSW.InputActions","IA_PrimaryWeaponCycle","Cycle Primary Weapon");
    if (ActionName==TEXT("IA_QuantumMenu")) return NSLOCTEXT("SSW.InputActions","IA_QuantumMenu","Open Quantum / Farcaster Navigation");
    if (ActionName==TEXT("IA_Quit")) return NSLOCTEXT("SSW.InputActions","IA_Quit","Open Pause Menu");
    if (ActionName==TEXT("IA_RadioMenu")) return NSLOCTEXT("SSW.InputActions","IA_RadioMenu","Open Radio Menu");
    if (ActionName==TEXT("IA_Roll")) return NSLOCTEXT("SSW.InputActions","IA_Roll","Roll");
    if (ActionName==TEXT("IA_SecondaryWeaponCycle")) return NSLOCTEXT("SSW.InputActions","IA_SecondaryWeaponCycle","Cycle Secondary Weapon");
    if (ActionName==TEXT("IA_SensorGroundMode")) return NSLOCTEXT("SSW.InputActions","IA_SensorGroundMode","Toggle Ground Sensor Mode");
    if (ActionName==TEXT("IA_SensorMode")) return NSLOCTEXT("SSW.InputActions","IA_SensorMode","Cycle Sensor Mode");
    if (ActionName==TEXT("IA_SensorRangeDown")) return NSLOCTEXT("SSW.InputActions","IA_SensorRangeDown","Decrease Sensor Range");
    if (ActionName==TEXT("IA_SensorRangeUp")) return NSLOCTEXT("SSW.InputActions","IA_SensorRangeUp","Increase Sensor Range");
    if (ActionName==TEXT("IA_ShieldsDown")) return NSLOCTEXT("SSW.InputActions","IA_ShieldsDown","Decrease Shields");
    if (ActionName==TEXT("IA_ShieldsFull")) return NSLOCTEXT("SSW.InputActions","IA_ShieldsFull","Full Shields");
    if (ActionName==TEXT("IA_ShieldsUp")) return NSLOCTEXT("SSW.InputActions","IA_ShieldsUp","Increase Shields");
    if (ActionName==TEXT("IA_ShieldsZero")) return NSLOCTEXT("SSW.InputActions","IA_ShieldsZero","Shields Off");
    if (ActionName==TEXT("IA_TargetHostile")) return NSLOCTEXT("SSW.InputActions","IA_TargetHostile","Target Hostile Ship");
    if (ActionName==TEXT("IA_TargetHostileThreat")) return NSLOCTEXT("SSW.InputActions","IA_TargetHostileThreat","Target Hostile Threat");
    if (ActionName==TEXT("IA_TargetNearest")) return NSLOCTEXT("SSW.InputActions","IA_TargetNearest","Target Nearest Ship");
    if (ActionName==TEXT("IA_TargetNearestThreat")) return NSLOCTEXT("SSW.InputActions","IA_TargetNearestThreat","Target Nearest Threat");
    if (ActionName==TEXT("IA_TargetPadlock")) return NSLOCTEXT("SSW.InputActions","IA_TargetPadlock","Toggle Target Padlock");
    if (ActionName==TEXT("IA_TargetSubsystemNext")) return NSLOCTEXT("SSW.InputActions","IA_TargetSubsystemNext","Next Target Subsystem");
    if (ActionName==TEXT("IA_TargetSubsystemPrevious")) return NSLOCTEXT("SSW.InputActions","IA_TargetSubsystemPrevious","Previous Target Subsystem");
    if (ActionName==TEXT("IA_Throttle")) return NSLOCTEXT("SSW.InputActions","IA_Throttle","Throttle");
    if (ActionName==TEXT("IA_ThrottleFull")) return NSLOCTEXT("SSW.InputActions","IA_ThrottleFull","Full Throttle");
    if (ActionName==TEXT("IA_ThrottleZero")) return NSLOCTEXT("SSW.InputActions","IA_ThrottleZero","Zero Throttle");
    if (ActionName==TEXT("IA_ViewChase")) return NSLOCTEXT("SSW.InputActions","IA_ViewChase","Chase View");
    if (ActionName==TEXT("IA_ViewCockpit")) return NSLOCTEXT("SSW.InputActions","IA_ViewCockpit","Cockpit View");
    if (ActionName==TEXT("IA_ViewOrbit")) return NSLOCTEXT("SSW.InputActions","IA_ViewOrbit","Orbit View");
    if (ActionName==TEXT("IA_ViewTarget")) return NSLOCTEXT("SSW.InputActions","IA_ViewTarget","Target View");
    if (ActionName==TEXT("IA_ViewVirtualCockpit")) return NSLOCTEXT("SSW.InputActions","IA_ViewVirtualCockpit","Virtual Cockpit View");
    if (ActionName==TEXT("IA_WeaponPattern")) return NSLOCTEXT("SSW.InputActions","IA_WeaponPattern","Cycle Weapon Pattern");
    if (ActionName==TEXT("IA_WeaponSweep")) return NSLOCTEXT("SSW.InputActions","IA_WeaponSweep","Cycle Weapon Sweep");
    if (ActionName==TEXT("IA_WeaponsPanel")) return NSLOCTEXT("SSW.InputActions","IA_WeaponsPanel","Open Weapons Panel");
    if (ActionName==TEXT("IA_Yaw")) return NSLOCTEXT("SSW.InputActions","IA_Yaw","Yaw");
    FString Friendly=Name;
    Friendly.RemoveFromStart(TEXT("IA_"),ESearchCase::IgnoreCase);
    Friendly.ReplaceInline(TEXT("_"),TEXT(" "));
    Friendly=FName::NameToDisplayString(Friendly,false).TrimStartAndEnd();
    return Friendly.IsEmpty() || ActionName.IsNone() ? NSLOCTEXT("SSW.InputLabels","UnknownAction","Unknown Action") : FText::FromString(Friendly);
}
FText UStarshatterInputLabels::GetLegacyActionLabel(EStarshatterInputAction Action)
{
    switch(Action)
    {
    case EStarshatterInputAction::None: return NSLOCTEXT("SSW.LegacyInputActions","None","None");
    case EStarshatterInputAction::ExitGame: return NSLOCTEXT("SSW.LegacyInputActions","ExitGame","Exit Game");
    case EStarshatterInputAction::Pause: return NSLOCTEXT("SSW.LegacyInputActions","Pause","Pause");
    case EStarshatterInputAction::TimeCompress: return NSLOCTEXT("SSW.LegacyInputActions","TimeCompress","Time Compress");
    case EStarshatterInputAction::TimeExpand: return NSLOCTEXT("SSW.LegacyInputActions","TimeExpand","Time Expand");
    case EStarshatterInputAction::TimeSkip: return NSLOCTEXT("SSW.LegacyInputActions","TimeSkip","Time Skip");
    case EStarshatterInputAction::PitchUp: return NSLOCTEXT("SSW.LegacyInputActions","PitchUp","Pitch Up");
    case EStarshatterInputAction::PitchDown: return NSLOCTEXT("SSW.LegacyInputActions","PitchDown","Pitch Down");
    case EStarshatterInputAction::YawLeft: return NSLOCTEXT("SSW.LegacyInputActions","YawLeft","Yaw Left");
    case EStarshatterInputAction::YawRight: return NSLOCTEXT("SSW.LegacyInputActions","YawRight","Yaw Right");
    case EStarshatterInputAction::RollLeft: return NSLOCTEXT("SSW.LegacyInputActions","RollLeft","Roll Left");
    case EStarshatterInputAction::RollRight: return NSLOCTEXT("SSW.LegacyInputActions","RollRight","Roll Right");
    case EStarshatterInputAction::PlusX: return NSLOCTEXT("SSW.LegacyInputActions","PlusX","Translate +X");
    case EStarshatterInputAction::MinusX: return NSLOCTEXT("SSW.LegacyInputActions","MinusX","Translate -X");
    case EStarshatterInputAction::PlusY: return NSLOCTEXT("SSW.LegacyInputActions","PlusY","Translate +Y");
    case EStarshatterInputAction::MinusY: return NSLOCTEXT("SSW.LegacyInputActions","MinusY","Translate -Y");
    case EStarshatterInputAction::PlusZ: return NSLOCTEXT("SSW.LegacyInputActions","PlusZ","Translate +Z");
    case EStarshatterInputAction::MinusZ: return NSLOCTEXT("SSW.LegacyInputActions","MinusZ","Translate -Z");
    case EStarshatterInputAction::Action0: return NSLOCTEXT("SSW.LegacyInputActions","Action0","Primary Action");
    case EStarshatterInputAction::Action1: return NSLOCTEXT("SSW.LegacyInputActions","Action1","Secondary Action");
    case EStarshatterInputAction::ThrottleUp: return NSLOCTEXT("SSW.LegacyInputActions","ThrottleUp","Throttle Up");
    case EStarshatterInputAction::ThrottleDown: return NSLOCTEXT("SSW.LegacyInputActions","ThrottleDown","Throttle Down");
    case EStarshatterInputAction::ThrottleZero: return NSLOCTEXT("SSW.LegacyInputActions","ThrottleZero","Throttle Zero");
    case EStarshatterInputAction::ThrottleFull: return NSLOCTEXT("SSW.LegacyInputActions","ThrottleFull","Throttle Full");
    case EStarshatterInputAction::Augmenter: return NSLOCTEXT("SSW.LegacyInputActions","Augmenter","Augmenter");
    case EStarshatterInputAction::FlcsModeAuto: return NSLOCTEXT("SSW.LegacyInputActions","FlcsModeAuto","FLCS Mode Auto");
    case EStarshatterInputAction::CommandMode: return NSLOCTEXT("SSW.LegacyInputActions","CommandMode","Command Mode");
    case EStarshatterInputAction::CyclePrimary: return NSLOCTEXT("SSW.LegacyInputActions","CyclePrimary","Cycle Primary Weapon");
    case EStarshatterInputAction::CycleSecondary: return NSLOCTEXT("SSW.LegacyInputActions","CycleSecondary","Cycle Secondary Weapon");
    case EStarshatterInputAction::FirePrimary: return NSLOCTEXT("SSW.LegacyInputActions","FirePrimary","Fire Primary");
    case EStarshatterInputAction::FireSecondary: return NSLOCTEXT("SSW.LegacyInputActions","FireSecondary","Fire Secondary");
    case EStarshatterInputAction::Decoy: return NSLOCTEXT("SSW.LegacyInputActions","Decoy","Decoy / Countermeasure");
    case EStarshatterInputAction::LockTarget: return NSLOCTEXT("SSW.LegacyInputActions","LockTarget","Lock Target");
    case EStarshatterInputAction::LockThreat: return NSLOCTEXT("SSW.LegacyInputActions","LockThreat","Lock Threat");
    case EStarshatterInputAction::LockClosestShip: return NSLOCTEXT("SSW.LegacyInputActions","LockClosestShip","Lock Closest Ship");
    case EStarshatterInputAction::LockClosestThreat: return NSLOCTEXT("SSW.LegacyInputActions","LockClosestThreat","Lock Closest Threat");
    case EStarshatterInputAction::LockHostileShip: return NSLOCTEXT("SSW.LegacyInputActions","LockHostileShip","Lock Hostile Ship");
    case EStarshatterInputAction::LockHostileThreat: return NSLOCTEXT("SSW.LegacyInputActions","LockHostileThreat","Lock Hostile Threat");
    case EStarshatterInputAction::TargetNext: return NSLOCTEXT("SSW.LegacyInputActions","TargetNext","Target Next");
    case EStarshatterInputAction::TargetPrevious: return NSLOCTEXT("SSW.LegacyInputActions","TargetPrevious","Target Previous");
    case EStarshatterInputAction::CycleSubtarget: return NSLOCTEXT("SSW.LegacyInputActions","CycleSubtarget","Cycle Subtarget");
    case EStarshatterInputAction::PrevSubtarget: return NSLOCTEXT("SSW.LegacyInputActions","PrevSubtarget","Previous Subtarget");
    case EStarshatterInputAction::TargetPadlock: return NSLOCTEXT("SSW.LegacyInputActions","TargetPadlock","Target Padlock");
    case EStarshatterInputAction::GearToggle: return NSLOCTEXT("SSW.LegacyInputActions","GearToggle","Gear Toggle");
    case EStarshatterInputAction::NavlightToggle: return NSLOCTEXT("SSW.LegacyInputActions","NavlightToggle","Nav Light Toggle");
    case EStarshatterInputAction::AutoNav: return NSLOCTEXT("SSW.LegacyInputActions","AutoNav","Auto Nav");
    case EStarshatterInputAction::DropOrbit: return NSLOCTEXT("SSW.LegacyInputActions","DropOrbit","Drop Orbit");
    case EStarshatterInputAction::ShieldsUp: return NSLOCTEXT("SSW.LegacyInputActions","ShieldsUp","Shields Up");
    case EStarshatterInputAction::ShieldsDown: return NSLOCTEXT("SSW.LegacyInputActions","ShieldsDown","Shields Down");
    case EStarshatterInputAction::ShieldsFull: return NSLOCTEXT("SSW.LegacyInputActions","ShieldsFull","Shields Full");
    case EStarshatterInputAction::ShieldsZero: return NSLOCTEXT("SSW.LegacyInputActions","ShieldsZero","Shields Zero");
    case EStarshatterInputAction::SensorMode: return NSLOCTEXT("SSW.LegacyInputActions","SensorMode","Sensor Mode");
    case EStarshatterInputAction::SensorGroundMode: return NSLOCTEXT("SSW.LegacyInputActions","SensorGroundMode","Sensor Ground Mode");
    case EStarshatterInputAction::LaunchProbe: return NSLOCTEXT("SSW.LegacyInputActions","LaunchProbe","Launch Probe");
    case EStarshatterInputAction::SensorRangeMinus: return NSLOCTEXT("SSW.LegacyInputActions","SensorRangeMinus","Sensor Range -");
    case EStarshatterInputAction::SensorRangePlus: return NSLOCTEXT("SSW.LegacyInputActions","SensorRangePlus","Sensor Range +");
    case EStarshatterInputAction::EmconMinus: return NSLOCTEXT("SSW.LegacyInputActions","EmconMinus","EMCON -");
    case EStarshatterInputAction::EmconPlus: return NSLOCTEXT("SSW.LegacyInputActions","EmconPlus","EMCON +");
    case EStarshatterInputAction::CameraNextView: return NSLOCTEXT("SSW.LegacyInputActions","CameraNextView","Next Camera View");
    case EStarshatterInputAction::CamBridge: return NSLOCTEXT("SSW.LegacyInputActions","CamBridge","Bridge Camera");
    case EStarshatterInputAction::CamVirt: return NSLOCTEXT("SSW.LegacyInputActions","CamVirt","Virtual Camera");
    case EStarshatterInputAction::CamChase: return NSLOCTEXT("SSW.LegacyInputActions","CamChase","Chase Camera");
    case EStarshatterInputAction::CamDrop: return NSLOCTEXT("SSW.LegacyInputActions","CamDrop","Drop Camera");
    case EStarshatterInputAction::CamExtern: return NSLOCTEXT("SSW.LegacyInputActions","CamExtern","External Camera");
    case EStarshatterInputAction::CameraChase: return NSLOCTEXT("SSW.LegacyInputActions","CameraChase","Chase Camera (Alt)");
    case EStarshatterInputAction::CameraExternal: return NSLOCTEXT("SSW.LegacyInputActions","CameraExternal","External Camera (Alt)");
    case EStarshatterInputAction::CameraZoomIn: return NSLOCTEXT("SSW.LegacyInputActions","CameraZoomIn","Zoom In");
    case EStarshatterInputAction::CameraZoomOut: return NSLOCTEXT("SSW.LegacyInputActions","CameraZoomOut","Zoom Out");
    case EStarshatterInputAction::ZoomWide: return NSLOCTEXT("SSW.LegacyInputActions","ZoomWide","Zoom Wide");
    case EStarshatterInputAction::CamCycleObject: return NSLOCTEXT("SSW.LegacyInputActions","CamCycleObject","Cycle Camera Object");
    case EStarshatterInputAction::CamExtPlusAz: return NSLOCTEXT("SSW.LegacyInputActions","CamExtPlusAz","External Cam +Az");
    case EStarshatterInputAction::CamExtMinusAz: return NSLOCTEXT("SSW.LegacyInputActions","CamExtMinusAz","External Cam -Az");
    case EStarshatterInputAction::CamExtPlusEl: return NSLOCTEXT("SSW.LegacyInputActions","CamExtPlusEl","External Cam +El");
    case EStarshatterInputAction::CamExtMinusEl: return NSLOCTEXT("SSW.LegacyInputActions","CamExtMinusEl","External Cam -El");
    case EStarshatterInputAction::CamExtPlusRange: return NSLOCTEXT("SSW.LegacyInputActions","CamExtPlusRange","External Cam +Range");
    case EStarshatterInputAction::CamExtMinusRange: return NSLOCTEXT("SSW.LegacyInputActions","CamExtMinusRange","External Cam -Range");
    case EStarshatterInputAction::CamViewSelection: return NSLOCTEXT("SSW.LegacyInputActions","CamViewSelection","Camera View Selection");
    case EStarshatterInputAction::CamVirtPlusAz: return NSLOCTEXT("SSW.LegacyInputActions","CamVirtPlusAz","Virtual Cam +Az");
    case EStarshatterInputAction::CamVirtMinusAz: return NSLOCTEXT("SSW.LegacyInputActions","CamVirtMinusAz","Virtual Cam -Az");
    case EStarshatterInputAction::CamVirtPlusEl: return NSLOCTEXT("SSW.LegacyInputActions","CamVirtPlusEl","Virtual Cam +El");
    case EStarshatterInputAction::CamVirtMinusEl: return NSLOCTEXT("SSW.LegacyInputActions","CamVirtMinusEl","Virtual Cam -El");
    case EStarshatterInputAction::HudMode: return NSLOCTEXT("SSW.LegacyInputActions","HudMode","HUD Mode");
    case EStarshatterInputAction::HudColor: return NSLOCTEXT("SSW.LegacyInputActions","HudColor","HUD Color");
    case EStarshatterInputAction::HudWarn: return NSLOCTEXT("SSW.LegacyInputActions","HudWarn","HUD Warnings");
    case EStarshatterInputAction::HudInst: return NSLOCTEXT("SSW.LegacyInputActions","HudInst","HUD Instrumentation");
    case EStarshatterInputAction::NavDialog: return NSLOCTEXT("SSW.LegacyInputActions","NavDialog","Navigation Dialog");
    case EStarshatterInputAction::WeaponDialog: return NSLOCTEXT("SSW.LegacyInputActions","WeaponDialog","Weapon Dialog");
    case EStarshatterInputAction::FlightDialog: return NSLOCTEXT("SSW.LegacyInputActions","FlightDialog","Flight Dialog");
    case EStarshatterInputAction::EngineDialog: return NSLOCTEXT("SSW.LegacyInputActions","EngineDialog","Engine Dialog");
    case EStarshatterInputAction::RadioMenu: return NSLOCTEXT("SSW.LegacyInputActions","RadioMenu","Radio Menu");
    case EStarshatterInputAction::QuantumMenu: return NSLOCTEXT("SSW.LegacyInputActions","QuantumMenu","Quantum Menu");
    case EStarshatterInputAction::MFD1: return NSLOCTEXT("SSW.LegacyInputActions","MFD1","MFD 1");
    case EStarshatterInputAction::MFD2: return NSLOCTEXT("SSW.LegacyInputActions","MFD2","MFD 2");
    case EStarshatterInputAction::SelfDestruct: return NSLOCTEXT("SSW.LegacyInputActions","SelfDestruct","Self Destruct");
    case EStarshatterInputAction::SwapRollYaw: return NSLOCTEXT("SSW.LegacyInputActions","SwapRollYaw","Swap Roll / Yaw");
    case EStarshatterInputAction::CommAttackTgt: return NSLOCTEXT("SSW.LegacyInputActions","CommAttackTgt","Comm: Attack Target");
    case EStarshatterInputAction::CommEscortTgt: return NSLOCTEXT("SSW.LegacyInputActions","CommEscortTgt","Comm: Escort Target");
    case EStarshatterInputAction::CommWepFree: return NSLOCTEXT("SSW.LegacyInputActions","CommWepFree","Comm: Weapons Free");
    case EStarshatterInputAction::CommWepHold: return NSLOCTEXT("SSW.LegacyInputActions","CommWepHold","Comm: Weapons Hold");
    case EStarshatterInputAction::CommCoverMe: return NSLOCTEXT("SSW.LegacyInputActions","CommCoverMe","Comm: Cover Me");
    case EStarshatterInputAction::CommSkipNav: return NSLOCTEXT("SSW.LegacyInputActions","CommSkipNav","Comm: Skip Nav");
    case EStarshatterInputAction::CommReturnToBase: return NSLOCTEXT("SSW.LegacyInputActions","CommReturnToBase","Comm: Return To Base");
    case EStarshatterInputAction::CommCallInbound: return NSLOCTEXT("SSW.LegacyInputActions","CommCallInbound","Comm: Call Inbound");
    case EStarshatterInputAction::CommRequestPicture: return NSLOCTEXT("SSW.LegacyInputActions","CommRequestPicture","Comm: Request Picture");
    case EStarshatterInputAction::CommRequestSupport: return NSLOCTEXT("SSW.LegacyInputActions","CommRequestSupport","Comm: Request Support");
    case EStarshatterInputAction::ChatBroadcast: return NSLOCTEXT("SSW.LegacyInputActions","ChatBroadcast","Chat: Broadcast");
    case EStarshatterInputAction::ChatTeam: return NSLOCTEXT("SSW.LegacyInputActions","ChatTeam","Chat: Team");
    case EStarshatterInputAction::ChatWing: return NSLOCTEXT("SSW.LegacyInputActions","ChatWing","Chat: Wing");
    case EStarshatterInputAction::ChatUnit: return NSLOCTEXT("SSW.LegacyInputActions","ChatUnit","Chat: Unit");
    case EStarshatterInputAction::IncStardate: return NSLOCTEXT("SSW.LegacyInputActions","IncStardate","Increase Stardate");
    case EStarshatterInputAction::DecStardate: return NSLOCTEXT("SSW.LegacyInputActions","DecStardate","Decrease Stardate");
    default: return NSLOCTEXT("SSW.InputLabels","UnknownAction","Unknown Action");
    }
}

FText UStarshatterInputLabels::GetBindingKeyLabel(const FEnhancedActionKeyMapping& Mapping)
{
    if (!Mapping.Key.IsValid()) return NSLOCTEXT("SSW.InputLabels","Unbound","Unbound");
    TArray<FString> Parts;
    auto AddChords=[&Parts](const auto& Triggers)
    {
        for (const auto& Trigger : Triggers)
            if (const auto* Chord=Cast<UInputTriggerChordAction>(Trigger.Get()))
                if (Chord->ChordAction)
                {
                    FString Label=GetActionLabel(Chord->ChordAction).ToString();
                    Label.RemoveFromEnd(TEXT(" Modifier"));
                    Parts.AddUnique(Label);
                }
    };
    AddChords(Mapping.Triggers);
    if (Mapping.Action) AddChords(Mapping.Action->Triggers);
    Parts.Add(Mapping.Key.GetDisplayName().ToString());
    return FText::FromString(FString::Join(Parts,TEXT(" + ")));
}
