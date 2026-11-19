#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "AgentCatalogSubsystem.h"
#include "AgentDirectoryRules.h"
#include "PlayerCharacter.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentDirectoryTest, "Starshatter.Mission.AgentDirectoryScopes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentDirectoryTest::RunTest(const FString&)
{
    FS_AgentCatalogRow Neutral;
    Neutral.Empire = EEMPIRE_NAME::Neutral;
    TestTrue(TEXT("Neutral officers are visible to Alliance pilots"), Neutral.ServesEmpire(EEMPIRE_NAME::Terellian));
    TestTrue(TEXT("Neutral officers are visible to Marakan pilots"), Neutral.ServesEmpire(EEMPIRE_NAME::Marakan));
    TestFalse(TEXT("No pilot identity cannot bypass the filter"), Neutral.ServesEmpire(EEMPIRE_NAME::NONE));
    FS_AgentCatalogRow Rival;
    Rival.Empire = EEMPIRE_NAME::Dantari;
    TestFalse(TEXT("Other empire officers are hidden"), Rival.ServesEmpire(EEMPIRE_NAME::Terellian));
    TestTrue(TEXT("Own empire officers remain visible"), Rival.ServesEmpire(EEMPIRE_NAME::Dantari));
    const TSet<FName> Neighbors{FName(TEXT("Adjacent"))};
    TestTrue(TEXT("Same-system match ignores case"), AgentDirectoryRules::IncludesSystem(TEXT("Janus"), TEXT("JANUS"), Neighbors, EAgentDirectoryScope::SameSystem));
    TestFalse(TEXT("Same-system scope excludes adjacent systems"), AgentDirectoryRules::IncludesSystem(TEXT("Janus"), TEXT("Adjacent"), Neighbors, EAgentDirectoryScope::SameSystem));
    TestTrue(TEXT("One jump includes origin"), AgentDirectoryRules::IncludesSystem(TEXT("Janus"), TEXT("Janus"), Neighbors, EAgentDirectoryScope::OneJump));
    TestTrue(TEXT("One jump includes a direct link"), AgentDirectoryRules::IncludesSystem(TEXT("Janus"), TEXT("Adjacent"), Neighbors, EAgentDirectoryScope::OneJump));
    TestFalse(TEXT("One jump does not include a second hop"), AgentDirectoryRules::IncludesSystem(TEXT("Janus"), TEXT("Distant"), Neighbors, EAgentDirectoryScope::OneJump));
    TestFalse(TEXT("Unknown location cannot imply local access"), AgentDirectoryRules::IncludesSystem(NAME_None, TEXT("Janus"), Neighbors, EAgentDirectoryScope::SameSystem));
    TestFalse(TEXT("An empty system never appears"), AgentDirectoryRules::IncludesSystem(TEXT("Janus"), NAME_None, Neighbors, EAgentDirectoryScope::EmpireWide));

    auto* Catalog = NewObject<UAgentCatalogSubsystem>(NewObject<UGameInstance>());
    Catalog->LoadCatalog();
    auto* Previous = PlayerCharacter::GetCurrentPlayer();
    ON_SCOPE_EXIT { PlayerCharacter::SetCurrentPlayer(Previous); };
    PlayerCharacter Pilot;
    Pilot.SetEmpire(EEMPIRE_NAME::Terellian);
    PlayerCharacter::SetCurrentPlayer(&Pilot);
    const auto Empire = Catalog->GetDirectorySystems(TEXT("Janus"), {}, EAgentDirectoryScope::EmpireWide);
    const auto Local = Catalog->GetDirectorySystems(TEXT("Janus"), {}, EAgentDirectoryScope::SameSystem);
    TestTrue(TEXT("Local system remains available"), Local.Num() == 1 && Local.Contains(TEXT("Janus")));
    TestTrue(TEXT("Empire-wide includes distant postings"), Empire.Num() > Local.Num());
    TSet<FName> Adjacent;
    for (FName System : Empire) if (System != TEXT("Janus")) { Adjacent.Add(System); break; }
    const auto Nearby = Catalog->GetDirectorySystems(TEXT("Janus"), Adjacent, EAgentDirectoryScope::OneJump);
    TestEqual(TEXT("Only origin and the supplied direct neighbor appear"), Nearby.Num(), 2);
    for (FName System : Empire)
        for (FName Id : Catalog->GetSystemAgents(System))
        {
            const auto* Agent = Catalog->FindAgent(Id);
            TestTrue(TEXT("Every selected agent belongs to the selected system"), Agent && FName(*Agent->System) == System);
            TestTrue(TEXT("All scopes retain hard empire boundaries"), Agent && Agent->ServesEmpire(Pilot.GetEmpire()));
        }
    TestTrue(TEXT("Clearing system selection clears agents"), Catalog->GetSystemAgents(NAME_None).IsEmpty());
    Pilot.SetEmpire(EEMPIRE_NAME::Marakan);
    TestTrue(TEXT("A different empire cannot retain Janus agents"), Catalog->GetSystemAgents(TEXT("Janus")).IsEmpty());
    return true;
}
#endif
