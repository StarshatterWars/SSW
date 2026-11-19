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
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;
private:
    UFUNCTION() void OnOpened(FString Url);
    UFUNCTION() void OnOpenFailed(FString Url);
    UPROPERTY(Transient) TObjectPtr<class UMediaPlayer> Player;
    UPROPERTY(Transient) TObjectPtr<class UMediaTexture> Texture;
    UPROPERTY(Transient) TObjectPtr<class UImage> Picture;
    UPROPERTY(Transient) TObjectPtr<class UMediaSoundComponent> Sound;
    // Slate children are not part of WidgetTree traversal; retain their brush owners.
    UPROPERTY(Transient) TArray<TObjectPtr<class UImage>> ControlImages;
    FText Status;
};
