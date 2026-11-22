#pragma once
#include "Components/NativeWidgetHost.h"
#include "PanelCloseGlyph.generated.h"

// Recreate the geometric glyph after UMG releases or reparents its Slate tree.
UCLASS()
class STARSHATTERWARS_API UPanelCloseGlyph : public UNativeWidgetHost
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
