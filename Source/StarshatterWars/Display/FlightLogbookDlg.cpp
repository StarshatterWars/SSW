#include "FlightLogbookDlg.h"
#include "PlayerDlg.h"

TSharedRef<SWidget> UFlightLogbookDlg::RebuildWidget()
{
    return BuildPanelContent(GetGameInstance());
}

TSharedRef<SWidget> UFlightLogbookDlg::BuildPanelContent(UGameInstance* Instance) const
{
    return UPlayerDlg::BuildFlightStatistics(Instance);
}
