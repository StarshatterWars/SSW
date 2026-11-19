#pragma once
#include "GameStructs_System.h"

// Presentation only. No legacy thrust, reactor demand or port data is changed.
namespace EnginePlumeTuning
{
    inline FLinearColor Color(EShipEmpire Empire, EDriveType Drive)
    {
        if(Empire==EShipEmpire::Terellian)return FLinearColor(.08f,.42f,1.f);
        if(Empire==EShipEmpire::Dantari)return FLinearColor(1.f,.38f,.025f);
        if(Empire==EShipEmpire::Marakan)return FLinearColor(.06f,1.f,.22f);
        if(Drive==EDriveType::YELLOW)return FLinearColor(1.f,.38f,.025f);
        if(Drive==EDriveType::GREEN)return FLinearColor(.06f,1.f,.22f);
        if(Drive==EDriveType::RED)return FLinearColor(1.f,.035f,.015f);
        return FLinearColor(.08f,.42f,1.f);
    }
    inline const TCHAR* Variant(EShipEmpire Empire, EDriveType Drive)
    {
        if(Empire==EShipEmpire::Dantari)return TEXT("OrangeYellow");
        if(Empire==EShipEmpire::Marakan)return TEXT("Green");
        if(Empire==EShipEmpire::Terellian)return TEXT("Blue");
        return Drive==EDriveType::YELLOW?TEXT("OrangeYellow"):Drive==EDriveType::GREEN?TEXT("Green"):Drive==EDriveType::RED?TEXT("Red"):TEXT("Blue");
    }
    inline FString SystemPath(EShipEmpire Empire, EDriveType Drive)
    {
        const TCHAR* Name=Variant(Empire,Drive);
        return FString::Printf(TEXT("/Game/Effects/Engines/NS_SSW_Nozzle_%s.NS_SSW_Nozzle_%s"),Name,Name);
    }
    inline double Radius(double Measured,double Fallback,double Separation,double HullRadius)
    {
        const double SafeFallback=FMath::IsFinite(Fallback)&&Fallback>0?Fallback:1.;
        double Result=FMath::IsFinite(Measured)&&Measured>0?Measured:SafeFallback;
        if(FMath::IsFinite(Separation)&&Separation>0)Result=FMath::Min(Result,Separation*.40);
        if(FMath::IsFinite(HullRadius)&&HullRadius>0)Result=FMath::Min(Result,HullRadius*.08);
        return FMath::Max(.001,Result);
    }
    inline float Power(float Value)
    {return FMath::IsFinite(Value)?FMath::Clamp(Value,0.f,1.15f):0.f;}
}
