#include "EconomyAuthority.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProperties.h"
#include "Misc/ScopeLock.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
namespace SSWEconomyAuthority
{
namespace
{
FCriticalSection WriterMutex;
#if PLATFORM_WINDOWS
struct FWriterLease {HANDLE Handle=nullptr;~FWriterLease(){if(Handle)CloseHandle(Handle);}};
FWriterLease& Lease(){static FWriterLease Value;return Value;}
#endif
}
FString StateDirectory(){FString Override;if(FParse::Value(FCommandLine::Get(),TEXT("SSWEconomyStateDir="),Override)&&!Override.IsEmpty())return FPaths::ConvertRelativePathToFull(Override);return FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("Fractal Dev Games/FractalEconomy/LocalEconomy"));}
FString MarkerPath(){return FPaths::Combine(StateDirectory(),TEXT("client-handoff.required"));}
namespace {bool& ServiceOnly(){static bool Required=FPlatformProperties::RequiresCookedData()||FParse::Param(FCommandLine::Get(),TEXT("SSWExternalEconomy"))||IFileManager::Get().FileExists(*MarkerPath());return Required;}}
bool RequiresService(){if(IsRunningCommandlet()&&!FParse::Param(FCommandLine::Get(),TEXT("SSWIntegrationEconomy")))return false;FScopeLock Lock(&WriterMutex);return ServiceOnly();}
bool MayAdvanceLocally()
{
    if(RequiresService())return false;
    if(IsRunningCommandlet())return true;
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WriterMutex);if(ServiceOnly())return false;
    auto& Writer=Lease();if(Writer.Handle)return true;
    if(IFileManager::Get().FileExists(*MarkerPath())){ServiceOnly()=true;return false;}
    SetLastError(ERROR_SUCCESS);HANDLE Candidate=CreateMutexW(nullptr,0,L"Local\\SSW-NPCEconomy-Writer-v1");
    if(!Candidate)return false;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(Candidate);return false;}
    Writer.Handle=Candidate;
#endif
    return true;
}
bool BeginHandoff(FString& Reason)
{
    // This marker precedes releasing the writer. A crash in either following step
    // leaves the game fail-closed instead of forking the NPC world on restart.
    FScopeLock Lock(&WriterMutex);
    const auto File=MarkerPath();IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
    const ANSICHAR Text[]="SSW NPC authority handoff v1\n";
    TUniquePtr<IFileHandle> Handle(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*File));
    if(!Handle||!Handle->Write(reinterpret_cast<const uint8*>(Text),sizeof(Text)-1)||!Handle->Flush(true)){Reason=TEXT("Could not durably persist the NPC handoff fence.");return false;}
    Handle.Reset();
    ServiceOnly()=true;
#if PLATFORM_WINDOWS
    auto& Writer=Lease();if(Writer.Handle){CloseHandle(Writer.Handle);Writer.Handle=nullptr;}
#endif
    return true;
}
}
