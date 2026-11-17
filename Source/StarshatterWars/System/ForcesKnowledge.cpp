#include "ForcesKnowledge.h"
EForcesScope FForcesKnowledge::ScopeForLevel(int32 Level)
{ return Level>=5 ? EForcesScope::Neighbors : Level>=3 ? EForcesScope::System : EForcesScope::Sector; }
TArray<FForcesReport> FForcesKnowledge::Filter(const TArray<FForcesReport>& Reports,int32 IFF,FName System,FName Sector,FName SelectedSector,
    EForcesScope Requested,int32 Level,const TSet<FName>& Neighbors)
{
    if (Level<1) return {};
    const auto Maximum=ScopeForLevel(Level); const auto Scope=static_cast<EForcesScope>(FMath::Min(static_cast<uint8>(Maximum),static_cast<uint8>(Requested)));
    const FName AllowedSector=Maximum==EForcesScope::Sector || SelectedSector.IsNone() ? Sector : SelectedSector;
    TArray<FForcesReport> Visible;
    for (const auto& Report : Reports)
    {
        if (!Report.bReported || !Report.bCampaignAllowed || IFF<=0 || Report.IFF<0) continue;
        const FName ReportSystem(*Report.System),ReportSector(*Report.Sector);
        if (ReportSystem!=System && (Scope!=EForcesScope::Neighbors || !Neighbors.Contains(ReportSystem))) continue;
        if (Scope==EForcesScope::Sector && ReportSector!=AllowedSector) continue;
        Visible.Add(Report);
    }
    return Visible;
}
