#include "LoadoutsDlg.h"
#include "MissionWeaponDlg.h"
#include "Widgets/SNullWidget.h"
TSharedRef<SWidget> ULoadoutsDlg::RebuildWidget()
{
    return Editor ? Editor->TakeWidget() : SNullWidget::NullWidget;
}
