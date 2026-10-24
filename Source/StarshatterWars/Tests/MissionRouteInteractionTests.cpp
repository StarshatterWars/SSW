#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissionRouteMap.h"
#include "Mission.h"
#include "MissionElement.h"
#include "Instruction.h"
#include "Widgets/SWidget.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionRouteInteractionTest,"Starshatter.Audit.F20.RouteInteraction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionRouteInteractionTest::RunTest(const FString& Parameters)
{
    Mission Model;auto* E=new MissionElement;E->SetName("Route test");E->SetLocation(FVector(0,500,0));Model.GetElements().append(E);
    auto* Map=NewObject<UMissionRouteMap>();Map->Model=&Model;auto Widget=Map->TakeWidget();
    const auto G=FGeometry::MakeRoot(FVector2D(700,400),FSlateLayoutTransform());
    const TSet<FKey> Pressed{EKeys::LeftMouseButton};const FModifierKeysState NoModifiers;
    const FModifierKeysState Shift(true,false,false,false,false,false,false,false,false);
    auto Click=[&](FVector2D P,const FModifierKeysState& Mods){FPointerEvent Event(0,P,P,Pressed,EKeys::LeftMouseButton,0,Mods);Widget->OnMouseButtonDown(G,Event);Widget->OnMouseButtonUp(G,Event);};
    Click(FVector2D(350,200),NoModifiers);Click(FVector2D(400,210),Shift);
    if(!TestEqual(TEXT("Shift click creates a waypoint on selected flight"),E->NavList().size(),1)){Map->Model=nullptr;return false;}
    auto* N=E->NavList()[0];TestTrue(TEXT("Map preserves altitude while converting horizontal axes"),N->GetLocation().Equals(FVector(50000,500,-10000)));
    FKeyEvent Raise(EKeys::Up,NoModifiers,0,false,0,0);Widget->OnKeyDown(G,Raise);
    TestEqual(TEXT("Altitude editing changes the actual mission instruction"),N->GetLocation().Y,600.);
    FKeyEvent Delete(EKeys::Delete,NoModifiers,0,false,0,0);Widget->OnKeyDown(G,Delete);
    TestEqual(TEXT("Delete removes selected route point"),E->NavList().size(),0);
    Map->Model=nullptr;return true;
}
#endif
