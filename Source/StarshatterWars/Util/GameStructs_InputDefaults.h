// GameStructs.h (or a new GameStructs_InputDefaults.h included by GameStructs.h)
//
// Purpose:
// - UE-native default key binds for Starshatter actions
// - Replaces giant switch blocks in UI and keeps defaults centralized
// - Supports modifiers (Shift/Ctrl/Alt) like your legacy VK_SHIFT/VK_CONTROL/VK_MENU patterns

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameStructs.h"      // EStarshatterInputAction
#include "GameStructs_InputDefaults.generated.h"

USTRUCT(BlueprintType)
struct FStarshatterKeyChord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    FKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    bool bShift = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    bool bCtrl = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    bool bAlt = false;

    FStarshatterKeyChord() = default;

    FStarshatterKeyChord(const FKey& InKey, bool bInShift=false, bool bInCtrl=false, bool bInAlt=false)
        : Key(InKey), bShift(bInShift), bCtrl(bInCtrl), bAlt(bInAlt)
    {}

    bool IsValid() const { return Key.IsValid(); }
};

USTRUCT(BlueprintType)
struct FStarshatterDefaultBind
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    EStarshatterInputAction Action = EStarshatterInputAction::Pause;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    FStarshatterKeyChord Chord;

    FStarshatterDefaultBind() = default;

    FStarshatterDefaultBind(EStarshatterInputAction InAction, const FStarshatterKeyChord& InChord)
        : Action(InAction), Chord(InChord)
    {}
};

