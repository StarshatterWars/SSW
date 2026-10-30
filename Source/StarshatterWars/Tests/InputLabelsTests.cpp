#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarshatterInputLabels.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputLabelHelpersTest,"Starshatter.InputLabels.DisplayHelpers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInputLabelHelpersTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Gear readable"),UStarshatterInputLabels::GetDefaultActionLabel(TEXT("IA_GearToggle")).ToString(),FString(TEXT("Raise / Lower Landing Gear")));
    TestEqual(TEXT("HUD acronym readable"),UStarshatterInputLabels::GetDefaultActionLabel(TEXT("IA_HUDWarnings")).ToString(),FString(TEXT("Toggle HUD Warnings")));
    TestEqual(TEXT("Unknown future action has safe readable fallback"),UStarshatterInputLabels::GetDefaultActionLabel(TEXT("IA_CustomAction")).ToString(),FString(TEXT("Custom Action")));
    TestEqual(TEXT("Boolean type"),UStarshatterInputLabels::GetValueTypeLabel(EInputActionValueType::Boolean).ToString(),FString(TEXT("Button")));
    TestEqual(TEXT("1D axis type"),UStarshatterInputLabels::GetValueTypeLabel(EInputActionValueType::Axis1D).ToString(),FString(TEXT("1D Axis")));
    TestEqual(TEXT("2D axis type"),UStarshatterInputLabels::GetValueTypeLabel(EInputActionValueType::Axis2D).ToString(),FString(TEXT("2D Axis")));
    TestEqual(TEXT("3D axis type"),UStarshatterInputLabels::GetValueTypeLabel(EInputActionValueType::Axis3D).ToString(),FString(TEXT("3D Axis")));
    auto* Action=NewObject<UInputAction>();Action->ActionDescription=FText::FromString(TEXT("Custom Pilot Action"));
    TestEqual(TEXT("Authored description takes precedence"),UStarshatterInputLabels::GetActionLabel(Action).ToString(),FString(TEXT("Custom Pilot Action")));
    Action->ActionDescription=FText::FromString(TEXT("IA_GearToggle"));
    TestFalse(TEXT("Technical description is not exposed"),UStarshatterInputLabels::GetActionLabel(Action).ToString().StartsWith(TEXT("IA_")));
    TestFalse(TEXT("Null action has readable label"),UStarshatterInputLabels::GetActionLabel(nullptr).IsEmpty());
    TestEqual(TEXT("Legacy complex name works without editor display metadata"),UStarshatterInputLabels::GetLegacyActionLabel(EStarshatterInputAction::CyclePrimary).ToString(),FString(TEXT("Cycle Primary Weapon")));
    FEnhancedActionKeyMapping Mapping;Mapping.Key=EKeys::T;
    auto* Shift=NewObject<UInputAction>(GetTransientPackage(),TEXT("IA_ModifierLShift"));
    auto* Chord=NewObject<UInputTriggerChordAction>();Chord->ChordAction=Shift;Mapping.Triggers.Add(Chord);
    TestEqual(TEXT("Shift chord is readable"),UStarshatterInputLabels::GetBindingKeyLabel(Mapping).ToString(),FString(TEXT("Left Shift + T")));
    const UEnum* Enum=StaticEnum<EStarshatterInputAction>();
    for(int32 Index=0;Index<Enum->NumEnums();++Index)
    {
        const FString Name=Enum->GetNameStringByIndex(Index);
        if(Name.EndsWith(TEXT("MAX")))continue;
        TestFalse(*FString::Printf(TEXT("Legacy %s has runtime label"),*Name),UStarshatterInputLabels::GetLegacyActionLabel(static_cast<EStarshatterInputAction>(Enum->GetValueByIndex(Index))).ToString()==TEXT("Unknown Action"));
    }
    return true;
}
namespace
{
    FString InputPropertiesExceptDescription(UInputAction* Action)
    {
        FString Result;
        for(TFieldIterator<FProperty> It(Action->GetClass());It;++It)
        {
            if(It->GetFName()==TEXT("ActionDescription"))continue;
            FString Value;It->ExportTextItem_Direct(Value,It->ContainerPtrToValuePtr<void>(Action),nullptr,Action,PPF_None);
            Result+=It->GetName()+TEXT("=")+Value+TEXT("\n");
        }
        return Result;
    }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputActionAssetsLabelTest,"Starshatter.InputLabels.ActionAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInputActionAssetsLabelTest::RunTest(const FString& Parameters)
{
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    Registry.ScanPathsSynchronous({TEXT("/Game/Input")},true);
    FARFilter Filter;Filter.PackagePaths.Add(TEXT("/Game/Input"));Filter.bRecursivePaths=true;
    Filter.ClassPaths.Add(UInputAction::StaticClass()->GetClassPathName());Filter.bRecursiveClasses=true;
    TArray<FAssetData> Assets;Registry.GetAssets(Filter,Assets);
    Assets.Sort([](const FAssetData& A,const FAssetData& B){return A.PackageName.LexicalLess(B.PackageName);});
    TestTrue(TEXT("Project input actions discovered"),Assets.Num()>0);
    const bool Write=FParse::Param(FCommandLine::Get(),TEXT("WriteInputLabels"));
    TArray<FString> Rows;Rows.Add(TEXT("Asset,Label,InputType,Changed,OtherInputPropertiesPreserved"));
    int32 Changed=0;
    for(const auto& Asset:Assets)
    {
        auto* Action=Cast<UInputAction>(Asset.GetAsset());if(!Action){AddError(TEXT("Input action failed to load"));continue;}
        const FString Before=InputPropertiesExceptDescription(Action);
        bool Updated=false;
        const FString Existing=Action->ActionDescription.ToString().TrimStartAndEnd();
        if(Write && (Existing.IsEmpty() || Existing.StartsWith(TEXT("IA_"),ESearchCase::IgnoreCase) || (Action->GetFName()==TEXT("IA_Quit") && Existing==TEXT("Quit Game"))))
        {
            Action->ActionDescription=UStarshatterInputLabels::GetDefaultActionLabel(Action->GetFName());
            Action->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
            const FString File=FPackageName::LongPackageNameToFilename(Asset.PackageName.ToString(),FPackageName::GetAssetPackageExtension());
            if(!UPackage::SavePackage(Action->GetPackage(),Action,*File,Args))AddError(TEXT("Could not save ")+File);
            Updated=true;++Changed;
        }
        TestFalse(*FString::Printf(TEXT("%s description persisted"),*Asset.AssetName.ToString()),Action->ActionDescription.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s label is readable"),*Asset.AssetName.ToString()),UStarshatterInputLabels::GetActionLabel(Action).ToString().StartsWith(TEXT("IA_")));
        const bool Preserved=Before==InputPropertiesExceptDescription(Action);
        TestTrue(*FString::Printf(TEXT("%s only label changed"),*Asset.AssetName.ToString()),Preserved);
        auto Quote=[](FString Value){Value.ReplaceInline(TEXT("\""),TEXT("\"\""));return TEXT("\"")+Value+TEXT("\"");};
        Rows.Add(Quote(Asset.GetSoftObjectPath().ToString())+TEXT(",")+Quote(UStarshatterInputLabels::GetActionLabel(Action).ToString())+TEXT(",")+Quote(UStarshatterInputLabels::GetActionTypeLabel(Action).ToString())+TEXT(",")+(Updated?TEXT("true"):TEXT("false"))+TEXT(",")+(Preserved?TEXT("true"):TEXT("false")));
    }
    FString Report;if(FParse::Value(FCommandLine::Get(),TEXT("InputLabelsReport="),Report))FFileHelper::SaveStringArrayToFile(Rows,*Report,FFileHelper::EEncodingOptions::ForceUTF8);
    AddInfo(FString::Printf(TEXT("Input actions checked=%d; descriptions updated=%d; write mode=%d"),Assets.Num(),Changed,Write?1:0));
    return true;
}
#endif
