#include "RuntimeFeaturePolicy.h"
#include "HAL/IConsoleManager.h"
namespace
{
TAutoConsoleVariable<int32> MultiplayerEnabled(TEXT("ssw.MultiplayerEnabled"),0,TEXT("Multiplayer is deferred during universe integration."),ECVF_ReadOnly);
TAutoConsoleVariable<int32> ModEditorEnabled(TEXT("ssw.ModEditorEnabled"),0,TEXT("Mod editing is deferred during universe integration."),ECVF_ReadOnly);
}
bool FSSWFeaturePolicy::Multiplayer(){return MultiplayerEnabled.GetValueOnGameThread()!=0;}
bool FSSWFeaturePolicy::ModEditor(){return ModEditorEnabled.GetValueOnGameThread()!=0;}
