#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldImpactMaterialContractTest,"Starshatter.Presentation.ShieldImpact.RadialMaterialContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShieldImpactMaterialContractTest::RunTest(const FString&)
{
    auto* Material=LoadObject<UMaterial>(nullptr,TEXT("/Game/Effects/Shields/M_SSW_ShieldImpact_Radial.M_SSW_ShieldImpact_Radial"));
    if(!TestNotNull(TEXT("Owned radial fallback material generated"),Material))return false;
    TestTrue(TEXT("Additive legacy shield style retained"),Material->GetBlendMode()==BLEND_Additive);
    TestTrue(TEXT("Both impact-patch sides visible"),Material->IsTwoSided());
    TArray<FMaterialParameterInfo> Scalars,Textures;TArray<FGuid> ScalarIds,TextureIds;
    Material->GetAllScalarParameterInfo(Scalars,ScalarIds);Material->GetAllTextureParameterInfo(Textures,TextureIds);
    TestTrue(TEXT("Native lifetime fade parameter retained"),Scalars.ContainsByPredicate([](const auto& Info){return Info.Name==TEXT("Opacity");}));
    TestTrue(TEXT("Per-ship original shield texture parameter retained"),Textures.ContainsByPredicate([](const auto& Info){return Info.Name==TEXT("ShieldTexture");}));
    return true;
}
#endif
