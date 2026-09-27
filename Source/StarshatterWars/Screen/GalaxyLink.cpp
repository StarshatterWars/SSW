// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "GalaxyLink.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"


void UGalaxyLink::ConfigureLine(float Length, float AngleDegrees, FLinearColor Tint /*= FLinearColor::White*/)
{
    if (!LineImage) return;

    if (UCanvasPanelSlot* LinkSlot = Cast<UCanvasPanelSlot>(LineImage->Slot))
    {
        LinkSlot->SetPosition(FVector2D(0.f, 0.f)); // Position at origin of parent
        LinkSlot->SetSize(FVector2D(Length, 2.f));
        LinkSlot->SetAlignment(FVector2D(0.f, 0.5f));
    }

    // Rotate line image
    FWidgetTransform Transform;
    Transform.Angle = AngleDegrees;  // don't add 90 degrees, only if pivot is wrong
    LineImage->SetRenderTransform(Transform);
    LineImage->SetRenderTransformPivot(FVector2D(0.f, 0.5f)); // ?? rotate around left-middle

    //LineImage->SetRenderTransformAngle(AngleDegrees);
    LineImage->SetColorAndOpacity(Tint);
}
