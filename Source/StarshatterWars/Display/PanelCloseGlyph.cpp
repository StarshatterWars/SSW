#include "PanelCloseGlyph.h"
#include "PanelCloseIcon.h"

TSharedRef<SWidget> UPanelCloseGlyph::RebuildWidget()
{
    SetContent(SNew(SPanelCloseIcon));
    return Super::RebuildWidget();
}
