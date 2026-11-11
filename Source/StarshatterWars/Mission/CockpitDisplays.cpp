#include "CockpitDisplays.h"
#include "GearIndicator.h"
#include "GameFramework/PlayerController.h"
#include "FighterHUDDetails.h"
#include "FighterActor.h"
#include "Ship.h"
#include "SimRegion.h"
#include "Sim.h"
#include "SSWRuntimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"

class SCockpitInstrument : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCockpitInstrument) {} SLATE_END_ARGS()
    void Construct(const FArguments&, TWeakPtr<SFighterHUDDetails> InHUD, int32 InPanel, TWeakObjectPtr<AFighterActor> InFighter = {})
    {
        HUD = InHUD;
        Panel = InPanel;
        Fighter = InFighter;
    }
    FVector2D ComputeDesiredSize(float) const override { return Panel==2?FVector2D(1024,512):FVector2D(512,512); }
    int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,int32 L,const FWidgetStyle&,bool) const override
    {
        auto H=HUD.Pin(); if(!H) return L;
        if(Panel==0 && Fighter.IsValid())SSWGearIndicator::Draw(E,G,L+3,FVector2D(426,24),FVector2D(60,55),Fighter->GetRuntimeShip());
        const FLinearColor Blue(.25f,.75f,1.f,1.f);
        auto Text=[&](float X,float Y,const FString& S,FLinearColor C,int32 Size=26){
            FSlateDrawElement::MakeText(E,L+1,G.ToPaintGeometry(FVector2f(1,1),FSlateLayoutTransform(FVector2f(X,Y))),S,FCoreStyle::GetDefaultFontStyle("Bold",Size),ESlateDrawEffect::None,C); };
        auto Line=[&](FVector2D A,FVector2D B,FLinearColor C){ TArray<FVector2D> P{A,B}; FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,2.f); };
        auto Image=[&](FVector2D P,FVector2D D,const FSlateBrush& Brush){FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(FVector2f(D),FSlateLayoutTransform(FVector2f(P))),&Brush);};
        if(Panel==2)
        {
            Text(60,30,FString::Printf(TEXT("SPD %.0f"),H->FlightSpeed),Blue);
            Text(730,30,FString::Printf(TEXT("HDG %03.0f"),H->HeadingDegrees),Blue);
            Line({482,256},{502,256},Blue);Line({522,256},{542,256},Blue);Line({512,226},{512,246},Blue);
            for(int32 Angle=-90;Angle<=90;Angle+=10)
            {
                const double Y=(H->PitchDegrees-Angle)*5; if(FMath::Abs(Y)>130)continue;
                const double C=FMath::Cos(H->BankRadians),S=FMath::Sin(H->BankRadians);
                auto P=[&](double X){return FVector2D(512+C*X-S*Y,256+S*X+C*Y);};
                Line(P(-150),P(-45),Blue);Line(P(45),P(150),Blue);
            }
            if(H->JumpSeconds>0)Text(340,80,FString::Printf(TEXT("QUANTUM JUMP: %d"),FMath::CeilToInt(H->JumpSeconds)),Blue,24);
            else if(H->ThreatLevel==2 && H->bWarningFlash)Text(340,80,TEXT("MISSILE WARNING"),FLinearColor::Red,24);
            else if(H->ThreatLevel==1)Text(350,80,TEXT("LOCK WARNING"),FLinearColor::Yellow,24);
            const bool bTacticalWeapons = H->bShowWeapons && H->FlightHUDMode == SFighterHUDDetails::EFlightHUDMode::Tactical;
            if(H->bShoot && bTacticalWeapons)Text(465,365,TEXT("SHOOT"),Blue,24);
            if(H->bAutoAvailable)Text(465,120,TEXT("AUTO"),Blue,24);
            if(bTacticalWeapons){Text(70,418,H->Primary.ToUpper(),Blue,24);
            Text(660,418,H->Secondary.ToUpper(),Blue,24);}
            if(H->NavReadouts.Num())Text(70,465,H->NavReadouts[0],Blue,22);
            if(H->bGearDown)Text(720,465,TEXT("GEAR DOWN"),Blue,22);
            return L+1;
        }
        if (Panel == 3 || (Panel == 0 && H->Modes[0] == SFighterHUDDetails::EMode::Mission))
        {
            const AFighterActor* Actor = Fighter.Get();
            Sim* Simulation = Sim::GetSim();
            const Ship* Player = Actor && Simulation ? Simulation->GetPlayerShip() : nullptr;
            if (Player && Player != Actor->GetRuntimeShip()) Player = nullptr;
            Text(20, 16, TEXT("MISSION STATUS"), Blue);
            Text(20, 92, H->PlayerName, Blue);
            const auto Rows = H->MissionStatusRows(Player);
            for (int32 Row = 0; Row < Rows.Num(); ++Row)
                Text(20, 160+Row*70, Rows[Row], Blue);
            if (Panel == 0)
            {
                const FString ModeName = H->CameraMode.Get(FString());
                if (!ModeName.IsEmpty()) Text(20, 477, ModeName, Blue, 20);
            }
            return L+1;
        }
        const auto Mode = Panel == 4 ? SFighterHUDDetails::EMode::Ship : H->Modes[Panel];
        if(Mode==SFighterHUDDetails::EMode::Off)return L;
        if(Panel==0 && Mode==SFighterHUDDetails::EMode::Ship)
        {
            Text(20,16,H->PlayerName,Blue);
            int32 Y=65;
            for(const auto& Row:H->StatusRows){Text(20,Y,Row.Label,Blue,24);Text(220,Y,SFighterHUDDetails::FormatStatusReadout(Row.Label,Row.Percent,H->bAugmenterActive),Blue,24);Y+=38;}
            if(H->bShowCaution){
                Text(20,260,TEXT("CAUTION"),Blue,20);
                for(int32 I=0;I<FMath::Min(12,H->DamageRows.Num());++I){const auto& Cell=H->DamageRows[I];if(Cell.Status>=0)Text(20+(I%3)*160,300+(I/3)*28,Cell.Label,SSWDamagePanel::Color(Cell.Status,FPlatformTime::Seconds(),Blue),16);}
            }
            else if(H->PlayerMFDPreview && H->PlayerMFDPreview->IsReady())Image({120,270},{270,135},H->PlayerMFDPreview->GetBrush());
            Text(20,440,H->HomeCarrierReadout,Blue,20);
            Text(20,477,H->PlayerClass,Blue,20);
            const FString ModeName = H->CameraMode.Get(FString());
            if (!ModeName.IsEmpty()) Text(190,477,ModeName,Blue,20);
        }
        else if(Mode==SFighterHUDDetails::EMode::Ship)
        {
            Text(20,16,H->bHasTarget ? H->TargetName : TEXT("NO TARGET"), H->bHasTarget ? H->TargetDisplayColor : Blue, 24);
            if(H->bHasTarget){
                if(H->TargetMFDPreview && H->TargetMFDPreview->IsReady())Image({30,100},{450,225},H->TargetMFDPreview->GetBrush());
                const FString Status = H->bTargetTracking ? TEXT("   TRACKING") :
                    H->bTargetLocked ? TEXT("   LOCKED") : TEXT("");
                Text(20,350,H->TargetClass + Status,H->TargetDisplayColor,22);
                Text(20,400,TEXT("HULL ")+SFighterHUDDetails::FormatStatusReadout(TEXT("HULL"),H->TargetHull),Blue,24);
                Text(20,450,FString::Printf(TEXT("RANGE %.1f KM"),H->TargetRange),Blue,24);
            }
        }
        else
        {
            Text(20,16,Mode==SFighterHUDDetails::EMode::FOV?TEXT("SNSR FOV"):Mode==SFighterHUDDetails::EMode::HSD?TEXT("SNSR HSD"):TEXT("SNSR 3D"),Blue);
            for(int32 Ring=1;Ring<=3;++Ring){FVector2D Last(256+60*Ring,260);for(int32 I=1;I<=64;++I){double A=2*PI*I/64;FVector2D P(256+FMath::Cos(A)*60*Ring,260+FMath::Sin(A)*60*Ring);Line(Last,P,Blue*.5f);Last=P;}}
            Line({76,260},{436,260},Blue);Line({256,80},{256,440},Blue);
            for(const auto& B:H->Contacts){
                FVector2D O;
                if(Mode==SFighterHUDDetails::EMode::FOV)O=FVector2D(B.Az/(PI/2),-B.El/(PI/2))*175;
                else {double R=FMath::Clamp(B.Range/FMath::Max(1.,H->SensorRange),0.,1.)*175;O=FVector2D(FMath::Sin(B.Az),-FMath::Cos(B.Az))*R;if(Mode==SFighterHUDDetails::EMode::ThreeD)O.Y=O.Y*.55-FMath::Sin(B.El)*R*.45;}
                if(O.Size()>175)O=O.GetSafeNormal()*175;
                FVector2D P=FVector2D(256,260)+O;Line(P-FVector2D(4,0),P+FVector2D(4,0),B.Color);Line(P-FVector2D(0,4),P+FVector2D(0,4),B.Color);
            }
            Text(20,470,FString::Printf(TEXT("%.0f KM   %d TRACKS"),H->SensorRange/1000,H->Contacts.Num()),Blue,22);
        }
        return Panel==0?L+4:L+1;
    }
