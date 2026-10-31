#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "GameStructs.h"
#include "Mission.h"
#include "Campaign.h"
#include "MissionInfo.h"
#include "CampaignPresentationRules.h"
#include "MissionElement.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "FormattingUtils.h"
#include "Misc/ScopeExit.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"

class FTrainingCampaignFixture : public Campaign { public: using Campaign::Campaign; using Campaign::SetCampaignData; };

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCampaignOneMissionAudit, "Starshatter.Campaign1.Missions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FCampaignOneMissionAudit::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    const TCHAR* Labels[]={TEXT("101 Basic Space Flight"),TEXT("102 Target Practice"),TEXT("103 Flight School"),TEXT("104 Ground Strike"),TEXT("105 Training Cruise"),TEXT("106 Advanced Starship Combat"),TEXT("107 Carrier Operations")};
    for(int32 Index=0;Index<7;++Index){Names.Add(Labels[Index]);Commands.Add(FString::FromInt(101+Index));}
}
bool FCampaignOneMissionAudit::RunTest(const FString& Parameters)
{
    const int32 Id=FCString::Atoi(*Parameters);
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_Campaign.DT_Campaign"));
    if(!TestNotNull(TEXT("Campaign table"),Table))return false;
    const FS_Campaign* CampaignRow=nullptr;
    for(const auto& Pair:Table->GetRowMap())
    {
        const auto* Candidate=reinterpret_cast<const FS_Campaign*>(Pair.Value);
        if(Candidate->Name.Equals(TEXT("Operation Live Fire"),ESearchCase::IgnoreCase)){CampaignRow=Candidate;break;}
    }
    if(!TestNotNull(TEXT("Campaign 1 authored row"),CampaignRow))return false;
    const auto Original=ShipDesignRegistry::GetAll();
    const auto OriginalWeapons=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT{
        ShipDesignRegistry::Clear();WeaponDesignRegistry::Clear();
        for(const auto& Pair:OriginalWeapons)WeaponDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);
        for(const auto& Pair:Original)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);
    };
    auto* Weapons=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    auto* Designs=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if(!TestNotNull(TEXT("Weapon design table"),Weapons)||!TestNotNull(TEXT("Ship design table"),Designs))return false;
    for(const auto& Pair:Weapons->GetRowMap())WeaponDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FWeaponDesign*>(Pair.Value));
    for(const auto& Pair:Designs->GetRowMap())ShipDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FShipDesign*>(Pair.Value));
    FTrainingCampaignFixture Fixture(1,"Campaign 1 verification",true);
    Fixture.LoadFromData(*CampaignRow);
    Fixture.SetCampaignData(CampaignRow);
    const FS_CampaignMission* Row=Fixture.FindCampaignMissionById(Id);
    if(const MissionInfo* Info=Fixture.GetMissionInfo(Id))
    {
        TestFalse(TEXT("Training mission is not hidden as a cutscene"),Info->is_cutscene);
        const auto* Entry=CampaignRow->MissionList.FindByPredicate([Id](const FS_CampaignMissionList& M){return M.Id==Id;});
        if(TestNotNull(TEXT("Authored mission-list entry"),Entry))
            TestEqual(TEXT("Mission metadata start remains campaign-relative"),Info->start,UFormattingUtils::ParseStarshatterTime(Entry->Start));
    }
    else AddError(TEXT("Training mission metadata missing"));
    if(!TestNotNull(TEXT("Authored training mission"),Row))return false;
    TestEqual(TEXT("Stored mission ID matches its def"),Row->Id,Id);
    TestTrue(TEXT("Training remains playable rather than a cutscene"),Row->MissionType==EMissionType::TRAINING);
    TestTrue(TEXT("Mission system map is available"),FPackageName::DoesPackageExist(TEXT("/Game/Maps/")+Row->MissionSystem));
    TArray<FString> MissingModels;
    for(const auto& Element:Row->Element)
    {
        if(!Element.Design.IsEmpty()&&!ShipDesignRegistry::FindLegacy(FName(*Element.Design)))
        {
            const auto* Design=Designs->FindRow<FShipDesign>(FName(*Element.Design),TEXT("Campaign1 verification"),false);
            if(TestNotNull(*FString::Printf(TEXT("Authored design %s"),*Element.Design),Design))ShipDesignRegistry::RegisterDesign(FName(*Element.Design),*Design);
        }
        if(!Element.Design.IsEmpty())
        {
            const FString Path=FString::Printf(TEXT("/Game/Models/%s/BP_%s"),*Element.Design,*Element.Design);
            if(!FPackageName::DoesPackageExist(Path))MissingModels.AddUnique(Element.Design);
        }
    }
    if(Id==107)TestEqual(TEXT("Carrier Operations retains legacy elapsed-hours start"),UFormattingUtils::ParseStarshatterTime(Row->StartTime),466200);
    Mission* Loaded=Fixture.GetMission(Id);
    if(!TestNotNull(TEXT("Production campaign loader resolves full training mission"),Loaded))return false;
    Mission& Converted=*Loaded;
    TestTrue(TEXT("Actual authored mission converts successfully"),Converted.IsOK());
    TestEqual(TEXT("Selected mission identity survives loading"),Converted.GetIdentity(),Id);
    TestEqual(TEXT("All authored elements survive conversion"),Converted.GetElements().size(),Row->Element.Num());
    int32 PlayerElements=0;
    for(int32 Index=0;Index<Converted.GetElements().size();++Index)
    {
        auto* Element=Converted.GetElements()[Index];
        if(Element&&Element->GetPlayerIndex()>0)
        {
            ++PlayerElements;
            TestFalse(TEXT("Player element has a resolved region"),FString(UTF8_TO_TCHAR(Element->GetRegion().data())).IsEmpty());
        }
    }
    TestTrue(TEXT("Training mission has a player-controlled element"),PlayerElements>0);
    TestEqual(TEXT("Authored training events retained"),Converted.GetEvents().size(),Row->Event.Num());
    if(!MissingModels.IsEmpty())AddWarning(FString::Printf(TEXT("Mission %d unresolved model Blueprint mappings (live visual review required): %s"),Id,*FString::Join(MissingModels,TEXT(", "))));
    FString ReportDir;
    if(FParse::Value(FCommandLine::Get(),TEXT("Campaign1ReportDir="),ReportDir))
    {
        const FString Report=FString::Printf(TEXT("Mission: %d %s\nSystem: %s\nRegion: %s\nElements: %d\nPlayer elements: %d\nEvents: %d\nUnresolved Blueprint mappings: %s\nData/conversion audit only; live play is recorded separately.\n"),Id,*Row->MissionName,*Row->MissionSystem,*Row->MissionRegion,Row->Element.Num(),PlayerElements,Row->Event.Num(),*FString::Join(MissingModels,TEXT(", ")));
        TestTrue(TEXT("Per-mission report saved"),FFileHelper::SaveStringToFile(Report,*(ReportDir/FString::Printf(TEXT("Mission%d.txt"),Id))));
    }
    return true;
}
#endif
