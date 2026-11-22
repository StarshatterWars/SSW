#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "DataLoader.h"
#include "Bitmap.h"
#include "Engine/Texture2D.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDImportedTextureTest,"Starshatter.HUD.ImportedTextureAssets",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHUDImportedTextureTest::RunTest(const FString&)
{
    DataLoader Loader;
    const char* Names[]={"sensor_fov","sensor_fwd","sensor_hsd","sensor_3d","ILS","HUDleft","HUDright","hud_icon","Cross","Ladder1","CHASE_L"};
    for(const char* Name:Names)
    {
        Bitmap Image;
        TestEqual(TEXT("Imported HUD asset resolves"),Loader.LoadHUDTexture(Name,Image,Bitmap::BMP_TRANSLUCENT),1);
        auto* Texture=Image.GetTexture();
        if(TestNotNull(TEXT("GPU texture retained"),Texture))
        {
            TestTrue(TEXT("Uses project asset, not a reuploaded copy"),Texture->GetPathName().StartsWith(TEXT("/Game/UI/HUD/")));
            TestTrue(TEXT("Dimensions retained"),Image.Width()>0 && Image.Height()>0);
        }
        TestNull(TEXT("No CPU pixel readback needed"),Image.HiPixels());
    }
    return true;
}
#endif
