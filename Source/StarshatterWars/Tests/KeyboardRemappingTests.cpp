#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarshatterInputBindings.h"
#include "StarshatterKeyboardSettings.h"
#include "StarshatterSettingsSaveGame.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "KeyDlg.h"
#include "KeyMap.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeyboardRemappingTest,"Starshatter.Options.KeyboardRemapping",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FKeyboardRemappingTest::RunTest(const FString& Parameters)
{
    auto* Settings=UStarshatterKeyboardSettings::Get();Settings->Load();
    const auto Previous=Settings->GetKeyboardConfig();
    ON_SCOPE_EXIT {Settings->SetKeyboardConfig(Previous);StarshatterInputBindings::Refresh();};
    TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
    TStrongObjectPtr<UInputMappingContext> Source(NewObject<UInputMappingContext>());
    auto* Action=NewObject<UInputAction>(Owner.Get(),TEXT("IA_TestCamera"));Action->ValueType=EInputActionValueType::Axis1D;
    auto* ModifierAction=NewObject<UInputAction>(Owner.Get(),TEXT("IA_TestModifier"));
    auto& Negative=Source->MapKey(Action,EKeys::Left);
    auto* Negate=NewObject<UInputModifierNegate>(Source.Get());Negative.Modifiers.Add(Negate);
    auto* Chord=NewObject<UInputTriggerChordAction>(Source.Get());Chord->ChordAction=ModifierAction;Negative.Triggers.Add(Chord);
    const FString NegativeId=StarshatterInputBindings::Id(Negative);
    Source->MapKey(Action,EKeys::Right);
    TestNotEqual(TEXT("Axis directions have independent IDs"),NegativeId,StarshatterInputBindings::Id(Source->GetMappings()[1]));
    const auto Before=Source->GetMappings();
    auto Config=Previous;Config.EnhancedBindings.Add(NegativeId,EKeys::F8);Settings->SetKeyboardConfig(Config);
    auto* Runtime=StarshatterInputBindings::RuntimeContext(Source.Get(),Owner.Get());
    TestTrue(TEXT("Runtime is separate from source"),Runtime!=Source.Get());
    TestEqual(TEXT("Override applied"),Runtime->GetMappings()[0].Key,EKeys::F8);
    TestEqual(TEXT("Other axis binding retained"),Runtime->GetMappings()[1].Key,EKeys::Right);
    TestTrue(TEXT("Source mappings retained"),Source->GetMappings()==Before);
    TestTrue(TEXT("Negate modifier retained"),Runtime->GetMappings()[0].Modifiers.Num()==1 && Runtime->GetMappings()[0].Modifiers[0]->IsA<UInputModifierNegate>());
    TestTrue(TEXT("Chord action retained"),CastChecked<UInputTriggerChordAction>(Runtime->GetMappings()[0].Triggers[0])->ChordAction==ModifierAction);
    auto* Derived=NewObject<UInputMappingContext>(Owner.Get());
    Derived->MapKey(Action,EKeys::F8)=Runtime->GetMappings()[0];StarshatterInputBindings::RegisterContext(Derived);
    Config.EnhancedBindings.Add(NegativeId,EKeys::F9);Settings->SetKeyboardConfig(Config);StarshatterInputBindings::Refresh();
    TestEqual(TEXT("Live context refreshed"),Runtime->GetMappings()[0].Key,EKeys::F9);
    TestEqual(TEXT("Higher priority copied context refreshed"),Derived->GetMappings()[0].Key,EKeys::F9);
    Config.EnhancedBindings.Remove(NegativeId);Settings->SetKeyboardConfig(Config);StarshatterInputBindings::Refresh();
    TestEqual(TEXT("Reset restores source key"),Runtime->GetMappings()[0].Key,EKeys::Left);
    TestEqual(TEXT("Copied context reset restores original key"),Derived->GetMappings()[0].Key,EKeys::Left);
    Action->ActionDescription=FText::FromString(TEXT("New readable label"));
    TestEqual(TEXT("Labels do not invalidate saved IDs"),StarshatterInputBindings::Id(Source->GetMappings()[0]),NegativeId);

    for (const TCHAR* Path:{TEXT("/Game/Input/IMC_Game.IMC_Game"),TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD")})
    {
        auto* Authored=LoadObject<UInputMappingContext>(nullptr,Path);
        if(!TestNotNull(TEXT("Authored context loads"),Authored)) continue;
        auto* Copy=StarshatterInputBindings::RuntimeContext(Authored,Owner.Get());
        for(int32 Index=0;Index<Authored->GetMappings().Num();++Index)
        {
            const auto& Original=Authored->GetMappings()[Index];const auto& Copied=Copy->GetMappings()[Index];
            TestTrue(TEXT("Runtime copy shares authored action asset"),Copied.Action==Original.Action);
            TestEqual(TEXT("Authored modifier count retained"),Copied.Modifiers.Num(),Original.Modifiers.Num());
            for(int32 Trigger=0;Trigger<Original.Triggers.Num();++Trigger)
                if(auto* OriginalChord=Cast<UInputTriggerChordAction>(Original.Triggers[Trigger]))
                    TestTrue(TEXT("Authored chord action retained"),CastChecked<UInputTriggerChordAction>(Copied.Triggers[Trigger])->ChordAction==OriginalChord->ChordAction);
        }
    }
    KeyMap Legacy;Legacy.DefaultKeyMap();const int32 PitchIndex=Legacy.FindMapIndex(KEY_PITCH_UP);
    const int32 OriginalPitch=Legacy.GetKeyMap(PitchIndex)->key;
    Config.RemappedKeys.Add(EStarshatterInputAction::PitchUp,EKeys::F9);
    StarshatterInputBindings::ApplyLegacy(Legacy,Config,Owner.Get());
    TestNotEqual(TEXT("Legacy controller key actually changes"),Legacy.GetKeyMap(PitchIndex)->key,OriginalPitch);
    Config.RemappedKeys.Remove(EStarshatterInputAction::PitchUp);StarshatterInputBindings::ApplyLegacy(Legacy,Config,Owner.Get());
    TestEqual(TEXT("Legacy reset restores original mapping"),Legacy.GetKeyMap(PitchIndex)->key,OriginalPitch);

    auto* Save=NewObject<UStarshatterSettingsSaveGame>();Save->KeyboardConfig=Config;Save->KeyboardConfig.EnhancedBindings.Add(NegativeId,EKeys::F10);
    const FString Slot=TEXT("Automation/KeyboardRemapping/")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("/options");
    ON_SCOPE_EXIT {UGameplayStatics::DeleteGameInSlot(Slot,0);};
    TestTrue(TEXT("Save isolated options.sav"),UGameplayStatics::SaveGameToSlot(Save,Slot,0));
    auto* Loaded=Cast<UStarshatterSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (TestNotNull(TEXT("Reload options payload"),Loaded)) TestEqual(TEXT("Enhanced binding survives options save/reload"),Loaded->KeyboardConfig.EnhancedBindings.FindRef(NegativeId),EKeys::F10);

    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);World->SetGameInstance(NewObject<UGameInstance>());
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    auto* Class=LoadClass<UKeyDlg>(nullptr,TEXT("/Game/Screens/Options/WBP_KeyboardDlg.WBP_KeyboardDlg_C"));
    if (!TestNotNull(TEXT("Authored keyboard page loads"),Class)) return false;
    auto* Page=NewObject<UKeyDlg>(World,Class);Page->Initialize();Page->TakeWidget();
    TestFalse(TEXT("Opening page does not capture a key"),Page->bCapturing);
    TestTrue(TEXT("Camera and HUD mappings have remap buttons"),Page->EnhancedRows.Num()>0);
    TestTrue(TEXT("Movement section exists"),Page->CategoryBoxes.Contains(TEXT("MOVEMENT & FLIGHT")));
    TestTrue(TEXT("Camera section exists"),Page->CategoryBoxes.Contains(TEXT("CAMERA & VIEWS")));
    for (const auto& Pair:Page->EnhancedRows)
    {
        TestNotNull(TEXT("Every enhanced row is a button"),Pair.Value.Button.Get());
        Page->SelectedEnhancedId=Pair.Key;Page->bHasSelection=true;Page->BeginCaptureForSelected();
        TestTrue(TEXT("Every enhanced control accepts a replacement"),Page->CaptureKey(EKeys::F11));
    }
    const uint32* F12Code=nullptr;const uint32* CharacterCode=nullptr;
    FInputKeyManager::Get().GetCodesFromKey(EKeys::F12,F12Code,CharacterCode);
    for(const auto& Pair:Page->RowMap)
    {
        Page->SelectAction(Pair.Key);TestTrue(TEXT("Every legacy row accepts a key"),Page->CaptureKey(EKeys::F12));
        FStarshatterKeyboardConfig OneBinding;OneBinding.RemappedKeys.Add(Pair.Key,EKeys::F12);
        StarshatterInputBindings::ApplyLegacy(Legacy,OneBinding,Owner.Get());
        bool bApplied=false;
        for(int32 Index=0;Index<Legacy.GetNumKeys();++Index) if(F12Code && Legacy.GetKeyMap(Index)->key==static_cast<int32>(*F12Code)) bApplied=true;
        TestTrue(*FString::Printf(TEXT("Legacy row has working runtime bridge: %s"),*StaticEnum<EStarshatterInputAction>()->GetNameStringByValue(static_cast<int64>(Pair.Key))),bApplied);
    }
    AddInfo(FString::Printf(TEXT("Remappable buttons: %d enhanced, %d legacy"),Page->EnhancedRows.Num(),Page->RowMap.Num()));
    Page->Cancel();TestEqual(TEXT("Cancel discards all legacy edits"),Page->PendingRemaps.Num(),0);
    TestEqual(TEXT("Cancel discards all enhanced edits"),Page->PendingEnhanced.Num(),0);
    TestTrue(TEXT("Cancel leaves settings unchanged"),Settings->GetKeyboardConfig().EnhancedBindings.OrderIndependentCompareEqual(Previous.EnhancedBindings));
    return true;
}
#endif
