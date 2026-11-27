#include "PerformanceCapture.h"
#include "CoreMinimal.h"
#include "Misc/CoreDelegates.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "RHIStats.h"
#include "RenderingThread.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <atomic>
namespace
{
std::atomic<bool> Enabled{false};
std::atomic<uint64> EconomyNs{0},EconomyCount{0},SimulationNs{0},SimulationCount{0},TextureBytes{0};
FDelegateHandle FrameHandle;
double Started=0,LastMemory=0,Duration=60,Warmup=10;
FString Scenario;
bool bEditorWorld=false;
TArray<TSharedPtr<FJsonValue>> Samples;
void WriteCapture()
{
    if(!Enabled.exchange(false))return;
    auto Root=MakeShared<FJsonObject>();Root->SetStringField(TEXT("scenarioLabel"),Scenario);
    Root->SetBoolField(TEXT("allowsEditorWorld"),bEditorWorld);
    Root->SetStringField(TEXT("cpu"),FPlatformMisc::GetCPUBrand());
    Root->SetStringField(TEXT("graphicsAdapter"),GRHIAdapterName);
    Root->SetStringField(TEXT("scope"),TEXT("Observed client frames; scenario label does not automate flight. GPU time may be unavailable. Texture allocation is not total VRAM use."));
    Root->SetNumberField(TEXT("economyWorkerTasks"),double(EconomyCount.load()));
    Root->SetNumberField(TEXT("economyWorkerMeanMs"),EconomyCount.load()?double(EconomyNs.load())/double(EconomyCount.load())/1.e6:0);
    Root->SetNumberField(TEXT("simulationWorkerTasks"),double(SimulationCount.load()));
    Root->SetNumberField(TEXT("simulationWorkerMeanMs"),SimulationCount.load()?double(SimulationNs.load())/double(SimulationCount.load())/1.e6:0);
    Root->SetArrayField(TEXT("samples"),Samples);
    FString Text;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Text));
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Profiling");IFileManager::Get().MakeDirectory(*Folder,true);
    const FString Path=Folder/(TEXT("ClientPerformance-")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+TEXT(".json"));
    if(FFileHelper::SaveStringToFile(Text,*Path)){UE_LOG(LogTemp,Display,TEXT("[PerformanceCapture] Saved %d samples to %s"),Samples.Num(),*Path);}
    else {UE_LOG(LogTemp,Error,TEXT("[PerformanceCapture] Could not write %s"),*Path);}
}
void SampleFrame()
{
    if(!Enabled.load())return;
    const double Now=FPlatformTime::Seconds();
    UWorld* World=nullptr;
    if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(Context.World() && Context.World()->IsGameWorld()){World=Context.World();break;}
    if(!World && bEditorWorld && GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(Context.World()){World=Context.World();break;}
    if(!World)return;
    if(Started<=0)Started=Now;
    const double Elapsed=Now-Started;
    if(Elapsed<Warmup)return;
    if(Elapsed>=Warmup+Duration){WriteCapture();return;}
    auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("elapsedSeconds"),Elapsed-Warmup);
    Row->SetNumberField(TEXT("frameMs"),FApp::GetDeltaTime()*1000);
    Row->SetNumberField(TEXT("gameThreadMs"),FPlatformTime::ToMilliseconds(GGameThreadTime));
    Row->SetNumberField(TEXT("renderThreadMs"),FPlatformTime::ToMilliseconds(GRenderThreadTime));
    const uint32 GpuCycles=GDynamicRHI && !GUsingNullRHI?RHIGetGPUFrameCycles():0;
    if(GpuCycles)Row->SetNumberField(TEXT("gpuMs"),FPlatformTime::ToMilliseconds(GpuCycles));else Row->SetField(TEXT("gpuMs"),MakeShared<FJsonValueNull>());
    if(Now-LastMemory>=1)
    {
        LastMemory=Now;const auto Memory=FPlatformMemory::GetStats();
        Row->SetNumberField(TEXT("processRamBytes"),double(Memory.UsedPhysical));Row->SetNumberField(TEXT("peakProcessRamBytes"),double(Memory.PeakUsedPhysical));
        Row->SetNumberField(TEXT("actorCount"),World->GetActorCount());Row->SetStringField(TEXT("map"),World->GetMapName());
        if(GDynamicRHI && !GUsingNullRHI)
        {
            ENQUEUE_RENDER_COMMAND(SSWCaptureTextureMemory)([](FRHICommandListImmediate&){FTextureMemoryStats Stats;RHIGetTextureMemoryStats(Stats);TextureBytes.store(Stats.StreamingMemorySize+Stats.NonStreamingMemorySize);});
        }
        Row->SetNumberField(TEXT("textureAllocatedBytes"),double(TextureBytes.load()));
    }
    if(Samples.Num()<120000)Samples.Add(MakeShared<FJsonValueObject>(Row));
}
}
namespace SSWPerformanceCapture
{
void Initialize()
{
    if(!FCommandLine::IsInitialized() || !FParse::Param(FCommandLine::Get(),TEXT("SSWPerfCapture")))return;
    if(FrameHandle.IsValid())return;
    FParse::Value(FCommandLine::Get(),TEXT("SSWPerfSeconds="),Duration);Duration=FMath::Clamp(Duration,5.0,600.0);
    FParse::Value(FCommandLine::Get(),TEXT("SSWPerfWarmup="),Warmup);Warmup=FMath::Clamp(Warmup,0.0,300.0);
    FParse::Value(FCommandLine::Get(),TEXT("SSWPerfScenario="),Scenario);
    bEditorWorld=FParse::Param(FCommandLine::Get(),TEXT("SSWPerfEditorWorld"));
    Samples.Reset();EconomyNs=EconomyCount=SimulationNs=SimulationCount=TextureBytes=0;
    Started=0;LastMemory=0;Enabled=true;
    FrameHandle=FCoreDelegates::OnEndFrame.AddStatic(&SampleFrame);
}
void Shutdown()
{
    WriteCapture();if(FrameHandle.IsValid())FCoreDelegates::OnEndFrame.Remove(FrameHandle);FrameHandle.Reset();Samples.Reset();
}
void RecordEconomy(double Seconds)
{if(Enabled.load() && FMath::IsFinite(Seconds) && Seconds>=0){EconomyNs.fetch_add(uint64(Seconds*1.e9));EconomyCount.fetch_add(1);}}
void RecordSimulation(double Seconds)
{if(Enabled.load() && FMath::IsFinite(Seconds) && Seconds>=0){SimulationNs.fetch_add(uint64(Seconds*1.e9));SimulationCount.fetch_add(1);}}
}
