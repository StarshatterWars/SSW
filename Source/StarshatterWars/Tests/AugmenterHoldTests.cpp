#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarshatterInputBindings.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputTriggers.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAugmenterHoldTest,"Starshatter.Input.AugmenterHold",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAugmenterHoldTest::RunTest(const FString& Parameters)
{
    auto* Context=NewObject<UInputMappingContext>();
    auto* Action=NewObject<UInputAction>(Context,TEXT("IA_Augmenter"));
    Action->ValueType=EInputActionValueType::Boolean;
    auto* Authored=NewObject<UInputTriggerHold>(Context);
    Authored->HoldTimeThreshold=1.f;Authored->bIsOneShot=true;
    auto& Mapping=Context->MapKey(Action,EKeys::J);Mapping.Triggers.Add(Authored);
    StarshatterInputBindings::ConfigureAugmenterHold(Mapping,Context);
    auto* Hold=Cast<UInputTriggerHold>(Mapping.Triggers[0]);
    TestTrue(TEXT("Existing remapped key retained"),Mapping.Key==EKeys::J);
    TestTrue(TEXT("Authored trigger unchanged"),Authored->bIsOneShot && Authored->HoldTimeThreshold==1.f);
    if(!TestNotNull(TEXT("Runtime hold trigger"),Hold))return false;
    for(int32 I=0;I<120;++I)
        if(Hold->UpdateState(nullptr,FInputActionValue(true),1.f/60.f)!=ETriggerState::Triggered)
            {AddError(TEXT("Augmenter failed to trigger continuously while held"));break;}
    TestTrue(TEXT("Release stops augmenter"),Hold->UpdateState(nullptr,FInputActionValue(false),1.f/60.f)==ETriggerState::None);
    TestTrue(TEXT("Press again activates immediately"),Hold->UpdateState(nullptr,FInputActionValue(true),1.f/60.f)==ETriggerState::Triggered);
    return true;
}
#endif
