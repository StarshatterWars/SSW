#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "StarshatterGameDataSubsystem.h"
#include "ShipDesignRegistry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStagedIngestionTest, "Starshatter.Ingestion.Standalone",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FStagedIngestionTest::RunTest(const FString&)
{
    UGameInstance* GameInstance = nullptr;
    for (const auto& World : GEngine->GetWorldContexts())
    {
        if (World.OwningGameInstance)
        {
            GameInstance = World.OwningGameInstance;
            break;
        }
    }
    if (!TestNotNull(TEXT("Standalone game instance"), GameInstance)) return false;
    auto* Live = GameInstance->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!TestNotNull(TEXT("Normal campaign ingestion completed"), Live)) return false;

    const auto LoadStaged = [this](const TCHAR* Name)
    {
        FString BaseName(Name);
        BaseName.RemoveFromStart(TEXT("DT_"));
        const FString TestName = TEXT("DT_Test_") + BaseName;
        const FString Path = FString::Printf(TEXT("/Game/Editor/GeneratedDesigns/%s.%s"), *TestName, *TestName);
        auto* Table = LoadObject<UDataTable>(nullptr, *Path);
        TestNotNull(Path, Table);
        if (Table) TestTrue(Path + TEXT(" has rows"), !Table->GetRowMap().IsEmpty());
        return Table;
    };
    for (const TCHAR* Name : {TEXT("DT_SystemDesign"), TEXT("DT_WeaponDesign"), TEXT("DT_CampaignAction"),
        TEXT("DT_CampaignZone"), TEXT("DT_SingleMissions"), TEXT("DT_MultiplayerMissions"),
        TEXT("FRankInfo"), TEXT("FMedalInfo"), TEXT("DT_CombatGroup"),
        TEXT("DT_GalaxyMap"), TEXT("DT_Region")})
        LoadStaged(Name);

    const auto* Overview = LoadStaged(TEXT("DT_OrderOfBattle"));
    const auto* ActiveOverview = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_OrderOfBattle.DT_OrderOfBattle"));
    if (!Overview || !TestNotNull(TEXT("Active OOB for comparison"), ActiveOverview)) return false;
    TestEqual(TEXT("OOB force count"), Overview->GetRowMap().Num(), ActiveOverview->GetRowMap().Num());
    for (const auto& Row : Overview->GetRowMap())
    {
        const auto* Existing = ActiveOverview->GetRowMap().Find(Row.Key);
        if (TestNotNull(Row.Key.ToString() + TEXT(" active OOB key"), Existing))
            TestTrue(Row.Key.ToString() + TEXT(" complete OOB record"),
                FS_OOBForce::StaticStruct()->CompareScriptStruct(Row.Value, *Existing, 0));
    }

    auto* StagedCampaign = NewObject<UStarshatterGameDataSubsystem>(GameInstance);
    StagedCampaign->CampaignDataTable = LoadStaged(TEXT("DT_Campaign"));
    if (!StagedCampaign->CampaignDataTable) return false;
    StagedCampaign->ReadCampaignData();
    for (int32 Index = 1; Index <= 5; ++Index)
    {
        const FS_Campaign* Staged = StagedCampaign->GetCampaignByIndex1Based(Index);
        const FS_Campaign* Runtime = Live->GetCampaignByIndex1Based(Index);
        if (!TestNotNull(TEXT("Staged campaign"), Staged) || !TestNotNull(TEXT("Runtime campaign"), Runtime))
            continue;
        TestEqual(TEXT("Campaign name"), Staged->Name, Runtime->Name);
        TestEqual(TEXT("Campaign action count"), Staged->Action.Num(), Runtime->Action.Num());
        TestEqual(TEXT("Campaign zone count"), Staged->Zone.Num(), Runtime->Zone.Num());
        TestEqual(TEXT("Campaign mission catalog count"), Staged->MissionList.Num(), Runtime->MissionList.Num());
        TestEqual(TEXT("Campaign mission data count"), Staged->Missions.Num(), Runtime->Missions.Num());
        TestEqual(TEXT("Campaign mission template count"), Staged->TemplateMissions.Num(), Runtime->TemplateMissions.Num());
        TestEqual(TEXT("Campaign scripted mission count"), Staged->ScriptedMissions.Num(), Runtime->ScriptedMissions.Num());
    }

    const auto* Scenes = LoadStaged(TEXT("DT_Cutscene"));
    if (!Scenes) return false;
    for (const auto& Pair : Scenes->GetRowMap())
    {
        const auto& Scene = *reinterpret_cast<const FS_CampaignMission*>(Pair.Value);
        TestTrue(Pair.Key.ToString() + TEXT(" has parsed events"), !Scene.Event.IsEmpty());
    }

    const auto* Ships = LoadStaged(TEXT("DT_ShipDesign"));
    if (!Ships) return false;
    for (const TCHAR* Name : {TEXT("ZZA"), TEXT("ZZF"), TEXT("ZZCA"), TEXT("ZZCS"), TEXT("ZZDD"), TEXT("ZZFF")})
    {
        const FName Key(Name);
        const auto* Staged = Ships->FindRow<FShipDesign>(Key, TEXT("Staged ingestion"));
        const auto* Runtime = ShipDesignRegistry::Find(Key);
        if (!TestNotNull(Name, Staged) || !TestNotNull(Name, Runtime)) continue;
        TestEqual(TEXT("Zolon drive count"), Staged->Drive.Num(), Runtime->Drive.Num());
        TestEqual(TEXT("Zolon running light count"), Staged->Navlight.Num(), Runtime->Navlight.Num());
        TestEqual(TEXT("Zolon weapon count"), Staged->Weapon.Num(), Runtime->Weapon.Num());
        if (TestEqual(TEXT("Zolon thruster count"), Staged->Thruster.Num(), Runtime->Thruster.Num()) &&
            !Staged->Thruster.IsEmpty())
            TestEqual(TEXT("Zolon thruster port count"), Staged->Thruster[0].Ports.Num(), Runtime->Thruster[0].Ports.Num());
        if (TestEqual(TEXT("Zolon shield count"), Staged->Shield.Num(), Runtime->Shield.Num()) && !Staged->Shield.IsEmpty())
            TestEqual(TEXT("Zolon shield alias"), Staged->Shield[0].ModelName, Runtime->Shield[0].ModelName);
    }
    return true;
}

#endif
