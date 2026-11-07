#pragma once
#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
class AFighterActor;
class SFighterHUDDetails;
class UWidgetComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;
// Local presentation only. Widgets read the same sampled HUD data as the external view.
class FCockpitDisplays
{
public:
    ~FCockpitDisplays();
    void Update(AFighterActor* Fighter, TSharedRef<SFighterHUDDetails> HUD, bool Visible);
    bool IsActive() const;
    bool HandleGearClick();
    void Clear();
private:
    TWeakObjectPtr<UStaticMeshComponent> Mesh;
    TArray<TWeakObjectPtr<UWidgetComponent>> Panels;
    TStrongObjectPtr<UMaterialInterface> OriginalHUD;
    TStrongObjectPtr<UTexture2D> Blank;
};