private:
    TWeakPtr<SFighterHUDDetails> HUD;
    TWeakObjectPtr<AFighterActor> Fighter;
    int32 Panel=0;
};

namespace
{
struct FCockpitPanelSurface
{
    FVector TopLeft;
    FVector TopRight;
    FVector BottomLeft;
};

// Authored HUD and MFD faces converted from OBJ coordinates to the imported FBX basis.
TConstArrayView<FCockpitPanelSurface> CockpitPanelLayout(FName MeshName)
{
    static const FCockpitPanelSurface FalconPit[] = {
        {{71.102417,182.349457,-40.402714}, {47.681965,190.185822,-40.402714}, {70.079567,179.292480,-64.888107}},
        {{-7.01,193.36,-33.73}, {-35.66,193.36,-33.73}, {-7.01,189.86,-62.17}},
        {{48.085361,183.565155,24.140862}, {-48.085361,183.565155,24.140862}, {48.085361,183.565155,-24.397217}},
        {{35.68,193.36,-33.73}, {7.03,193.36,-33.73}, {35.68,189.86,-62.17}},
        {{-47.681965,190.185822,-40.402714}, {-71.102417,182.349457,-40.402714}, {-46.659107,187.128845,-64.888107}},
    };
    if (MeshName == TEXT("FalconPit")) return FalconPit;
    static const FCockpitPanelSurface EaglePit[] = {
        {{61.28292, 190.59924, -35.39997}, {30.64771, 194.63245, -35.39997}, {60.76295, 186.64972, -65.65838}},
        {{21.82903, 187.85002, -82.27844}, {-21.67839, 187.85002, -82.27844}, {21.82903, 179.75490, -102.31459}},
        {{45.08002, 183.56516, 22.62400}, {-45.08002, 183.56516, 22.62401}, {45.08002, 183.56516, -22.88045}},
        {{9.03302, 195.67630, -29.79222}, {-8.87209, 195.67630, -29.79222}, {9.03302, 193.33922, -47.54412}},
        {{-30.39818, 194.60727, -35.59299}, {-61.03339, 190.57407, -35.59299}, {-29.87822, 190.65775, -65.85140}},
    };
    static const FCockpitPanelSurface StormhawkPit[] = {
        {{80.67818, 190.56984, -27.62829}, {46.14000, 195.07799, -28.22180}, {80.67818, 186.07870, -61.74181}},
        {{30.62526, 196.64900, -36.43555}, {-30.18109, 196.64900, -36.43555}, {30.62526, 192.85352, -65.26510}},
        {{48.08536, 183.56516, 24.14082}, {-48.08536, 183.56516, 24.14082}, {48.08536, 183.56516, -24.39726}},
        {{66.02338, 186.10338, -79.00187}, {44.06894, 187.44617, -79.00187}, {65.50951, 177.70174, -99.32305}},
        {{-46.13939, 195.07799, -28.22180}, {-80.67758, 190.56984, -27.62829}, {-46.13939, 190.58685, -62.33533}},
    };
    static const FCockpitPanelSurface ThunderboltPit[] = {
        {{61.48940, 188.05246, -49.83309}, {41.34616, 188.05246, -49.83309}, {61.48940, 185.42325, -69.80397}},
        {{16.65184, 194.80428, -34.40786}, {-17.33989, 194.80428, -34.40786}, {16.65184, 190.36746, -68.10864}},
        {{45.08002, 183.56516, 22.62400}, {-45.08002, 183.56516, 22.62401}, {45.08002, 183.56516, -22.88045}},
        {{21.82903, 185.95863, -82.96233}, {-21.67839, 185.95863, -82.96233}, {21.82903, 177.86351, -102.99847}},
        {{-40.63260, 188.05246, -49.83309}, {-60.77584, 188.05246, -49.83309}, {-40.63260, 185.42325, -69.80397}},
    };
    if (MeshName == TEXT("EaglePit")) return EaglePit;
    if (MeshName == TEXT("StormhawkPit")) return StormhawkPit;
    if (MeshName == TEXT("ThunderboltPit")) return ThunderboltPit;
    return {};
 }
}

