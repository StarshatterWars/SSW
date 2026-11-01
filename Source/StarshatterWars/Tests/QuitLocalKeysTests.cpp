#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "QuitMissionMenu.h"
#include "InputCoreTypes.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuitLocalKeysTest,"Starshatter.Menu.QuitLocalNumberKeys",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FQuitLocalKeysTest::RunTest(const FString& Parameters)
{
    auto* Menu=NewObject<UQuitMissionMenu>();Menu->SetDialogInputEnabled(true);
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive};
    int32 Count=0;EMissionQuitAction Last=EMissionQuitAction::None;
    Menu->OnActionRequested.BindLambda([&](EMissionQuitAction Action){Last=Action;++Count;});
    for(int32 I=0;I<10;++I){FKeyEvent Event(Keys[I],FModifierKeysState(),0,false,0,0);
        const auto Reply=I<5?Menu->NativeOnKeyDown(FGeometry(),Event):Menu->NativeOnPreviewKeyDown(FGeometry(),Event);
        TestTrue(TEXT("Local number key consumed"),Reply.IsEventHandled());TestEqual(TEXT("Number maps to matching action"),static_cast<int32>(Last),I%5+1);}
    TestEqual(TEXT("One action per press"),Count,10);
    FKeyEvent Repeat(EKeys::Five,FModifierKeysState(),0,true,0,0);Menu->NativeOnKeyDown(FGeometry(),Repeat);
    TestEqual(TEXT("Held exit key cannot repeat the action"),Count,10);
    Menu->SetDialogInputEnabled(false);FKeyEvent Hidden(EKeys::One,FModifierKeysState(),0,false,0,0);
    TestFalse(TEXT("Inactive dialog does not consume flight keys"),Menu->NativeOnKeyDown(FGeometry(),Hidden).IsEventHandled());TestEqual(TEXT("Inactive dialog fires no action"),Count,10);
    Menu->OnActionRequested.Unbind();return true;
}
#endif
