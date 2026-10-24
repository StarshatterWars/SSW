#pragma once
#include "CoreMinimal.h"
// Legacy FlightDeck::Orient lays four hoops at 1/4, 1/2, 3/4 and 1 of the
// end-to-start vector. These named cues identify the outer, middle and inner
// stations on that same final corridor; they do not redefine the AI route.
namespace RecoveryGuidance
{
    inline FVector Marker(const FVector& Start,const FVector& End,int32 Index)
    { const double Fraction=Index==0?1.0:Index==1?0.5:0.25;return End+(Start-End)*Fraction; }
    inline const TCHAR* Name(int32 Index) {return Index==0?TEXT("OUTER"):Index==1?TEXT("MIDDLE"):TEXT("INNER");}
}
