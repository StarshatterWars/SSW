#pragma once
#include "CoreMinimal.h"

class UWidgetTree;
class UWidget;
class UVerticalBox;
class UHorizontalBox;
class UButton;

namespace ModernMenuLayout
{
    struct FShell { UWidget* Root=nullptr; UVerticalBox* Body=nullptr; UHorizontalBox* Actions=nullptr; };
    FShell Build(UWidgetTree* Tree,const FString& Title,bool bTransparentBody=false);
    UButton* AddButton(UWidgetTree* Tree,UHorizontalBox* Strip,UButton* Existing,const FString& Caption);
    UButton* AddArrow(UWidgetTree* Tree,UHorizontalBox* Strip,UButton* Existing,bool bAccept);
    void SetSelected(UButton* Button,bool bSelected);
}
