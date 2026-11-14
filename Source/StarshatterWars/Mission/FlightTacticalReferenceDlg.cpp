#include "FlightTacticalReferenceDlg.h"
#include "FlightReferencePanel.h"

TSharedRef<SWidget> UFlightTacticalReferenceDlg::RebuildWidget()
{
    SetIsFocusable(true);
    return SNew(SFlightReferencePanel);
}
