#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EconomyServiceEndpoint.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyEndpointTest,"Starshatter.Economy.Service.ConfigurableEndpointPolicy",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FEconomyEndpointTest::RunTest(const FString&)
{
    FString Normalized,Error;bool Local=false;
    for(const TCHAR* Url:{TEXT("http://127.0.0.1:17881"),TEXT("http://localhost:17881"),TEXT("https://economy.example.com/ssw"),TEXT("https://[::1]:17881")})
        TestTrue(TEXT("Supported endpoint accepted"),FEconomyServiceEndpoint::Normalize(Url,Normalized,Local,Error));
    for(const TCHAR* Url:{TEXT("http://economy.example.com:17881"),TEXT("https://name:password@example.com"),TEXT("https://example.com?key=secret"),TEXT("https://example.com/#fragment"),TEXT("file:///C:/SSW"),TEXT("http://127.0.0.1:99999"),TEXT("https://[:::]:17881")})
        TestFalse(TEXT("Unsafe or invalid endpoint rejected"),FEconomyServiceEndpoint::Normalize(Url,Normalized,Local,Error));
    TestTrue(TEXT("Trailing slash normalizes"),FEconomyServiceEndpoint::Normalize(TEXT("https://economy.example.com/ssw/"),Normalized,Local,Error));
    TestEqual(TEXT("Requests preserve configured server prefix"),Normalized,FString(TEXT("https://economy.example.com/ssw")));
    TestFalse(TEXT("Remote endpoint never implies local autostart"),Local);return true;
}
#endif
