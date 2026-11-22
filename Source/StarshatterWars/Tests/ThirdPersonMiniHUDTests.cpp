#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FighterHUDDetails.h"
#include "FighterHUDPanels.h"
#include "Widgets/SOverlay.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "AssetCompilingManager.h"
#include "ContentStreaming.h"
#include "Engine/DataTable.h"
#include "GameStructs_System.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "ShaderCompiler.h"
#include "MiniHUDLayout.h"
#include "Widgets/Layout/SBox.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThirdPersonMiniHUDTest,"Starshatter.HUD.ThirdPersonMiniHUD",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FThirdPersonMiniHUDTest::RunTest(const FString&)
{
    if(GUsingNullRHI){AddWarning(TEXT("Render comparison requires an RHI."));return true;}
    bool ThirdPerson=false;
    auto Details=SNew(SFighterHUDDetails).ShowHUD(true).ThirdPerson_Lambda([&]{return ThirdPerson;});
    Details->NextSample=DBL_MAX;Details->bHasPlayer=true;Details->PlayerName=TEXT("Delta 1");Details->PlayerClass=TEXT("Falcon");
    Details->Primary=TEXT("GUNS: DELTA");Details->Secondary=TEXT("SRM 4");Details->FlightSpeed=500;Details->HeadingDegrees=90;
    Details->MissionReadouts={TEXT("00:12:34  1X"),TEXT("JANEK"),TEXT("ACTIVE")};
    Details->NavReadouts={TEXT("AUTO NAV 1"),TEXT("VECTOR SPD 500")};Details->CompactNavLabel=TEXT("NAV 01  JANEK");Details->StatusRows={{TEXT("THRUST"),50},{TEXT("FUEL"),100},{TEXT("HULL"),100},{TEXT("GUNS"),99}};
    auto* PreviewTable=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    const auto* PreviewDesign=PreviewTable?PreviewTable->FindRow<FShipDesign>(TEXT("Falcon"),TEXT("Damage preview fixture")):nullptr;
    if(PreviewDesign){Details->PlayerHull=83;Details->PlayerMFDPreview=MakeUnique<FShipMFDPreview>(true);Details->PlayerMFDPreview->SetDesign(PreviewDesign);}
    auto Deck=SNew(SOverlay)
        +SOverlay::Slot()[SNew(SFighterHUDPanels).ShowPanels_Lambda([&]{return !ThirdPerson;})]
        +SOverlay::Slot()[Details];
    TSharedPtr<SBox> FixedFrame;auto Root=SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)[SAssignNew(FixedFrame,SBox).WidthOverride(1280).HeightOverride(720)[Deck]];
    FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();
    if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    IStreamingManager::Get().StreamAllResources(0);FlushRenderingCommands();
    if(PreviewDesign){FPlatformProcess::Sleep(1.1f);Details->PlayerMFDPreview->SetDesign(PreviewDesign);FlushRenderingCommands();}
    FWidgetRenderer* Renderer=new FWidgetRenderer(true);
    TArray<FColor> Frames[2];
    FString Capture;FParse::Value(FCommandLine::Get(),TEXT("MiniHUDCapture="),Capture);
    for(int32 I=0;I<2;++I){
        ThirdPerson=I==1;
        TStrongObjectPtr<UTextureRenderTarget2D> RT(FWidgetRenderer::CreateTargetFor(FVector2D(1280,720),TF_Bilinear,true));RT->ClearColor=FLinearColor::Transparent;
        Renderer->DrawWidget(RT.Get(),Root,FVector2D(1280,720),0);FlushRenderingCommands();
        FReadSurfaceDataFlags Flags;Flags.SetLinearToGamma(false);RT->GameThread_GetRenderTargetResource()->ReadPixels(Frames[I],Flags);
        if(!Capture.IsEmpty()){TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1280,720,Frames[I],PNG);TestTrue(TEXT("HUD preview saved"),FFileHelper::SaveArrayToFile(PNG,*(Capture+(I?TEXT("thirdperson.png"):TEXT("full.png")))));}
    }
    int32 FullCentre=0,MiniCentre=0,MiniReadout=0,FullLeftMFD=0,MiniLeftMFD=0,FullSensorMFD=0,SensorDifferences=0;
    for(int32 Y=0;Y<720;++Y)for(int32 X=0;X<1280;++X){const int32 I=Y*1280+X;
        if(X>380 && X<900 && Y>180 && Y<490){if(Frames[0][I].A>10)++FullCentre;if(Frames[1][I].A>10)++MiniCentre;}
        if(X>350 && X<930 && Y<180 && Frames[1][I].A>10)++MiniReadout;
        if (Y>=505 && X<220)
        {
            if (Frames[0][I].A>10) ++FullLeftMFD;
            if (Frames[1][I].A>10) ++MiniLeftMFD;
        }
        if (Y>=505 && X>=1060)
        {
            if (Frames[0][I].A>10) ++FullSensorMFD;
            if (Frames[0][I]!=Frames[1][I]) ++SensorDifferences;
        }
    }
    TestTrue(TEXT("Original central cockpit/virtual HUD is retained"),FullCentre>1000);
    TestEqual(TEXT("External view clears large central HUD"),MiniCentre,0);
    TestTrue(TEXT("External mini flight deck is visible at top center"),MiniReadout>100);
    TestTrue(TEXT("Full HUD retains the bottom status and sensor panels"),FullLeftMFD>1000 && FullSensorMFD>1000);
    TestEqual(TEXT("Third-person HUD removes the separate ship status panel"),MiniLeftMFD,0);
    TestEqual(TEXT("Third-person HUD preserves the sensor panel"),SensorDifferences,0);
    Details->CycleMFD(0);TestTrue(TEXT("Consolidated left information cannot be cycled away"),Details->Modes[0]==SFighterHUDDetails::EMode::Ship);
    const auto SensorMode=Details->Modes[1];Details->CycleMFD(1);TestTrue(TEXT("Right sensor retains its cycling control"),Details->Modes[1]!=SensorMode);
    bool CameraScrolled=false;Details->OnOverviewWheel.BindLambda([&](float){CameraScrolled=true;return true;});
    const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1280,720),FSlateLayoutTransform());
    const FVector2D OverModel(850,65);FPointerEvent Hover(0,OverModel,OverModel,TSet<FKey>(),FKey(),0,FModifierKeysState());
    TestFalse(TEXT("Hover does not open an inspection popup"),Details->OnMouseMove(Geometry,Hover).IsEventHandled());
    auto* Designs=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    const auto* Falcon=Designs?Designs->FindRow<FShipDesign>(TEXT("Falcon"),TEXT("Damage inspection")):nullptr;
    if(TestNotNull(TEXT("Inspection uses the real Falcon model"),Falcon))
    {
        Details->PlayerHull=83;
        TestTrue(TEXT("Damage model renders"),Details->PlayerMFDPreview->SetDesign(Falcon));
        Details->DamagePoints.Add({SSWDamagePanel::DefaultLocation(EShipSystemIcon::Engine),35,TEXT("Engine"),EShipSystemIcon::Engine});
        Details->DamagePoints.Add({SSWDamagePanel::DefaultLocation(EShipSystemIcon::Sensor),100,TEXT("Sensor"),EShipSystemIcon::Sensor});
        FAssetCompilingManager::Get().FinishAllCompilation();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        Details->PlayerMFDPreview->SetDesign(Falcon);FlushRenderingCommands();
        TStrongObjectPtr<UTextureRenderTarget2D> Inspection(FWidgetRenderer::CreateTargetFor(FVector2D(1280,720),TF_Bilinear,true));
        Renderer->DrawWidget(Inspection.Get(),Root,FVector2D(1280,720),0);FlushRenderingCommands();TArray<FColor> Image;
        Inspection->GameThread_GetRenderTargetResource()->ReadPixels(Image);
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1280,720,Image,PNG);
        IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Review")),true);
        TestTrue(TEXT("Damage inspection screenshot saved"),FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("Review/MiniHUDDamage.png"))));
    }
    const FVector2D Instrument=SSWMiniHUDLayout::GizmoCenter(FVector2D(1280,720),1);
    const FVector2D Origin=SSWMiniHUDLayout::Origin(FVector2D(1280,720),1);
    TestEqual(TEXT("Directional instrument is centered between its fixed column borders"),Instrument.X-Origin.X,46.0);
    TestTrue(TEXT("Engines sit aft of sensors in the damage schematic"),SSWDamagePanel::DefaultLocation(EShipSystemIcon::Engine).X>SSWDamagePanel::DefaultLocation(EShipSystemIcon::Sensor).X);
    TestTrue(TEXT("Weapons occupy opposite sides of the hull"),SSWDamagePanel::DefaultLocation(EShipSystemIcon::Weapon,0).Y<.5 && SSWDamagePanel::DefaultLocation(EShipSystemIcon::Weapon,1).Y>.5);
    TestEqual(TEXT("Only the damaged engine receives a caution icon"),SSWDamagePanel::DamageCount(Details->DamagePoints),1);
    Details->bShowCaution=true;
    auto CautionFrame=TStrongObjectPtr<UTextureRenderTarget2D>(FWidgetRenderer::CreateTargetFor(FVector2D(1280,720),TF_Bilinear,true));
    Renderer->DrawWidget(CautionFrame.Get(),Root,FVector2D(1280,720),0);FlushRenderingCommands();TArray<FColor> CautionImage;
    CautionFrame->GameThread_GetRenderTargetResource()->ReadPixels(CautionImage);
    TArray64<uint8> CautionPNG;FImageUtils::PNGCompressImageArray(1280,720,CautionImage,CautionPNG);
    TestTrue(TEXT("Caution hull schematic capture saved"),FFileHelper::SaveArrayToFile(CautionPNG,*(FPaths::ProjectSavedDir()/TEXT("Review/CautionSchematic.png"))));
    FPointerEvent Wheel(0,OverModel,OverModel,TSet<FKey>(),FKey(),1,FModifierKeysState());
    TestTrue(TEXT("Wheel over the fixed instrument is consumed"),Details->OnMouseWheel(Geometry,Wheel).IsEventHandled());
    TestFalse(TEXT("Instrument wheel cannot zoom the flight camera"),CameraScrolled);
    const FVector2D Outside(100,400);FPointerEvent OutsideWheel(0,Outside,Outside,TSet<FKey>(),FKey(),1,FModifierKeysState());
    Details->OnMouseWheel(Geometry,OutsideWheel);TestTrue(TEXT("Wheel outside the instrument still reaches the flight camera"),CameraScrolled);

    Details->bHasTarget=true;Details->TargetName=TEXT("NAV 04 - JANEK TRANSFER STATION");Details->TargetClass=TEXT("Waypoint (next)");Details->TargetHull=Details->TargetShield=-1;Details->TargetRange=26.11;Details->TargetSpeed=-1;
    Details->TargetMFDPreview=MakeUnique<FShipMFDPreview>(true);Details->TargetMFDPreview->SetWaypoint();
    auto TargetFrame=TStrongObjectPtr<UTextureRenderTarget2D>(FWidgetRenderer::CreateTargetFor(FVector2D(1280,720),TF_Bilinear,true));
    Renderer->DrawWidget(TargetFrame.Get(),Root,FVector2D(1280,720),0);FlushRenderingCommands();TArray<FColor> TargetImage;
    TargetFrame->GameThread_GetRenderTargetResource()->ReadPixels(TargetImage);TArray64<uint8> TargetPNG;
    FImageUtils::PNGCompressImageArray(1280,720,TargetImage,TargetPNG);
    TestTrue(TEXT("Target title highlight and separate second line captured"),FFileHelper::SaveArrayToFile(TargetPNG,*(FPaths::ProjectSavedDir()/TEXT("Review/TargetMFDHeader.png"))));
    BeginCleanup(Renderer);return true;
}
#endif
