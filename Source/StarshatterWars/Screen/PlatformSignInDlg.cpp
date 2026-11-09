#include "PlatformSignInDlg.h"
#include "SSWGameInstance.h"

TSharedRef<SWidget> UPlatformSignInDlg::RebuildWidget()
{
    if (USSWGameInstance* Instance = Cast<USSWGameInstance>(GetGameInstance()))
        return Instance->BuildPlatformSignInContent();

    return Super::RebuildWidget();
}
