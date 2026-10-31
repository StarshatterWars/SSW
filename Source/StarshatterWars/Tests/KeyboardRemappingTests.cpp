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
    auto* Supplemental=StarshatterInputBindings::SupplementalContext(Owner.Get());
    for(const auto& Mapping:Supplemental->GetMappings())
    {
        Config.EnhancedBindings.Add(StarshatterInputBindings::Id(Mapping),EKeys::F10);Settings->SetKeyboardConfig(Config);
        TestEqual(TEXT("C++ controls honor saved overrides"),StarshatterInputBindings::ResolvedKey(Mapping.Action->GetName(),Mapping.Key),EKeys::F10);
    }
    auto* Save=NewObject<UStarshatterSettingsSaveGame>();Save->KeyboardConfig=Config;Save->KeyboardConfig.EnhancedBindings.Add(NegativeId,EKeys::F10);
    const FString Slot=TEXT("Automation/KeyboardRemapping/")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("/options");
    ON_SCOPE_EXIT {UGameplayStatics::DeleteGameInSlot(Slot,0);};
    TestTrue(TEXT("Save isolated options.sav"),UGameplayStatics::SaveGameToSlot(Save,Slot,0));
    auto* Loaded=Cast<UStarshatterSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (TestNotNull(TEXT("Reload options payload"),Loaded)) TestEqual(TEXT("Enhanced binding survives options save/reload"),Loaded->KeyboardConfig.EnhancedBindings.FindRef(NegativeId),EKeys::F10);

    Settings->SetKeyboardConfig(Previous);StarshatterInputBindings::Refresh();
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
    int32 DisplayedRows=0;for(const auto& Pair:Page->CategoryBoxes) if(Pair.Value.IsValid()) DisplayedRows+=Pair.Value->GetChildrenCount();
    TestEqual(TEXT("Only actual bindings are displayed, without legacy rows"),DisplayedRows,Page->EnhancedRows.Num());
    for (const auto& Pair:Page->EnhancedRows)
    {
        const FString Name=Pair.Value.Mapping.Action->GetName();
        TestFalse(TEXT("Obsolete camera aliases removed"),Name==TEXT("IA_ViewCockpit") || Name==TEXT("IA_ViewVirtualCockpit") || Name==TEXT("IA_ViewChase") || Name==TEXT("IA_ViewOrbit"));
        auto* Caption=Cast<UTextBlock>(Pair.Value.Button->GetContent());
        if(TestNotNull(TEXT("Binding button has caption"),Caption)) TestFalse(TEXT("Keys only appear in key column"),Caption->GetText().ToString().Contains(TEXT("[")));
    }
    AddInfo(FString::Printf(TEXT("Remappable real binding buttons: %d"),Page->EnhancedRows.Num()));
    Page->Cancel();
    TestEqual(TEXT("Cancel discards all enhanced edits"),Page->PendingEnhanced.Num(),0);
    TestTrue(TEXT("Cancel leaves settings unchanged"),Settings->GetKeyboardConfig().EnhancedBindings.OrderIndependentCompareEqual(Previous.EnhancedBindings));
    return true;
}
#endif
