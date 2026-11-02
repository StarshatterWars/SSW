#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Reader.h"
#include "Parser.h"
#include "ParseUtil.h"
#include "Term.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FT1ParagraphTest,"Starshatter.Mission.T1Paragraphs",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FT1ParagraphTest::RunTest(const FString&)
{
    TArray<uint8> Bytes;const FString Path=FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/T1.def");
    if(!TestTrue(TEXT("Current T1 DEF loads"),FFileHelper::LoadFileToArray(Bytes,*Path)))return false;
    Bytes.Add(0);Parser Parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
    FString Sitrep;int32 Id=0;
    while(Term* Node=Parser.ParseTerm()){
        if(auto* Def=Node->isDef()){
            if(Def->name()->value()=="sitrep"){Text Value;GetDefText(Value,Def,TCHAR_TO_ANSI(*Path));Sitrep=ANSI_TO_TCHAR(Value.data());}
            if(Def->name()->value()=="id" && Def->term()->isNumber())Id=int32(Def->term()->isNumber()->value());
        }
        delete Node;
    }
    Sitrep.ReplaceInline(TEXT("\r\n"),TEXT("\n"));
    TestEqual(TEXT("Multiline text retains following mission identity"),Id,101);
    TestTrue(TEXT("Parsed situation has a real blank line"),Sitrep.Contains(TEXT("\n\n")));
    TestFalse(TEXT("Parsed situation has no literal backslash-n"),Sitrep.Contains(TEXT("\\n")));
    TestTrue(TEXT("Second paragraph retained"),Sitrep.Contains(TEXT("fleet carrier Archon")));
    return true;
}
#endif
