#include "MissionObjectivesDlg.h"
#include "Mission.h"
void UMissionObjectivesDlg::SetMissionBriefing(const Mission& Mission,FText InTitle,FText Agent,FText Status,
    FSimpleDelegate OnAccept,FSimpleDelegate OnDecline,FSimpleDelegate OnClose)
{
    const auto Text=[](const char* Value){return FText::FromString(UTF8_TO_TCHAR(Value ? Value : ""));};
    const FText BriefingText=FText::Format(INVTEXT("AGENT: {0}\nSTATUS: {1}\nLOCATION: {2} / {3}\n\nBRIEF\n{4}\n\nOBJECTIVES\n{5}\n\nSITUATION\n{6}"),
        Agent,Status,Text(Mission.GetSystem()),Text(Mission.GetRegion()),Text(Mission.GetDescription()),Text(Mission.GetObjective()),Text(Mission.GetSituation()));
    SetDetails(MoveTemp(InTitle),BriefingText,MoveTemp(OnAccept),MoveTemp(OnDecline),MoveTemp(OnClose));
}
