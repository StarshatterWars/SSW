#pragma once
#include "Widgets/SCompoundWidget.h"
#include "ShipSystemIcon.h"
class UWidgetComponent;
class SFighterHUDDetails;
struct FCockpitShortcut { FName Action; const TCHAR* Tooltip; EShipSystemIcon Icon; };
class SCockpitMiniControls : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCockpitMiniControls) {} SLATE_END_ARGS()
    void Construct(const FArguments&, TWeakPtr<SFighterHUDDetails>, UWidgetComponent*);
    void SampleHover();
    bool HandleClick();
    static TConstArrayView<FCockpitShortcut> Shortcuts();
    static int32 CellAt(const FVector2D& Point);
private:
    bool RayPoint(FVector2D&) const;
    TWeakPtr<SFighterHUDDetails> HUD;
    TWeakObjectPtr<UWidgetComponent> Surface;
    int32 Hovered=INDEX_NONE;
};
