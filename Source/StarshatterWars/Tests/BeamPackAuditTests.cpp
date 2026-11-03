#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "NiagaraParameterStore.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/Beam/ParticleModuleBeamSource.h"
#include "Particles/Beam/ParticleModuleBeamTarget.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeamPackAudit,"Starshatter.Effects.BeamPackAudit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBeamPackAudit::RunTest(const FString&)
{
 FString Out;
 for(const TCHAR* Group:{TEXT("beam"),TEXT("laser")})for(int I=1;I<=(FString(Group)==TEXT("beam")?14:6);++I){
  const FString Name=FString::Printf(TEXT("ky_%s%02d"),Group,I);
  auto* System=LoadObject<UNiagaraSystem>(nullptr,*(TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_")+Name));if(!System)continue;
  Out+=Name+TEXT("\n");TArray<FNiagaraVariable> Vars;System->GetExposedParameters().GetParameters(Vars);
  for(const auto& V:Vars){Out+=V.GetName().ToString()+TEXT(" ")+V.GetType().GetName()+TEXT(" ");
   const uint8* D=System->GetExposedParameters().GetParameterData(V);
   if(D && V.GetSizeInBytes()==4)Out+=FString::SanitizeFloat(*reinterpret_cast<const float*>(D));
   if(D && V.GetSizeInBytes()==12)Out+=reinterpret_cast<const FVector3f*>(D)->ToString();
   if(D && V.GetSizeInBytes()==16)Out+=reinterpret_cast<const FLinearColor*>(D)->ToString();
   Out+=TEXT("\n");
  }
  if(auto* P=LoadObject<UParticleSystem>(nullptr,*(TEXT("/Game/Beam_and_laserFX01/Particles/P_")+Name)))
   for(const auto& E:P->Emitters)if(E && E->LODLevels.Num())for(const auto& M:E->LODLevels[0]->Modules){
    if(auto* S=Cast<UParticleModuleBeamSource>(M))Out+=FString::Printf(TEXT("source method=%d absolute=%d\n"),int(S->SourceMethod),S->bSourceAbsolute);
    if(auto* T=Cast<UParticleModuleBeamTarget>(M))Out+=FString::Printf(TEXT("target method=%d absolute=%d\n"),int(T->TargetMethod),T->bTargetAbsolute);
   }
 }
 FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("BeamPackParameters.txt")));return true;
}
#endif
