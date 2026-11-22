#pragma once
#include "CoreMinimal.h"

// Runtime-only authentication lease. Monotonic server time; never saved or supplied by clients.
class FFractalVerseSession
{
public:
    bool Bind(FName VerifiedPrincipal,double Now)
    {
        if (VerifiedPrincipal.IsNone() || !FMath::IsFinite(Now) || Now<0 ||
            (!Owner.IsNone() && Owner!=VerifiedPrincipal)) { Clear();return false; }
        Owner=VerifiedPrincipal; Issued=Now; Expires=Now+300; return true;
    }
    FName Principal(double Now) const
    { return FMath::IsFinite(Now) && Now>=Issued && Now<Expires ? Owner : NAME_None; }
    bool NeedsRefresh(double Now) const { return !Principal(Now).IsNone() && Now>=Expires-60; }
    bool MayBind(FName Principal) const { return Owner.IsNone() || Owner==Principal; }
    bool HasBinding() const { return !Owner.IsNone(); }
    void Clear() { Owner=NAME_None; Issued=Expires=0; }
private:
    FName Owner;
    double Issued=0,Expires=0;
};
