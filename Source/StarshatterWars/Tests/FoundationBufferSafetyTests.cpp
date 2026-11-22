#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FormatUtil.h"
#include "Text.h"
#include "Token.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundationBufferSafetyTest,"Starshatter.Foundation.BoundedFormattingAndTextCopies",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFoundationBufferSafetyTest::RunTest(const FString&)
{
    struct FGuard { char Buffer[4]={}; uint8 Sentinel=0x7f; } Guard;
    FormatNumber(Guard.Buffer,1e15);
    TestEqual(TEXT("Formatter respects actual array boundary"),Guard.Sentinel,uint8(0x7f));
    TestEqual(TEXT("Truncated number is terminated"),Guard.Buffer[3],char(0));
    char Clock[8]={}; FormatTime(Clock,3661);
    TestEqual(TEXT("Small time buffer remains correct"),FString(UTF8_TO_TCHAR(Clock)),FString(TEXT("1:01:01")));
    const char Raw[3]={'a',0,'b'}; Text Original(Raw,3),Copy(Original); Copy[0]='z';
    TestEqual(TEXT("Copy-on-write retains bytes after embedded zero"),Copy[2],'b');
    TestEqual(TEXT("Original remains unchanged"),Original[0],'a');
    Copy[-1]='q'; TestEqual(TEXT("Negative index cannot overwrite text"),Copy[0],'z');
    TestEqual(TEXT("Negative slice is empty"),Original(-1,2).length(),0);
    TestEqual(TEXT("Large slice length cannot overflow"),Original(1,MAX_int32).length(),2);
    Scanner Reader{Text("alpha beta")}; Reader=Reader;
    TestEqual(TEXT("Scanner self assignment retains input"),FString(UTF8_TO_TCHAR(Reader.Get().symbol().data())),FString(TEXT("alpha")));
    Scanner Empty(static_cast<class Reader*>(nullptr)); Scanner EmptyCopy(Empty);
    return true;
}
#endif
