#pragma once
#include "BaseScreen.h"
#include "RecordedCutsceneDlg.generated.h"
UCLASS()
class STARSHATTERWARS_API URecordedCutsceneDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    static FString MoviePath(int32 CampaignNumber,const FString& Scene);
    bool LoadMovie(const FString& Filename);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;
private:
    UFUNCTION() void OnOpened(FString Url);
    UFUNCTION() void OnOpenFailed(FString Url);
    UPROPERTY(Transient) TObjectPtr<class UMediaPlayer> Player;
    UPROPERTY(Transient) TObjectPtr<class UMediaTexture> Texture;
    UPROPERTY(Transient) TObjectPtr<class UImage> Picture;
    UPROPERTY(Transient) TObjectPtr<class UMediaSoundComponent> Sound;
    FText Status;
};
