#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "QuitMissionMenu.h"
#include "InputCoreTypes.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuitLocalKeysTest,"Starshatter.Menu.QuitLocalNumberKeys",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FQuitLocalKeysTest::RunTest(const FString& Parameters)
{
    auto* Menu=NewObject<UQuitMissionMenu>();Menu->SetDialogInputEnabled(true);
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Five,EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFive};
    int32 Count=0;EMissionQuitAction Last=EMissionQuitAction::None;
    Menu->OnActionRequested.BindLambda([&](EMissionQuitAction Action){Last=Action;++Count;});
    for(int32 I=0;I<8;++I){FKeyEvent Event(Keys[I],FModifierKeysState(),0,false,0,0);
        const auto Reply=I<4?Menu->NativeOnKeyDown(FGeometry(),Event):Menu->NativeOnPreviewKeyDown(FGeometry(),Event);
        TestTrue(TEXT("Local number key consumed"),Reply.IsEventHandled());TestEqual(TEXT("Number maps to matching action"),static_cast<int32>(Last),(I%4==3?5:I%4+1));}
    TestEqual(TEXT("One action per press"),Count,8);
    FKeyEvent Retired(EKeys::Four,FModifierKeysState(),0,false,0,0);
    TestFalse(TEXT("Retired Controls action is not a hidden shortcut"),Menu->NativeOnKeyDown(FGeometry(),Retired).IsEventHandled());
    FKeyEvent Repeat(EKeys::Five,FModifierKeysState(),0,true,0,0);Menu->NativeOnKeyDown(FGeometry(),Repeat);
    TestEqual(TEXT("Held exit key cannot repeat the action"),Count,8);
    Menu->SetDialogInputEnabled(false);FKeyEvent Hidden(EKeys::One,FModifierKeysState(),0,false,0,0);
    TestFalse(TEXT("Inactive dialog does not consume flight keys"),Menu->NativeOnKeyDown(FGeometry(),Hidden).IsEventHandled());TestEqual(TEXT("Inactive dialog fires no action"),Count,8);
    Menu->OnActionRequested.Unbind();return true;
}
#endif
