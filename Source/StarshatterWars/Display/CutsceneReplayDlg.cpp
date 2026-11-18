#include "Galaxy.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "PlanetActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "CutsceneReplayDlg.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Viewport.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/AudioComponent.h"
#include "Components/Image.h"
#include "ShipDesignRegistry.h"
#include "MissionUIStyle.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

namespace
{
    // Keep close-up cameras beyond UE's near clip plane; preserve authored relative distances.
    constexpr double ReplayUnitsPerMetre = 1.0;
}

TSharedRef<SWidget> UCutsceneReplayDlg::RebuildWidget()
{
    if(!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this);
    if(!RuntimeHost)
    {
        auto* Layout=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Layout;
        RuntimeHost=WidgetTree->ConstructWidget<UBorder>(); RuntimeHost->SetPadding(FMargin(0));
        Layout->AddChildToVerticalBox(RuntimeHost)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Controls=WidgetTree->ConstructWidget<UHorizontalBox>(); Layout->AddChildToVerticalBox(Controls);
        auto Add=[&](const TCHAR* Icon,const TCHAR* Tip,const TCHAR* Color)
        {
            auto* Button=WidgetTree->ConstructWidget<UButton>();
            auto* Image=WidgetTree->ConstructWidget<UImage>();
            const FString Path=FString::Printf(TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/%s.%s"),Icon,Icon);
            MissionUIStyle::SetActionIcon(Image,*Path,FLinearColor::White);
            Image->SetDesiredSizeOverride(FVector2D(24));
            Image->SetVisibility(ESlateVisibility::HitTestInvisible);
            Button->SetContent(Image);
            if(auto* Slot=Cast<UButtonSlot>(Image->Slot))
            { Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(6)); }
            Button->SetToolTipText(FText::FromString(Tip));MissionUIStyle::ApplyActionButtonStyle(Button,Color);
            auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(48);Size->SetHeightOverride(38);Size->SetContent(Button);
            Controls->AddChildToHorizontalBox(Size)->SetPadding(FMargin(4,4));return Button;
        };
        Add(TEXT("T_Play_Rev_256x256"),TEXT("Restart replay"),TEXT("Cyan"))->OnClicked.AddDynamic(this,&UCutsceneReplayDlg::ReplayRewind);
        Add(TEXT("T_Play_256x256"),TEXT("Play replay"),TEXT("Green"))->OnClicked.AddDynamic(this,&UCutsceneReplayDlg::ReplayPlay);
        Add(TEXT("T_Pause_256x256"),TEXT("Pause replay"),TEXT("Blue"))->OnClicked.AddDynamic(this,&UCutsceneReplayDlg::ReplayPause);
        Add(TEXT("T_Stop_256x256"),TEXT("Stop replay"),TEXT("Red"))->OnClicked.AddDynamic(this,&UCutsceneReplayDlg::ReplayStop);
    }
    return Super::RebuildWidget();
}
void UCutsceneReplayDlg::NativeConstruct()
{
    Super::NativeConstruct();
    if(RuntimeOverlay)
    {
        ReplayViewport=WidgetTree->ConstructWidget<UViewport>();
        auto Children=RuntimeOverlay->GetAllChildren();
        struct FOverlayLayout { EHorizontalAlignment H; EVerticalAlignment V; FMargin Padding; };
        TArray<FOverlayLayout> Layouts;
        for(auto* Child:Children)
        {
            auto* Existing=Cast<UOverlaySlot>(Child->Slot);
            Layouts.Add({Existing?Existing->GetHorizontalAlignment():HAlign_Fill,Existing?Existing->GetVerticalAlignment():VAlign_Fill,Existing?Existing->GetPadding():FMargin(0)});
            RuntimeOverlay->RemoveChild(Child);
        }
        auto* ViewSlot=RuntimeOverlay->AddChildToOverlay(ReplayViewport);
        ViewSlot->SetHorizontalAlignment(HAlign_Fill);
        ViewSlot->SetVerticalAlignment(VAlign_Fill);
        RuntimeOverlay->SetClipping(EWidgetClipping::ClipToBounds);
        for(int32 Index=0;Index<Children.Num();++Index)
        {
            auto* Added=RuntimeOverlay->AddChildToOverlay(Children[Index]);
            Added->SetHorizontalAlignment(Layouts[Index].H);Added->SetVerticalAlignment(Layouts[Index].V);Added->SetPadding(Layouts[Index].Padding);
        }
        ReplayViewport->SetEnableAdvancedFeatures(false); ReplayViewport->SetLightIntensity(3);
        ReplayViewport->SetBackgroundColor(FLinearColor(.002,.004,.009,1));
        ReplayViewport->SetShowFlag(TEXT("EyeAdaptation"),false);
    }
    for(auto* Text:{HeaderText,MessageTitleText,MessageSubtitleText,CaptionTextBottom})
        if(Text){Text->SetFont(MissionUIStyle::GetRowFont(14));Text->SetMinDesiredWidth(0);Text->SetWrapTextAt(0);Text->SetAutoWrapText(true);}
    if(HeaderText)if(auto* Layout=Cast<UOverlaySlot>(HeaderText->Slot))Layout->SetPadding(FMargin(16,12,16,0));
    if(MessageTitleText)if(auto* Layout=Cast<UOverlaySlot>(MessageTitleText->Slot))Layout->SetPadding(FMargin(16,42,16,0));
    if(MessageSubtitleText)if(auto* Layout=Cast<UOverlaySlot>(MessageSubtitleText->Slot))Layout->SetPadding(FMargin(16,66,16,0));
    if(CaptionTextBottom)if(auto* Layout=Cast<UOverlaySlot>(CaptionTextBottom->Slot))Layout->SetPadding(FMargin(24,0,24,18));
    SetVisibility(ESlateVisibility::Visible); SetIsEnabled(true);
}
void UCutsceneReplayDlg::LoadReplay(const FS_CampaignMission& Scene,int32 CampaignNumber)
{
    ReplayStop();
    for(auto& Pair:ReplayActors)if(Pair.Value.IsValid())Pair.Value->Destroy();
    ReplayActors.Empty();InitialPositions.Empty();RetainedParts.Empty();bPreviewBuilt=false;
    ActiveMissionData=Scene;CurrentCampaignNumber=CampaignNumber;BuildSortedEventQueue();PreloadSceneAssets();
    SceneDurationSeconds=ResolveSceneDurationSeconds();
    if(HeaderText)HeaderText->SetText(FText::FromString(Scene.Objective));
    ReplayRewind();ReplayPlay();
}
void UCutsceneReplayDlg::BuildPreview()
{
    UWorld* World=ReplayViewport?ReplayViewport->GetViewportWorld():nullptr;
    if(!World)return;
    bPreviewBuilt=true;
    bool bLight=true;
    for(const auto& Element:ActiveMissionData.Element)
    {
        const auto* Design=ShipDesignRegistry::Find(Element.Design);
        if(!Design || Element.Design.Equals(TEXT("CameraPod"),ESearchCase::IgnoreCase))continue;
        TArray<FBriefingShipPreviewPart> Parts;
        if(!BriefingShipPreview::ResolveParts(*Design,Parts))continue;
        const FBox ModelBounds=BriefingShipPreview::CalculateBounds(Parts);
        if(!ModelBounds.IsValid)continue;
        for(auto& Part:Parts)Part.Transform.AddToTranslation(-ModelBounds.GetCenter());
        RetainedParts.Append(Parts);
        AActor* Visual=BriefingShipPreview::CreateVisual(World,Parts,bLight);
        if(!Visual)continue;
        bLight=false;
        Visual->SetActorLocation(Element.Location*ReplayUnitsPerMetre);
        Visual->SetActorRotation(FRotator(0,Element.Heading,0));
        // Normalize imported geometry to the simulation radius instead of assuming mesh units.
        const ShipDesign* Legacy=ShipDesignRegistry::FindLegacy(Element.Design);
        const double Radius=Legacy?FMath::Max(1.f,Legacy->radius):100.;
        Visual->SetActorScale3D(FVector(Radius*ReplayUnitsPerMetre/FMath::Max(ModelBounds.GetExtent().Size(),1.)));
        ReplayActors.Add(Element.Name,Visual);InitialPositions.Add(Element.Name,Element.Location*ReplayUnitsPerMetre);
        if(FocusName.IsEmpty())FocusName=Element.Name;
    }
    UE_LOG(LogTemp,Display,TEXT("[IntelReplay] Built %d passive ships from %d scene elements"),ReplayActors.Num(),ActiveMissionData.Element.Num());
}
void UCutsceneReplayDlg::ReplayCamera(const FS_MissionEvent& Event)
{
    const int32 Command=Event.EventParam.IsEmpty()?0:Event.EventParam[0];
    if(Command==3)
    {
        if(!Event.EventTarget.IsEmpty())FocusName=Event.EventTarget;
        FocusName.RemoveFromStart(TEXT("body:"));
        if(!ReplayActors.Contains(FocusName) && ReplayViewport && ReplayViewport->GetViewportWorld())
        {
            auto* Universe=Galaxy::GetInstance();
            auto* System=Universe?Universe->GetSystem(TCHAR_TO_UTF8(*ActiveMissionData.MissionSystem)):nullptr;
            Orbital* Found=System?System->FindOrbital(TCHAR_TO_UTF8(*FocusName)):nullptr;
            auto* Body=Found && (Found->GetType()==Orbital::STAR || Found->GetType()==Orbital::PLANET || Found->GetType()==Orbital::MOON)?static_cast<OrbitalBody*>(Found):nullptr;
            if(Body)
            {
                // Read orbital metadata only; build a passive miniature in the preview world.
                auto* Planet=ReplayViewport->GetViewportWorld()->SpawnActor<APlanetActor>();
                if(Planet)
                {
                    Planet->SetPlanetRadius(Body->Radius()*ReplayUnitsPerMetre);
                    auto LoadSurface=[](const char* Token)->UTexture2D*
                    {
                        if(!Token || !*Token)return nullptr;
                        FString Name=FPaths::GetBaseFilename(UTF8_TO_TCHAR(Token));
                        if(!Name.RemoveFromEnd(TEXT("_hi")))Name.RemoveFromEnd(TEXT("_lo"));
                        for(const TCHAR* Folder:{TEXT("/Game/UI/MissionNav/Surfaces"),TEXT("/Game/GameData/Galaxy/PlanetMaterials"),TEXT("/Game/GameData/Galaxy/MoonMaterials")})
                        {
                            const FString Path=FString(Folder)/Name+TEXT(".")+Name;
                            if(auto* Texture=LoadObject<UTexture2D>(nullptr,*Path))return Texture;
                        }
                        return nullptr;
                    };
                    UTexture2D* Surface=FocusName.Equals(TEXT("Janek"),ESearchCase::IgnoreCase)
                        ?LoadObject<UTexture2D>(nullptr,TEXT("/Game/Models/Universe/Planets/Janek/T_JanekWorldScapeOrbit.T_JanekWorldScapeOrbit")):nullptr;
                    if(!Surface)Surface=LoadSurface(Body->GetHighResTexture());
                    if(!Surface)Surface=LoadSurface(Body->GetTexture());
                    Planet->SetPlanetTextures(Surface,LoadSurface(Body->GetGlossTexture()),LoadSurface(Body->GetGlowTexture()));
                    Planet->SetLightDirection(FVector(-1,-.45,.45).GetSafeNormal());
                    Planet->SetActorEnableCollision(false);Planet->SetActorTickEnabled(false);
                    ReplayActors.Add(FocusName,Planet);
                }
            }
        }
        Orbit=Event.EventPoint;OrbitRate=FVector::ZeroVector;
    }
    else if(Command==5)OrbitRate=Event.EventPoint;
    else if(Command==1)Orbit=Event.EventPoint;
}
void UCutsceneReplayDlg::UpdatePreview(float DeltaTime)
{
    if(!ReplayViewport)return;
    // Display-only actors follow the recorded route; no navigation, damage or rewards execute.
    for(const auto& Element:ActiveMissionData.Element)
    {
        auto* Entry=ReplayActors.Find(Element.Name); if(!Entry || !Entry->IsValid())continue;
        FVector Position=Element.Location; double Remaining=ReplaySeconds;
        for(const auto& Leg:Element.Navpoint)
        {
            const FVector Delta=Leg.ObjectiveLocation-Position;const double Travel=Delta.Size()/FMath::Max(1,Leg.Speed);
            if(Remaining<=Travel){Position+=Delta*FMath::Clamp(Remaining/FMath::Max(.001,Travel),0.,1.);break;}
            Position=Leg.ObjectiveLocation;Remaining-=Travel;
        }
        Entry->Get()->SetActorLocation(Position*ReplayUnitsPerMetre);
    }
    Orbit+=OrbitRate*DeltaTime;
    FVector Focus=FVector::ZeroVector;
    if(auto* Entry=ReplayActors.Find(FocusName);Entry && Entry->IsValid())Focus=Entry->Get()->GetActorLocation();
    const FVector Direction=FRotator(FMath::RadiansToDegrees(Orbit.Y),FMath::RadiansToDegrees(Orbit.X),0).Vector();
    ReplayViewport->SetViewLocation(Focus+Direction*FMath::Max(2.,Orbit.Z*ReplayUnitsPerMetre));
    ReplayViewport->SetViewRotation((-Direction).Rotation());
}
void UCutsceneReplayDlg::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
    UUserWidget::NativeTick(Geometry,DeltaTime); // Never tick the live campaign scene/camera driver.
    if(!bPreviewBuilt)BuildPreview();
    if(!bPreviewBuilt || !bReplayPlaying)return;
    ReplaySeconds+=DeltaTime;
    while(SortedEvents.IsValidIndex(NextReplayEvent) && SortedEvents[NextReplayEvent].EventTime<=ReplaySeconds)
    {
        const auto& Event=SortedEvents[NextReplayEvent++];
        if(Event.EventType==MISSIONEVENT_TYPE::CAMERA)ReplayCamera(Event);
        else if(Event.EventType==MISSIONEVENT_TYPE::MESSAGE)ExecuteMessageEvent(Event);
        else if(Event.EventType==MISSIONEVENT_TYPE::DISPLAY)ExecuteDisplayEvent(Event);
        else if(Event.EventType==MISSIONEVENT_TYPE::JUMP)
        {if(auto* Entry=ReplayActors.Find(Event.EventShip);Entry && Entry->IsValid())Entry->Get()->SetActorHiddenInGame(true);}
        else if(Event.EventType==MISSIONEVENT_TYPE::END_SCENE){ReplayPause();StopCutsceneAudio();break;}
    }
    UpdatePreview(DeltaTime);UpdatePanelFade(UGameplayStatics::GetRealTimeSeconds(GetWorld()));
    if(ReplaySeconds>=SceneDurationSeconds)ReplayPause();
}
void UCutsceneReplayDlg::ReplayPlay(){if(ReplaySeconds>=SceneDurationSeconds)ReplayRewind();bReplayPlaying=true;for(auto Audio:CutsceneAudioComponents)if(Audio.IsValid())Audio->SetPaused(false);}
void UCutsceneReplayDlg::ReplayPause(){bReplayPlaying=false;for(auto Audio:CutsceneAudioComponents)if(Audio.IsValid())Audio->SetPaused(true);}
void UCutsceneReplayDlg::ReplayRewind(){StopCutsceneAudio();ReplaySeconds=0;NextReplayEvent=0;OrbitRate=FVector::ZeroVector;ClearPanelTexture();for(auto& Pair:ReplayActors)if(Pair.Value.IsValid())Pair.Value->SetActorHiddenInGame(false);if(CaptionTextBottom)CaptionTextBottom->SetText(FText::GetEmpty());if(MessageTitleText)MessageTitleText->SetText(FText::GetEmpty());if(MessageSubtitleText)MessageSubtitleText->SetText(FText::GetEmpty());}
void UCutsceneReplayDlg::ReplayStop(){ReplayPause();ReplayRewind();}
void UCutsceneReplayDlg::NativeDestruct(){ReplayStop();for(auto& Pair:ReplayActors)if(Pair.Value.IsValid())Pair.Value->Destroy();ReplayActors.Empty();Super::NativeDestruct();}
FReply UCutsceneReplayDlg::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event){return UUserWidget::NativeOnKeyDown(Geometry,Event);}
