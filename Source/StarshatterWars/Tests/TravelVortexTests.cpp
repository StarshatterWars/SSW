#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && (PLATFORM_WINDOWS || PLATFORM_MAC)
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "TravelVortexActor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelVortexPackTest,"Starshatter.Travel.ProceduralVortexPack",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTravelVortexPackTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false);FScopedSSWTestWorldContext Context(World);
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Tunnel=World->SpawnActor<ATravelVortexActor>();
    if(!TestNotNull(TEXT("Native travel actor spawns"),Tunnel))return false;
    TestFalse(TEXT("Invalid geometry cannot be built"),Tunnel->Configure(-1,100));
    if(!TestTrue(TEXT("Installed pack builds its actual procedural tunnel"),Tunnel->Configure(1200,16000)))return false;
    TestFalse(TEXT("Cosmetic tunnel has no gameplay collision"),Tunnel->GetActorEnableCollision());
    TestFalse(TEXT("No idle per-frame actor tick"),Tunnel->PrimaryActorTick.bCanEverTick);
    const auto* Section=Tunnel->TunnelMesh->GetProcMeshSection(0);
    if(TestNotNull(TEXT("Mesh section exists"),Section))
    {
        TestTrue(TEXT("Travel mesh stays within its bounded visual budget"),Section->ProcVertexBuffer.Num()>0 && Section->ProcVertexBuffer.Num()<2000);
        for(const auto& V:Section->ProcVertexBuffer)if(V.Position.ContainsNaN()){AddError(TEXT("Tunnel geometry contains non-finite positions"));break;}
        const int32 Vertices=Section->ProcVertexBuffer.Num();Tunnel->Configure(1200,16000);
        TestEqual(TEXT("Repeated configuration does not append geometry"),Tunnel->TunnelMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num(),Vertices);
    }
    auto* Material=Cast<UMaterialInstanceDynamic>(Tunnel->TunnelMesh->GetMaterial(0));
    if(!TestNotNull(TEXT("Actual pack material has an instance"),Material))return false;
    TestTrue(TEXT("Material comes from installed pack"),Material->Parent && Material->Parent->GetPathName().StartsWith(TEXT("/ProceduralVortexTunnel/Materials/M_PlasmaVortex")));
    TArray<FMaterialParameterInfo> Info;TArray<FGuid> Ids;Material->GetAllVectorParameterInfo(Info,Ids);
    for(const auto& Parameter:Info)AddInfo(TEXT("Vortex vector parameter: ")+Parameter.Name.ToString());
    TestTrue(TEXT("Departure and arrival bind authored local colors"),Info.ContainsByPredicate([](const auto& P){return P.Name==TEXT("DefaultColor1");}) && Info.ContainsByPredicate([](const auto& P){return P.Name==TEXT("DefaultColor2");}));
    Info.Reset();Ids.Reset();Material->GetAllScalarParameterInfo(Info,Ids);
    for(const auto& Parameter:Info)AddInfo(TEXT("Vortex scalar parameter: ")+Parameter.Name.ToString());
    Tunnel->SetEnvelope(false,1);TestFalse(TEXT("Departure is visible"),Tunnel->IsHidden());
    Tunnel->SetEnvelope(true,1);TestFalse(TEXT("Arrival is visible"),Tunnel->IsHidden());
    Tunnel->SetEnvelope(true,0);TestTrue(TEXT("Completion hides the presentation"),Tunnel->IsHidden());
    Tunnel->Destroy();TestTrue(TEXT("Abort destroys transient actor"),Tunnel->IsActorBeingDestroyed());
    return true;
}
#endif
