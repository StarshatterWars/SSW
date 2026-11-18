#pragma once
#include "CoreMinimal.h"
class UMenuScreen;
class UWorld;
namespace OfflineCutsceneExport
{
    bool IsRequested();
    bool TryStart(UMenuScreen* Menu);
    void Begin(UWorld* World,const FString& Scene);
    bool Finish(UWorld* World);
}
