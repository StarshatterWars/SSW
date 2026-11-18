#include "RecordedCutsceneDlg.h"
#include "Blueprint/WidgetTree.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "MediaSoundComponent.h"
#include "Components/Image.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "MissionUIStyle.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
FString URecordedCutsceneDlg::MoviePath(int32 Number,const FString& Scene)
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("Movies")/FString::Printf(TEXT("Campaign%02d"),Number)/(FPaths::MakeValidFileName(Scene)+TEXT(".mp4")));
}
TSharedRef<SWidget> URecordedCutsceneDlg::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!Player)
    {
        Player=NewObject<UMediaPlayer>(this);Player->PlayOnOpen=true;Player->SetLooping(false);
        Player->OnMediaOpened.AddDynamic(this,&URecordedCutsceneDlg::OnOpened);
        Player->OnMediaOpenFailed.AddDynamic(this,&URecordedCutsceneDlg::OnOpenFailed);
        Texture=NewObject<UMediaTexture>(this);Texture->SetMediaPlayer(Player);Texture->UpdateResource();
        Picture=WidgetTree->ConstructWidget<UImage>();Picture->SetBrushResourceObject(Texture);
        Picture->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    auto Controls=SNew(SHorizontalBox);
    auto Add=[&](const TCHAR* Icon,const TCHAR* Tip,TFunction<void()> Action)
    {
        auto* Image=WidgetTree->ConstructWidget<UImage>();
        const FString Path=FString::Printf(TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/%s.%s"),Icon,Icon);
        MissionUIStyle::SetActionIcon(Image,*Path,FLinearColor::White);Image->SetDesiredSizeOverride(FVector2D(24));
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        Controls->AddSlot().AutoWidth().Padding(4)
            [SNew(SBox).WidthOverride(44).HeightOverride(36)
                [SNew(SButton).ToolTipText(FText::FromString(Tip))
                    .OnClicked_Lambda([Action](){Action();return FReply::Handled();})[Image->TakeWidget()]]];
    };
    Add(TEXT("T_Play_Rev_256x256"),TEXT("Restart cutscene"),[this](){Player->Seek(FTimespan::Zero());Player->Play();});
    Add(TEXT("T_Play_256x256"),TEXT("Play cutscene"),[this](){Player->Play();});
    Add(TEXT("T_Pause_256x256"),TEXT("Pause cutscene"),[this](){Player->Pause();});
    Add(TEXT("T_Stop_256x256"),TEXT("Stop cutscene"),[this](){Player->Pause();Player->Seek(FTimespan::Zero());});
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1)
        [SNew(SBox).MinAspectRatio(16.f/9.f).MaxAspectRatio(16.f/9.f)[Picture->TakeWidget()]]
        + SVerticalBox::Slot().AutoHeight()
        [SNew(SSlider).ToolTipText(INVTEXT("Cutscene position"))
            .Value_Lambda([this](){const double Duration=Player->GetDuration().GetTotalSeconds();return Duration>0?float(Player->GetTime().GetTotalSeconds()/Duration):0.f;})
            .OnValueChanged_Lambda([this](float Value){Player->Seek(FTimespan::FromSeconds(Player->GetDuration().GetTotalSeconds()*Value));})]
        + SVerticalBox::Slot().AutoHeight()[Controls]
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this](){return Status;}).Font(MissionUIStyle::GetRowFont(12))];
}
bool URecordedCutsceneDlg::LoadMovie(const FString& Filename)
{
    if(!Player)TakeWidget();
    if(!IFileManager::Get().FileExists(*Filename))
    {Status=INVTEXT("This cutscene has not been exported yet.");return false;}
    if(!Sound && GetWorld())
    {
        auto* Owner=GetWorld()->GetWorldSettings();Sound=NewObject<UMediaSoundComponent>(Owner);
        Owner->AddInstanceComponent(Sound);Sound->SetMediaPlayer(Player);Sound->RegisterComponent();Sound->UpdatePlayer();Sound->Start();
    }
    Status=INVTEXT("Loading cutscene...");return Player->OpenFile(Filename);
}
void URecordedCutsceneDlg::OnOpened(FString Url){Status=FText::GetEmpty();UE_LOG(LogTemp,Display,TEXT("[IntelMovie] Opened recorded cutscene: %s"),*Url);}
void URecordedCutsceneDlg::OnOpenFailed(FString Url){Status=INVTEXT("The cutscene video could not be opened.");UE_LOG(LogTemp,Error,TEXT("[IntelMovie] Open failed: %s"),*Url);}
void URecordedCutsceneDlg::NativeDestruct(){if(Player)Player->Close();if(Sound){Sound->Stop();Sound->DestroyComponent();Sound=nullptr;}Super::NativeDestruct();}