FCockpitDisplays::~FCockpitDisplays(){Clear();}
bool FCockpitDisplays::IsActive() const
{
    if(!Mesh.IsValid() || Panels.Num()!=5)return false;
    for(const auto& Panel:Panels)if(!Panel.IsValid() || !Panel->GetRenderTarget())return false;
    return true;
}
bool FCockpitDisplays::HandleGearClick()
{
    if(!IsActive() || Panels.IsEmpty() || !Panels[0].IsValid())return false;
    auto* Panel=Panels[0].Get();auto* Fighter=Cast<AFighterActor>(Panel->GetOwner());
    auto* PC=Fighter && Fighter->GetWorld()?Fighter->GetWorld()->GetFirstPlayerController():nullptr;
    FVector Origin,Direction;if(!PC || !PC->DeprojectMousePositionToWorld(Origin,Direction))return false;
    const FTransform T=Panel->GetComponentTransform();const FVector O=T.InverseTransformPosition(Origin),D=T.InverseTransformVector(Direction);
    if(FMath::Abs(D.X)<SMALL_NUMBER)return false;
    const double Distance=-O.X/D.X;if(Distance<=0)return false;
    const FVector Hit=O+D*Distance;
    if(!SSWGearIndicator::Contains(FVector2D(-Hit.Y,-Hit.Z),FVector2D(426,24),FVector2D(60,55)))return false;
    if(Ship* P=Fighter->GetRuntimeShip())P->ToggleGear();return true;
}
void FCockpitDisplays::Clear()
{
    if(Mesh.IsValid() && OriginalHUD.IsValid() && OriginalHUDSlot != INDEX_NONE)
        Mesh->SetMaterial(OriginalHUDSlot, OriginalHUD.Get());
    OriginalHUDSlot = INDEX_NONE;
    for(auto& Panel:Panels)if(Panel.IsValid())Panel->DestroyComponent();
    Panels.Reset();Mesh.Reset();OriginalHUD.Reset();Blank.Reset();
}
void FCockpitDisplays::Update(AFighterActor* Fighter,TSharedRef<SFighterHUDDetails> HUD,bool Visible)
{
    AActor* Child=Fighter && Fighter->CockpitComponent?Fighter->CockpitComponent->GetChildActor():nullptr;
    UStaticMeshComponent* Screen=Child?Child->FindComponentByClass<UStaticMeshComponent>():nullptr;
    const auto Layout = Screen && Screen->GetStaticMesh()
        ? CockpitPanelLayout(Screen->GetStaticMesh()->GetFName()) : TConstArrayView<FCockpitPanelSurface>{};
    if (!Visible || !Fighter || !Fighter->IsCockpitViewActive() || Layout.IsEmpty()) { Clear(); return; }
    if (Mesh.Get() != Screen)
    {
        Clear();
        Mesh = Screen;
        OriginalHUDSlot = Screen->GetMaterialIndex(TEXT("HUD"));
        if (OriginalHUDSlot != INDEX_NONE) OriginalHUD.Reset(Screen->GetMaterial(OriginalHUDSlot));
        for (int32 I = 0; I < Layout.Num(); ++I)
        {
            const auto& Surface = Layout[I];
            auto* W=NewObject<UWidgetComponent>(Child,NAME_None,RF_Transient);
            W->SetupAttachment(Screen);W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(I==2?FVector2D(1024,512):FVector2D(512,512));
            W->SetPivot(FVector2D::ZeroVector);W->SetTwoSided(true);W->SetBlendMode(I==2?EWidgetBlendMode::Transparent:EWidgetBlendMode::Opaque);
            W->SetBackgroundColor(I==2?FLinearColor::Transparent:FLinearColor::Black);
            W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->SetTickWhenOffscreen(true);W->SetRedrawTime(.1f);
            const FVector Right=Surface.TopRight-Surface.TopLeft,Down=Surface.BottomLeft-Surface.TopLeft;const FVector Normal=FVector::CrossProduct(Right,Down).GetSafeNormal();
            // Keep the panel in front of the surface after fighter cockpit scaling.
            const double SurfaceClearance = I == 2 ? 0.03 : 1.0;
            W->SetRelativeLocation(Surface.TopLeft+Normal*SurfaceClearance);W->SetRelativeRotation(FRotationMatrix::MakeFromYZ(-Right,-Down).Rotator());
            W->SetRelativeScale3D(FVector(1,Right.Size()/(I==2?1024:512),Down.Size()/512));
            W->SetSlateWidget(SNew(SCockpitInstrument,HUD,I,TWeakObjectPtr<AFighterActor>(Fighter)));W->RegisterComponent();Panels.Add(W);
        }
    }
    // Blank the imported HUD artwork only once the replacement is rendered.
    if(IsActive() && OriginalHUDSlot != INDEX_NONE && !Blank.IsValid())
    {
        Blank.Reset(UTexture2D::CreateTransient(1,1,PF_B8G8R8A8));
        if(Blank.IsValid()){
            auto& Data=Blank->GetPlatformData()->Mips[0].BulkData;void* Pixels=Data.Lock(LOCK_READ_WRITE);FMemory::Memzero(Pixels,4);Data.Unlock();Blank->UpdateResource();
            // Use a base material. A widget's dynamic MID is not a valid parent.
            auto* Material = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
            if (Material)
            {
                auto* Empty = UMaterialInstanceDynamic::Create(Material, Screen);
                Empty->SetTextureParameterValue(TEXT("SlateUI"), Blank.Get());
                Empty->SetVectorParameterValue(TEXT("TintColorAndOpacity"), FLinearColor::Transparent);
                Screen->SetMaterial(OriginalHUDSlot, Empty);
            }
        }
    }
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"
#include "MissionCameraRig.h"
#include "Components/SceneCaptureComponent2D.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ShowFlags.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCockpitDisplaysTest,"Starshatter.HUD.CockpitDisplays",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCockpitDisplaysTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Preview world"),World))return false;
    UClass* Class=LoadClass<AFighterActor>(nullptr,TEXT("/Game/Models/Falcon/BP_Falcon.BP_Falcon_C"));
    auto* Fighter=Class?World->SpawnActor<AFighterActor>(Class):nullptr;
    if(!TestNotNull(TEXT("Actual Falcon"),Fighter)){World->DestroyWorld(false);return false;}
    auto HUD=SNew(SFighterHUDDetails).ShowHUD(true);
    HUD->bHasPlayer=true;HUD->PlayerName=TEXT("Delta 1");HUD->PlayerClass=TEXT("Falcon");HUD->HomeCarrierReadout=TEXT("HOME Archon");
    HUD->StatusRows={{TEXT("THRUST"),50},{TEXT("FUEL"),100},{TEXT("HULL"),100},{TEXT("GUNS"),99}};
    HUD->Primary=TEXT("GUNS: DELTA");HUD->Secondary=TEXT("SRM 4");HUD->FlightSpeed=500;HUD->HeadingDegrees=90;HUD->NavReadouts.Add(TEXT("AUTO NAV 1"));
    HUD->Contacts.Add({.1,.1,3000,FLinearColor(.25,.65,1)});HUD->SensorRange=75000;
    FCockpitDisplays Displays;Fighter->SetCockpitViewActive(true);Displays.Update(Fighter,HUD,true);
    AActor* Child=Fighter->CockpitComponent->GetChildActor();
    TArray<UWidgetComponent*> Panels;Child->GetComponents(Panels);
    TestEqual(TEXT("Four physical MFDs and a combiner"),Panels.Num(),5);
    for(int32 I=0;I<Panels.Num();++I){
        TestEqual(TEXT("Attached to actual cockpit mesh"),Panels[I]->GetAttachParent(),static_cast<USceneComponent*>(Child->FindComponentByClass<UStaticMeshComponent>()));
        TestEqual(TEXT("Display collision disabled"),Panels[I]->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
        const auto T=Panels[I]->GetRelativeTransform();const auto D=Panels[I]->GetDrawSize();
        const FVector TL=T.TransformPosition(FVector::ZeroVector),TR=T.TransformPosition(FVector(0,-D.X,0)),BL=T.TransformPosition(FVector(0,0,-D.Y));
        TestTrue(TEXT("Screens face pilot behind panel"),FVector::CrossProduct(TR-TL,BL-TL).GetSafeNormal().Y<-.9);
        TestTrue(TEXT("Screen right maps to decreasing mesh X; no mirrored text"),TR.X<TL.X);
    }
    auto* Mesh=Child->FindComponentByClass<UStaticMeshComponent>();UMaterialInterface* Original=Mesh->GetMaterial(2);
    FString Capture;
    if(!GUsingNullRHI && FParse::Value(FCommandLine::Get(),TEXT("CockpitCapture="),Capture)){
        FWidgetRenderer* Renderer=new FWidgetRenderer(true);
        for(int32 I=0;I<Panels.Num();++I){
            const FVector2D Size=I==2?FVector2D(1024,512):FVector2D(512,512);
            TStrongObjectPtr<UTextureRenderTarget2D> RT(FWidgetRenderer::CreateTargetFor(Size,TF_Bilinear,true));
            RT->ClearColor=I==2?FLinearColor::Transparent:FLinearColor::Black;
            Renderer->DrawWidget(RT.Get(),SNew(SCockpitInstrument,HUD,I),Size,0);
            FlushRenderingCommands();TArray<FColor> Pixels;FReadSurfaceDataFlags Flags;Flags.SetLinearToGamma(false);
            RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags);
            int32 Lit=0;for(const auto& P:Pixels)if(P.B>50 && P.G>50)++Lit;
            TestTrue(TEXT("Live readouts render nonempty"),Lit>100);
            TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
            TestTrue(TEXT("Panel preview written"),FFileHelper::SaveArrayToFile(PNG,*(Capture+FString::FromInt(I)+TEXT(".png"))));
        }
        BeginCleanup(Renderer);
        Fighter->CockpitComponent->SetVisibility(true,true);Child->SetActorHiddenInGame(false);
        for(auto* Panel:Panels){Panel->SetRedrawTime(0);Panel->TickComponent(.2f,LEVELTICK_All,nullptr);}
        FlushRenderingCommands();
        Displays.Update(Fighter,HUD,true);
        TestTrue(TEXT("Registered physical displays become ready"),Displays.IsActive());
        for(auto* Panel:Panels){TArray<FColor> P;Panel->GetRenderTarget()->GameThread_GetRenderTargetResource()->ReadPixels(P);int32 Lit=0;for(const auto& C:P)if(C.B>50 && C.G>50)++Lit;TestTrue(TEXT("Mounted widget render target contains live readouts"),Lit>100);}

        auto* Camera=World->SpawnActor<AMissionViewCamera>();
        auto* View=Cast<UMissionViewCameraComponent>(Camera->GetCameraComponent());
        TestNotNull(TEXT("Mission camera owns near-plane adapter"),View);
        if(View){FMinimalViewInfo POV;View->bCockpitView=true;View->GetCameraView(0,POV);TestEqual(TEXT("Cockpit panels survive near clipping"),POV.PerspectiveNearClipPlane,.05f);View->bCockpitView=false;View->GetCameraView(0,POV);TestTrue(TEXT("External near plane remains default"),POV.PerspectiveNearClipPlane<0);}
        auto* Owner=World->SpawnActor<AActor>();auto* CaptureComponent=NewObject<USceneCaptureComponent2D>(Owner);
        CaptureComponent->RegisterComponent();CaptureComponent->SetWorldLocation(Fighter->CockpitComponent->GetComponentLocation());CaptureComponent->SetWorldRotation(Fighter->GetActorRotation());
        CaptureComponent->FOVAngle=70;CaptureComponent->bOverride_CustomNearClippingPlane=true;CaptureComponent->CustomNearClippingPlane=.05f;
        CaptureComponent->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;CaptureComponent->ShowOnlyActors.Add(Child);for(auto* Panel:Panels)CaptureComponent->ShowOnlyComponent(Panel);ApplyViewMode(VMI_Unlit,true,CaptureComponent->ShowFlags);CaptureComponent->CaptureSource=SCS_SceneColorHDR;CaptureComponent->ShowFlags.SetPostProcessing(false);
        TStrongObjectPtr<UTextureRenderTarget2D> CockpitRT(NewObject<UTextureRenderTarget2D>());CockpitRT->InitAutoFormat(1280,720);CaptureComponent->TextureTarget=CockpitRT.Get();
        FAssetCompilingManager::Get().FinishAllCompilation();
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        World->UpdateWorldComponents(false,false);World->SendAllEndOfFrameUpdates();CaptureComponent->CaptureScene();FlushRenderingCommands();
        TArray<FColor> CockpitPixels;CockpitRT->GameThread_GetRenderTargetResource()->ReadPixels(CockpitPixels);
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1280,720,CockpitPixels,PNG);
        TestTrue(TEXT("Mounted cockpit preview written"),FFileHelper::SaveArrayToFile(PNG,*(Capture+TEXT("mounted.png"))));
    }
    Fighter->SetCockpitViewActive(false);Displays.Update(Fighter,HUD,true);Panels.Reset();Child->GetComponents(Panels);
    TestEqual(TEXT("Leaving cockpit destroys transient displays"),Panels.Num(),0);TestEqual(TEXT("Imported HUD material restored"),Mesh->GetMaterial(2),Original);
    Displays.Update(Fighter,HUD,true);TestFalse(TEXT("External mode retains floating HUD fallback"),Displays.IsActive());
    Fighter->SetCockpitViewActive(true);FMissionCameraRig Rig;Rig.PlayerActor=Fighter;Rig.Reset(nullptr);TestFalse(TEXT("Camera reset clears cockpit mode"),Fighter->IsCockpitViewActive());
    for (const TCHAR* Name : {TEXT("Eagle"), TEXT("Stormhawk"), TEXT("Thunderbolt"), TEXT("Zodiac"), TEXT("Jackal")})
    {
        const FString Path = FString::Printf(TEXT("/Game/Models/%s/BP_%s.BP_%s_C"), Name, Name, Name);
        UClass* FighterClass = LoadClass<AFighterActor>(nullptr, *Path);
        AFighterActor* Other = FighterClass ? World->SpawnActor<AFighterActor>(FighterClass) : nullptr;
        if (!TestNotNull(FString(Name) + TEXT(" fighter Blueprint"), Other)) continue;
        Other->SetCockpitViewActive(true);
        Displays.Update(Other, HUD, true);
        AActor* Cockpit = Other->CockpitComponent->GetChildActor();
        TArray<UWidgetComponent*> Instruments;
        if (Cockpit) Cockpit->GetComponents(Instruments);
        const bool bHasSuppliedCockpit = FCString::Stricmp(Name, TEXT("Zodiac")) != 0 && FCString::Stricmp(Name, TEXT("Jackal")) != 0;
        TestEqual(FString(Name) + TEXT(" physical instruments"), Instruments.Num(), bHasSuppliedCockpit ? 5 : 0);
        for (auto* Instrument : Instruments)
        {
            TestEqual(TEXT("Attached to the cockpit surface"), Instrument->GetAttachParent(),
                static_cast<USceneComponent*>(Cockpit->FindComponentByClass<UStaticMeshComponent>()));
            TestEqual(TEXT("Instrument has no gameplay collision"), Instrument->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            const FTransform Transform = Instrument->GetRelativeTransform();
            TestTrue(TEXT("Instrument faces aft toward pilot"), Transform.GetRotation().GetForwardVector().Y < -0.8);
        }
        if (bHasSuppliedCockpit && !GUsingNullRHI)
        {
            Other->CockpitComponent->SetVisibility(true, true);
            Cockpit->SetActorHiddenInGame(false);
            for (auto* Instrument : Instruments)
            {
                Instrument->SetRedrawTime(0);
                Instrument->TickComponent(0.2f, LEVELTICK_All, nullptr);
            }
            FlushRenderingCommands();
            Displays.Update(Other, HUD, true);
            TestTrue(FString(Name) + TEXT(" instrument render targets are ready"), Displays.IsActive());
            for (auto* Instrument : Instruments)
            {
                auto* Target = Instrument->GetRenderTarget();
                if (!TestNotNull(FString(Name) + TEXT(" panel render target"), Target)) continue;
                TArray<FColor> Pixels;
                Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
                int32 Lit = 0;
                for (const FColor& Pixel : Pixels) if (Pixel.B > 50 && Pixel.G > 50) ++Lit;
                TestTrue(FString(Name) + TEXT(" panel renders live information"), Lit > 100);
            }
        }
        Displays.Clear();
        Other->Destroy();
    }
    World->DestroyWorld(false);return true;
}
#endif

