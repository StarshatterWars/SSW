#include "EconomyServiceEndpoint.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Internationalization/Regex.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace
{
bool HexGroup(const FString& Group)
{if(Group.IsEmpty()||Group.Len()>4)return false;for(TCHAR C:Group)if(!FChar::IsHexDigit(C))return false;return true;}
bool IPv6(const FString& Host,bool& Local)
{
    if(Host.Contains(TEXT(":::")))return false;
    FString Left,Right;TArray<FString> L,R;const bool Compressed=Host.Split(TEXT("::"),&Left,&Right);
    if(Compressed){if(Right.Contains(TEXT("::")))return false;Left.ParseIntoArray(L,TEXT(":"),false);Right.ParseIntoArray(R,TEXT(":"),false);if(Left.IsEmpty())L.Reset();if(Right.IsEmpty())R.Reset();}
    else Host.ParseIntoArray(L,TEXT(":"),false);
    if((Compressed&&L.Num()+R.Num()>=8)||(!Compressed&&L.Num()!=8))return false;
    TArray<uint32> Words;for(const auto& G:L){if(!HexGroup(G))return false;Words.Add(uint32(FCString::Strtoui64(*G,nullptr,16)));}
    if(Compressed)while(Words.Num()<8-R.Num())Words.Add(0);
    for(const auto& G:R){if(!HexGroup(G))return false;Words.Add(uint32(FCString::Strtoui64(*G,nullptr,16)));}
    Local=Words.Num()==8&&Words.Last()==1;for(int32 I=0;I<7&&Local;++I)Local=Words[I]==0;
    return true;
}
}
bool FEconomyServiceEndpoint::Normalize(const FString& Input,FString& Out,bool& Local,FString& Error)
{
    Out.Empty();Local=false;Error=TEXT("Use an HTTP localhost address or an authenticated HTTPS economy endpoint.");
    if(Input.IsEmpty()||Input.Len()>2048||Input.Contains(TEXT("\\")))return false;
    for(TCHAR C:Input)if(C<32||C==127)return false;
    FRegexMatcher Match(FRegexPattern(TEXT("^(http|https)://(\\[[0-9a-fA-F:]+\\]|[A-Za-z0-9][A-Za-z0-9.-]*)(?::([0-9]{1,5}))?(/[^\\s?#]*)?$")),Input);
    if(!Match.FindNext())return false;
    const FString Scheme=Match.GetCaptureGroup(1);FString Host=Match.GetCaptureGroup(2).ToLower();
    if(Host.StartsWith(TEXT("["))){if(!IPv6(Host.Mid(1,Host.Len()-2),Local))return false;}
    else{
        TArray<FString> Labels;Host.ParseIntoArray(Labels,TEXT("."),false);
        for(const auto& Label:Labels)if(Label.IsEmpty()||Label.Len()>63||Label.StartsWith(TEXT("-"))||Label.EndsWith(TEXT("-")))return false;
        Local=Host==TEXT("localhost")||Host==TEXT("127.0.0.1");
        bool Numeric=Labels.Num()==4;for(const auto& Label:Labels)Numeric&=Label.IsNumeric();
        if(Numeric)for(const auto& Label:Labels)if(FCString::Atoi(*Label)>255)return false;
    }
    if(Scheme==TEXT("http")&&!Local)return false;
    int32 Port=Scheme==TEXT("https")?443:80;const FString PortText=Match.GetCaptureGroup(3);
    if(!PortText.IsEmpty()&&(!LexTryParseString(Port,*PortText)||Port<1||Port>65535))return false;
    FString Path=Match.GetCaptureGroup(4);while(Path.EndsWith(TEXT("/")))Path.LeftChopInline(1);
    Out=Scheme+TEXT("://")+Host;
    if(Port!=(Scheme==TEXT("https")?443:80))Out+=TEXT(":")+FString::FromInt(Port);
    Out+=Path;Error.Empty();return true;
}
FString FEconomyServiceEndpoint::SettingsPath()
{
    FString Override;if(FCommandLine::IsInitialized()&&FParse::Value(FCommandLine::Get(),TEXT("SSWServicesConfig="),Override))return Override;
    return FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("Fractal Dev Games/SSW/service-endpoints.json"));
}
FEconomyServiceEndpoint FEconomyServiceEndpoint::Load(const FString& Given)
{
    FEconomyServiceEndpoint Result;const FString Path=Given.IsEmpty()?SettingsPath():Given;
    const auto Size=IFileManager::Get().FileSize(*Path);if(Size<0&&!IFileManager::Get().FileExists(*Path))return Result;
    FString Text;TSharedPtr<FJsonObject> Root;const TSharedPtr<FJsonObject>* Economy=nullptr;double Schema=0,Revision=0;FString Url;
    if(Size<0||Size>16384||!FFileHelper::LoadFileToString(Text,*Path)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root||
       !Root->TryGetNumberField(TEXT("schema"),Schema)||Schema!=1||!Root->TryGetNumberField(TEXT("revision"),Revision)||!FMath::IsFinite(Revision)||Revision<0||Revision>9007199254740991.0||Revision!=FMath::FloorToDouble(Revision)||
       !Root->TryGetObjectField(TEXT("economy"),Economy)||!Economy||!Economy->IsValid()||!(*Economy)->TryGetStringField(TEXT("endpoint"),Url)||!(*Economy)->TryGetBoolField(TEXT("autoStartLocal"),Result.bAutoStartLocal))
    {Result.bValid=false;Result.Error=TEXT("Economy endpoint settings are invalid or unreadable. Local fallback is disabled.");return Result;}
    Result.bValid=Normalize(Url,Result.Url,Result.bLoopback,Result.Error);Result.Revision=int64(Revision);return Result;
}