namespace StarshatterInputDefaults
{
    // ------------------------------------------------------------
    // Single source of truth: defaults table (mirrors your switch)
    // ------------------------------------------------------------
    inline const TArray<FStarshatterDefaultBind>& GetDefaults()
    {
        static const TArray<FStarshatterDefaultBind> Defaults =
        {
            // Flight controls
            { EStarshatterInputAction::PitchUp,        { EKeys::Down } },         // VK_DOWN
            { EStarshatterInputAction::PitchDown,      { EKeys::Up } },           // VK_UP
            { EStarshatterInputAction::YawLeft,        { EKeys::Left } },         // VK_LEFT
            { EStarshatterInputAction::YawRight,       { EKeys::Right } },        // VK_RIGHT
            { EStarshatterInputAction::RollLeft,       { EKeys::NumPadSeven } },  // VK_NUMPAD7
            { EStarshatterInputAction::RollRight,      { EKeys::NumPadNine } },   // VK_NUMPAD9

            // PLUS/MINUS axes
            { EStarshatterInputAction::PlusX,          { EKeys::Period } },       // 190 '.'
            { EStarshatterInputAction::MinusX,         { EKeys::Comma } },        // 188 ','
            { EStarshatterInputAction::PlusY,          { EKeys::Home } },         // VK_HOME
            { EStarshatterInputAction::MinusY,         { EKeys::End } },          // VK_END
            { EStarshatterInputAction::PlusZ,          { EKeys::PageUp } },       // VK_PRIOR
            { EStarshatterInputAction::MinusZ,         { EKeys::PageDown } },     // VK_NEXT

            // Actions
            { EStarshatterInputAction::Action0,        { EKeys::LeftControl } },  // VK_CONTROL
            { EStarshatterInputAction::Action1,        { EKeys::SpaceBar } },     // VK_SPACE

            // Throttle
            { EStarshatterInputAction::ThrottleUp,     { EKeys::A } },            // 'A'
            { EStarshatterInputAction::ThrottleDown,   { EKeys::Z } },            // 'Z'
            { EStarshatterInputAction::ThrottleFull,   { EKeys::A, true } },      // 'A'+SHIFT
            { EStarshatterInputAction::ThrottleZero,   { EKeys::Z, true } },      // 'Z'+SHIFT
            { EStarshatterInputAction::Augmenter,      { EKeys::Tab } },          // VK_TAB
            { EStarshatterInputAction::FlcsModeAuto,   { EKeys::M } },            // 'M'
            { EStarshatterInputAction::CommandMode,    { EKeys::M, true } },      // 'M'+SHIFT

            // Weapons/targeting
            { EStarshatterInputAction::CyclePrimary,   { EKeys::BackSpace, true } }, // VK_BACK+SHIFT
            { EStarshatterInputAction::CycleSecondary, { EKeys::BackSpace } },       // VK_BACK

            // Weapons/targeting
            { EStarshatterInputAction::FirePrimary,    { EKeys::LeftControl } },     // VK_ACTION_ZERO
            { EStarshatterInputAction::FireSecondary,  { EKeys::SpaceBar  } },       // VK_BACK

            { EStarshatterInputAction::LockTarget,         { EKeys::T } },            // 'T'
            { EStarshatterInputAction::LockThreat,         { EKeys::T, true } },      // 'T'+SHIFT
            { EStarshatterInputAction::LockClosestShip,    { EKeys::U } },            // 'U'
            { EStarshatterInputAction::LockClosestThreat,  { EKeys::U, true } },      // 'U'+SHIFT
            { EStarshatterInputAction::LockHostileShip,    { EKeys::Y } },            // 'Y'
            { EStarshatterInputAction::LockHostileThreat,  { EKeys::Y, true } },      // 'Y'+SHIFT

            // Subtarget cycle (best-effort OEM mapping)
            { EStarshatterInputAction::CycleSubtarget, { EKeys::Semicolon } },
            { EStarshatterInputAction::PrevSubtarget,  { EKeys::Semicolon, true } },

            // Decoy / gear / lights
            { EStarshatterInputAction::Decoy,          { EKeys::D } },            // 'D'
            { EStarshatterInputAction::GearToggle,     { EKeys::G } },            // 'G'
            { EStarshatterInputAction::NavlightToggle, { EKeys::L } },            // 'L'
            { EStarshatterInputAction::AutoNav,        { EKeys::N, true } },      // 'N'+SHIFT
            { EStarshatterInputAction::DropOrbit,      { EKeys::O } },            // 'O'

            // Shields
            { EStarshatterInputAction::ShieldsUp,      { EKeys::S } },            // 'S'
            { EStarshatterInputAction::ShieldsDown,    { EKeys::X } },            // 'X'
            { EStarshatterInputAction::ShieldsFull,    { EKeys::S, true } },      // 'S'+SHIFT
            { EStarshatterInputAction::ShieldsZero,    { EKeys::X, true } },      // 'X'+SHIFT

            // Sensors
            { EStarshatterInputAction::SensorMode,         { EKeys::F5 } },
            { EStarshatterInputAction::SensorGroundMode,   { EKeys::F5, true } },
            { EStarshatterInputAction::LaunchProbe,        { EKeys::F6 } },
            { EStarshatterInputAction::SensorRangeMinus,   { EKeys::F7 } },
            { EStarshatterInputAction::SensorRangePlus,    { EKeys::F8 } },
            { EStarshatterInputAction::EmconMinus,         { EKeys::F9 } },
            { EStarshatterInputAction::EmconPlus,          { EKeys::F10 } },

            // Exit/pause
            { EStarshatterInputAction::ExitGame,       { EKeys::Escape } },
            { EStarshatterInputAction::Pause,          { EKeys::Pause } },

            // Time
            { EStarshatterInputAction::TimeExpand,     { EKeys::Delete } },
            { EStarshatterInputAction::TimeCompress,   { EKeys::Insert } },
            { EStarshatterInputAction::TimeSkip,       { EKeys::Insert, true } },

            // Cameras
            { EStarshatterInputAction::CamBridge,      { EKeys::F1 } },
            { EStarshatterInputAction::CamVirt,        { EKeys::F1, true } },
            { EStarshatterInputAction::CamChase,       { EKeys::F2 } },
            { EStarshatterInputAction::CamDrop,        { EKeys::F2, true } },
            { EStarshatterInputAction::CamExtern,      { EKeys::F3 } },
            { EStarshatterInputAction::TargetPadlock,  { EKeys::F4 } },

            // HUD + dialogs
            { EStarshatterInputAction::ZoomWide,       { EKeys::K } },
            { EStarshatterInputAction::HudMode,        { EKeys::H } },
            { EStarshatterInputAction::HudColor,       { EKeys::H, true } },
            { EStarshatterInputAction::HudWarn,        { EKeys::C } },
            { EStarshatterInputAction::HudInst,        { EKeys::I } },
            { EStarshatterInputAction::NavDialog,      { EKeys::N } },
            { EStarshatterInputAction::WeaponDialog,   { EKeys::W } },
            { EStarshatterInputAction::EngineDialog,   { EKeys::E } },
            { EStarshatterInputAction::FlightDialog,   { EKeys::F } },
            { EStarshatterInputAction::RadioMenu,      { EKeys::R } },
            { EStarshatterInputAction::QuantumMenu,    { EKeys::Q } },

            // MFD
            { EStarshatterInputAction::MFD1,           { EKeys::LeftBracket } },
            { EStarshatterInputAction::MFD2,           { EKeys::RightBracket } },

            // Self destruct (ESC + SHIFT)
            { EStarshatterInputAction::SelfDestruct,   { EKeys::Escape, true } },

            // Swap roll/yaw
            { EStarshatterInputAction::SwapRollYaw,    { EKeys::J } },

            // Comms (ALT)
            { EStarshatterInputAction::CommAttackTgt,      { EKeys::A, false, false, true } },
            { EStarshatterInputAction::CommEscortTgt,      { EKeys::E, false, false, true } },
            { EStarshatterInputAction::CommWepFree,        { EKeys::B, false, false, true } },
            { EStarshatterInputAction::CommWepHold,        { EKeys::F, false, false, true } },
            { EStarshatterInputAction::CommCoverMe,        { EKeys::H, false, false, true } },
            { EStarshatterInputAction::CommSkipNav,        { EKeys::N, false, false, true } },
            { EStarshatterInputAction::CommReturnToBase,   { EKeys::R, false, false, true } },
            { EStarshatterInputAction::CommCallInbound,    { EKeys::I, false, false, true } },
            { EStarshatterInputAction::CommRequestPicture, { EKeys::P, false, false, true } },
            { EStarshatterInputAction::CommRequestSupport, { EKeys::S, false, false, true } },

            // Chat (ALT+1..4)
            { EStarshatterInputAction::ChatBroadcast,  { EKeys::One,   false, false, true } },
            { EStarshatterInputAction::ChatTeam,       { EKeys::Two,   false, false, true } },
            { EStarshatterInputAction::ChatWing,       { EKeys::Three, false, false, true } },
            { EStarshatterInputAction::ChatUnit,       { EKeys::Four,  false, false, true } },
        };

        return Defaults;
    }

    // ------------------------------------------------------------
    // Query helpers
    // ------------------------------------------------------------
    inline bool GetDefaultKeyChord(EStarshatterInputAction Action, FStarshatterKeyChord& OutChord)
    {
        const TArray<FStarshatterDefaultBind>& D = GetDefaults();
        for (const FStarshatterDefaultBind& E : D)
        {
            if (E.Action == Action)
            {
                OutChord = E.Chord;
                return OutChord.IsValid();
            }
        }
        return false;
    }

    // Convenience: legacy-style out params (matches your switch signature)
    inline bool GetDefaultKey(
        EStarshatterInputAction Action,
        FKey& OutKey,
        bool& bOutShift,
        bool& bOutCtrl,
        bool& bOutAlt)
    {
        FStarshatterKeyChord Chord;
        if (!GetDefaultKeyChord(Action, Chord))
            return false;

        OutKey = Chord.Key;
        bOutShift = Chord.bShift;
        bOutCtrl  = Chord.bCtrl;
        bOutAlt   = Chord.bAlt;
        return true;
    }
}
